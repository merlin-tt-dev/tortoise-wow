# Shyalya LFG reference audit

Reference repository: `Shyalya/tortoise-wow`  
Reference branch: `playerbots-integration-gh`  
Reference HEAD inspected: `be0706b7b20d6f91ec917c5f9e3501ee7b5ada04`

This audit is **reference-only**. No Shyalya LFG code is copied into this bundle.

## What Shyalya did

Shyalya's PlayerBots live under `src/modules/PlayerBots`. In the ZERO branch of `playerbot/strategy/actions/LfgActions.cpp`, dungeon selection first calls:

`MeetingStoneSet stones = sWorld.GetLFGQueue().GetDungeonsForPlayer(bot);`

and returns `false` immediately when that set is empty. Queue insertion later calls native `sLFGMgr.AddToQueue(bot, stoneInfo.area)`.

To support the old bot API, Shyalya also changed Core `World` to expose `World::GetLFGQueue()`, forwarding it to `sLFGMgr`. More importantly, Shyalya's `src/game/LFG/LFGMgr.h` adds cmangos compatibility types and a `GetDungeonsForPlayer(Player*)` method whose implementation is an inline stub returning `{}`.

## Consequence

Because the ZERO action rejects an empty `stones` set, that compatibility implementation makes this specific automatic dungeon-join path return `false` before reaching `sLFGMgr.AddToQueue`. In other words, the adapter makes the code compile but does not provide functional dungeon selection.

## Decision for LivingBots

Do not adopt this compatibility pattern. In particular:

- no `World::GetLFGQueue()` Core adapter solely for Playerbot;
- no `GetDungeonsForPlayer()` empty stub;
- no fake `MeetingStoneSet` abstraction;
- retain Penqle's native Meeting-Stone queue and implement any future bot dungeon-choice policy above it in the Mod.

Useful reference lesson: compile compatibility is insufficient here; dungeon-selection semantics must be real.
