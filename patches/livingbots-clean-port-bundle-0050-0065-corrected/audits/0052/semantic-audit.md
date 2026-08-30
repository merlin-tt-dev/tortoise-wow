# Semantic audit — 0052

## Native class item-use APIs

- Replaces Rogue and Warlock `GetItemByEntry` assumptions with native inventory traversal/item spell access.
- Removes the remaining normal class-code dependency on the fork-style item lookup helper.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
