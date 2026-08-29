# 0040 semantic audit

## Gossip

Old playerbot code used `GetPlayerMenu()`, `GossipText`, direct text arrays and `MAX_GOSSIP_TEXT_OPTIONS`.
Penqle exposes the active menu as `Player::PlayerTalkClass` and stores NPC gossip text as `NpcText` entries containing `BroadcastTextID` references.

0040 follows Penqle's native data model instead of recreating the old structs. Display text is resolved through `BroadcastText`, using the player's locale and the talking creature's gender.

## SayAction quest list

The old code reached into private quest-slot state. PlayerbotAI already exposes `GetCurrentIncompleteQuestIds()`, which is the proper module abstraction for the actual intent. 0040 uses it rather than adding a core getter or making private quest state public.

## WorldPacket ownership

Penqle's `WorldPacket` is move-oriented/non-copyable in the affected path. Debug packet prepending was changed to move packets into a reserved vector and swap, preserving ordering without adding copy compatibility shims.

## WorldLocation fields

The affected files only used obsolete CMaNGOS field names. They are mapped directly to Penqle's existing `mapId`, `x`, `y`, `z` fields; no behavior is added.

## Exclusions

Trainer, battleground, formation/free-flight and debug spell-visual clusters are independent semantic migrations and are intentionally not mixed into this patch.
