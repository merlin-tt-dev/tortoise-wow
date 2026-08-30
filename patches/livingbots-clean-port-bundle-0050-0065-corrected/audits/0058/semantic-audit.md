# Semantic audit — 0058

## Native class trigger APIs

- Migrates Druid attacker/mana checks and Rogue melee reach to native APIs.
- Uses explicit `GetPowerType() != POWER_MANA` semantics instead of legacy `HasMana` assumptions.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
