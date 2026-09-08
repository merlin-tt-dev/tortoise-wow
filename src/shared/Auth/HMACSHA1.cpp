/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
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

#include "Auth/HMACSHA1.h"
#include "BigNumber.h"

HMACSHA1::HMACSHA1(uint32 len, uint8 *seed)
{
    if (seed && len > 0)
        m_key.assign(seed, seed + len);
}

HMACSHA1::~HMACSHA1() = default;

void HMACSHA1::UpdateBigNumber(BigNumber *bn)
{
    UpdateData(bn->AsByteArray());
}

void HMACSHA1::UpdateData(const std::vector<uint8>& data)
{
    m_data.insert(m_data.end(), data.begin(), data.end());
}

void HMACSHA1::UpdateData(const uint8 *data, int length)
{
    if (data && length > 0)
        m_data.insert(m_data.end(), data, data + length);
}

void HMACSHA1::UpdateData(const std::string &str)
{
    UpdateData(reinterpret_cast<uint8 const*>(str.data()), str.length());
}

void HMACSHA1::Finalize()
{
    unsigned int length = 0;
    MANGOS_ASSERT(HMAC(EVP_sha1(), m_key.data(), static_cast<int>(m_key.size()),
        m_data.data(), m_data.size(), m_digest, &length));
    MANGOS_ASSERT(length == SHA_DIGEST_LENGTH);
}

uint8 *HMACSHA1::ComputeHash(BigNumber *bn)
{
    UpdateBigNumber(bn);
    Finalize();
    return m_digest;
}
