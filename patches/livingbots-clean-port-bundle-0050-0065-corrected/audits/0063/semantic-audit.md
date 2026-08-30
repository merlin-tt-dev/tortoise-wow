# Semantic audit — 0063

## Native locale storage APIs

- Replaces removed storage-locale lookup with Penqle `GetOrNewIndexForLocale()` for the Playerbot help/text storage maps.
- This preserves the module database locale columns while using the host storage-index mechanism.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
