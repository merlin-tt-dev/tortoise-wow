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

#include "Auth/Sha1.h"
#include "Auth/BigNumber.h"
#include "Log.h"
#include <stdarg.h>

Sha1Hash::Sha1Hash() : m_ctx(EVP_MD_CTX_new())
{
    MANGOS_ASSERT(m_ctx);
    MANGOS_ASSERT(EVP_DigestInit_ex(m_ctx, EVP_sha1(), nullptr) == 1);
}

Sha1Hash::~Sha1Hash()
{
    EVP_MD_CTX_free(m_ctx);
}

void Sha1Hash::UpdateData(const uint8 *dta, int len)
{
    MANGOS_ASSERT(EVP_DigestUpdate(m_ctx, dta, len) == 1);
}

void Sha1Hash::UpdateData(const std::vector<uint8>& data)
{
    MANGOS_ASSERT(EVP_DigestUpdate(m_ctx, data.data(), data.size()) == 1);
}

void Sha1Hash::UpdateData(const std::string &str)
{
    UpdateData((uint8 const*)str.c_str(), str.length());
}

void Sha1Hash::UpdateBigNumbers(BigNumber *bn0, ...)
{
    va_list v;
    BigNumber *bn;

    va_start(v, bn0);
    bn = bn0;
    while (bn)
    {
        UpdateData(bn->AsByteArray());
        bn = va_arg(v, BigNumber *);
    }
    va_end(v);
}

void Sha1Hash::Initialize()
{
    MANGOS_ASSERT(EVP_DigestInit_ex(m_ctx, EVP_sha1(), nullptr) == 1);
}

void Sha1Hash::Finalize(void)
{
    unsigned int length = 0;
    MANGOS_ASSERT(EVP_DigestFinal_ex(m_ctx, mDigest, &length) == 1);
    MANGOS_ASSERT(length == SHA_DIGEST_LENGTH);
}
