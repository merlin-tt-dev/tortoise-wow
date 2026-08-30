# Semantic audit — 0054

## Native talent and combat value APIs

- Keeps `Player::CalculateTalentsPoints()` private and reproduces the host formula locally from level points, public bonus talent count and `Rate.Talent`.
- Migrates attacker/rank and battleground group/type lookups to native APIs.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
