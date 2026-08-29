# 0047 semantic audit
- Removes normal playerbot consumers of private quest-slot reads.
- Uses `Player::IsCurrentQuest`, PlayerbotAI active/incomplete quest helpers, and the quest status map according to the consumer's intent.
- Does not expose `Player::GetQuestSlotQuestId` publicly and does not add a core shim.
- `DebugAction`, `LootAction`, and `ItemUsageValue` were deliberately left for separate semantic clusters where additional unrelated APIs existed.
- Eight changed quest/guild TUs compile: PASS.
