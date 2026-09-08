/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Auth/Hmac.h"
#include "BigNumber.h"

HmacHash::HmacHash(const uint8* data, int length)
{
    if (data && length > 0)
        m_key.assign(data, data + length);
}

HmacHash::~HmacHash() = default;

void HmacHash::UpdateBigNumber(BigNumber* bn)
{
    UpdateData(bn->AsByteArray());
}

void HmacHash::UpdateData(const std::vector<uint8>& data)
{
    m_data.insert(m_data.end(), data.begin(), data.end());
}

void HmacHash::UpdateData(const uint8* data, int length)
{
    if (data && length > 0)
        m_data.insert(m_data.end(), data, data + length);
}

void HmacHash::Finalize()
{
    unsigned int length = 0;
    MANGOS_ASSERT(HMAC(EVP_sha1(), m_key.data(), static_cast<int>(m_key.size()),
        m_data.data(), m_data.size(), m_digest, &length));
    MANGOS_ASSERT(length == SHA_DIGEST_LENGTH);
}
