/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
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

#ifndef __UPDATEMASK_H
#define __UPDATEMASK_H

#include "UpdateFields.h"
#include "Errors.h"

class UpdateMask
{
    public:
        UpdateMask() : mCount(0), mBlocks(0) {}

        void SetBit (uint32 index)
        {
            (reinterpret_cast<uint8*>(mUpdateMask.data()))[ index >> 3 ] |= 1 << ( index & 0x7 );
        }

        void UnsetBit (uint32 index)
        {
            (reinterpret_cast<uint8*>(mUpdateMask.data()))[ index >> 3 ] &= (0xff ^ (1 <<  ( index & 0x7 ) ) );
        }

        bool GetBit (uint32 index) const
        {
            return ( (reinterpret_cast<uint8 const*>(mUpdateMask.data()))[ index >> 3 ] & ( 1 << ( index & 0x7 ) )) != 0;
        }

        uint32 GetBlockCount() const { return mBlocks; }
        uint32 GetLength() const { return mBlocks << 2; }
        uint32 GetCount() const { return mCount; }
        uint8* GetMask() { return reinterpret_cast<uint8*>(mUpdateMask.data()); }

        void SetCount(uint32 valuesCount)
        {
            mCount = valuesCount;
            mBlocks = (valuesCount + 31) / 32;
            mUpdateMask.assign(mBlocks, 0);
        }

        void Clear()
        {
            std::fill(mUpdateMask.begin(), mUpdateMask.end(), 0);
        }

        void operator &= ( const UpdateMask& mask )
        {
            //MANGOS_ASSERT(mask.mCount <= mCount);
            for (uint32 i = 0; i < mBlocks; ++i)
                mUpdateMask[i] &= mask.mUpdateMask[i];
        }

        void operator |= ( const UpdateMask& mask )
        {
            //MANGOS_ASSERT(mask.mCount <= mCount);
            for (uint32 i = 0; i < mBlocks; ++i)
                mUpdateMask[i] |= mask.mUpdateMask[i];
        }

        UpdateMask operator & ( const UpdateMask& mask ) const
        {
            //MANGOS_ASSERT(mask.mCount <= mCount);

            UpdateMask newmask;
            newmask = *this;
            newmask &= mask;

            return newmask;
        }

        UpdateMask operator | ( const UpdateMask& mask ) const
        {
            //MANGOS_ASSERT(mask.mCount <= mCount);

            UpdateMask newmask;
            newmask = *this;
            newmask |= mask;

            return newmask;
        }

    private:
        uint32 mCount;
        uint32 mBlocks;
        std::vector<uint32> mUpdateMask;
};
#endif
