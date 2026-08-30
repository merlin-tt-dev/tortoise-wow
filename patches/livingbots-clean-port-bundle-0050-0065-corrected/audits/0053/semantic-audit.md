# Semantic audit — 0053

## Native DebugAction APIs

- Rewrites the large DebugAction compatibility residue onto native quest, graveyard, transport, summon, packet, faction, player lookup, item and map APIs.
- Replaces private quest-slot access with public quest-state operations (`IsCurrentQuest`, `AddQuest`, PlayerbotAI drop path).
- Does not add an anticheat bypass: socketless bot sessions already use the core null-anticheat path.
- Also fixes a silent stream bug in level debug output where a float was combined with `||` instead of inserted into the stream.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
