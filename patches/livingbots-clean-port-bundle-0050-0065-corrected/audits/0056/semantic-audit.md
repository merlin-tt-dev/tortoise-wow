# Semantic audit — 0056

## Native temp summon and dungeon search APIs

- Migrates RTSC, LOS, wait-for-attack and dungeon/grid-search code to native temporary summon and grid-search APIs.
- Uses the established session-local spell visual packet model for RTSC feedback.
- Removes the now-unused `TEMPSPAWN` compatibility block and related dead compatibility helpers.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
