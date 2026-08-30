# Semantic audit — 0055

## Native combat target and threat APIs

- Migrates combat targetability, threat access, target GUID, owner/group resolution and melee reach to Penqle native APIs.
- Uses native DynamicObject caster/aura-holder semantics for AOE evaluation instead of fork-only helpers.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
