# 0048 semantic audit
- Removes the obsolete `sLootMgr` compatibility layer and uses Penqle native loot object/template paths.
- Keeps Vanilla roll semantics explicit; WotLK-only disenchant roll feedback is not exposed in ZERO builds.
- Fixes a pre-existing syntax defect discovered by the audit in `LootValues.cpp`.
- Removes compatibility defines/shims made unnecessary by the native migration.
- `LootAction.cpp`, `LootRollAction.cpp`, and `LootValues.cpp` compile: PASS.
