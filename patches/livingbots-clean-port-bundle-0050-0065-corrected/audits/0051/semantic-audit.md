# Semantic audit — 0051

## Native spell readiness and cast APIs

- Generalizes `ServerFacade::IsSpellReady` from `Player*` to `Unit*` and maps player/pet/vehicle cooldown checks to native unit cooldown APIs.
- Migrates generic spell/item casting to native melee reach, spell interruption, group/owner resolution, `_ItemSpell`, client-started spell state and item cooldown/lifecycle APIs.
- Updates Hunter, Mage, Priest, Warrior and spell/value consumers without recreating removed fork helpers.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
