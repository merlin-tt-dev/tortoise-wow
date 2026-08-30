# Semantic audit — 0059

## Native repair and stuck APIs

- Migrates repair state and AFK/player lookup calls to native APIs.
- For MMap stuck detection, checks the actual Detour tile state before/after native load rather than interpreting `loadMap()` return value as a loaded-state flag.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
