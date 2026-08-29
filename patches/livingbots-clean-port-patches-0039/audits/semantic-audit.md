# 0039 semantic audit

Baseline is patch 0038 applied on remote work-branch HEAD `835f1c0a605da4323220e909ece5e0f081cd520c`.

## Native migrations

- RPG gossip close: use Penqle `Player::PlayerTalkClass->CloseGossip()`; do not recreate fork-only `GetPlayerMenu()`.
- WorldLocation orientation: `.o` replaces obsolete `.orientation` field access.
- Minecart nearby-miner guard: use Penqle's native `NearestCreatureEntryWithLiveStateInObjectRangeCheck(obj, entry, alive, range)` signature. The behavior required here is nearest living entry 28841 within 500 yards.
- Manual RPG/LLM chat status: carry the real command requester into `ManualChat()` and reply through `PlayerbotAI::TellPlayerNoFacing()`. Do not add the fork-only `Player::SendMessageToPlayer()` API.
- RNG: all remaining `GetRandomGenerator()` consumers use module-native `std::mt19937` seeded via Penqle `urand`; remove that compatibility shim entirely.
- Quest abandonment: stop accessing private quest-log slots / obsolete `TakeQuestSourceItem`. Use existing `PlayerbotAI::DropQuest()` -> Penqle `Player::RemoveQuest()`, so core-owned source item, timed quest, quest item, status and slot teardown is preserved.
- Mount permission: the flag belongs to `MapEntry`; use `Map::GetMapEntry()->IsMountAllowed()` as Penqle core does in spell mount validation.
- Mount item ownership: use public `Player::HasItemCount(itemId, 1)` instead of fork-only `GetItemByEntry()`.
- Passive spell predicate: use native `Spells::IsPassiveSpell()` namespace.
- Spell recovery: use native `SpellEntry::GetRecoveryTime()` and remove `GetSpellRecoveryTime()` shim.
- Session packets: pass `WorldPacket*` to Penqle `WorldSession::SendPacket()`.

No core API was invented and no no-op compatibility behavior was added.
