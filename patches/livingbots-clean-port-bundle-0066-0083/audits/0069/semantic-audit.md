# Semantic audit 0069

## Native auction item stack count

Removes the non-native AuctionEntry::itemCount assumption. Resolves the auction Item through AuctionMgr and reads Item::GetCount(), with missing/zero-stack guards.

Core impact: **none**. This patch is Mod-only.
