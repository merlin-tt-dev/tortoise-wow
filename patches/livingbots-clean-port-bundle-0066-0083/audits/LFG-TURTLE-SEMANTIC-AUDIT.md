# Turtle/Penqle LFG semantic audit

## Active Vanilla / MANGOSBOT_ZERO

The active Turtle path is based on Penqle's native Meeting-Stone manager (`sLFGMgr`). Join/leave/queue-state operations use the native manager rather than the old cmangos/WotLK queue abstraction. Group queue area is available through native group state. Solo queue area is observed module-side from the real Meeting-Stone join opcode because Penqle intentionally keeps internal queue details private.

Decision: **keep the ZERO architecture native**. Do not expose a bot-only Core getter solely to mimic `sWorld.GetLFGQueue()`.

## Dormant MANGOSBOT_TWO

The TWO code contains a different WotLK LFG architecture: queue-data objects, role/dungeon sets, proposals, proposal updates, join/leave orchestration and dungeon teleport. Penqle Vanilla Meeting Stone does not have one-to-one equivalents.

Decision: do **not** search/replace `sWorld.GetLFGQueue()` with `sLFGMgr` in TWO code and do not fabricate proposal/teleport shims. Future LivingBots Turtle LFG improvements should form a Vanilla-specific layer above `sLFGMgr` (dungeon selection, group intent, retry/timeout behavior), using TWO only as behavioral reference.
