# 0049 semantic audit
- Replaces the incompatible assumption that Penqle `AuctionEntry` stores an item stack count.
- `RandomPlayerbotMgr::MirrorAh()` now snapshots only the fields pricing needs plus the native `Item::GetCount()` value into module-owned `AuctionPriceEntry` records.
- Avoids extending/altering the core `AuctionEntry` structure.
- Replaces old player APIs with native equivalents (`GetPowerType() == POWER_MANA`, `GetGUIDLow()`).
- Removes the remaining private quest-slot read in `ItemUsageValue` using native current-quest/status APIs.
- Uses `Spells::IsPassiveSpell`.
- Both `RandomPlayerbotMgr.cpp` and `ItemUsageValue.cpp` compile: PASS.
