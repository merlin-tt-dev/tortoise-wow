# LivingBots Clean Port — Bundle 0050–0065 (corrected ownership split)

This corrected bundle replaces the earlier 0050–0065 archive in which the 0050 core and module changes were combined in one patch file.

## Target

- Repository: `https://github.com/merlin-tt-dev/tortoise-wow`
- Branch: `livingbots/playerbots-clean-base`
- Required baseline HEAD: `851b6d229d9b8a13beefcf669393b4addc6229c9`
- Base branch `1181dev` must not be modified.

## Permanent patch ownership layout

```text
patches/
├── core/
│   └── core-only patches
└── mod/
    └── mod-playerbots-only patches
```

Patch numbering remains global. A numbered compatibility step may therefore have both a core and mod file when ownership requires it. The exact order is always authoritative in `APPLY_ORDER.txt`.

**Core changes are never to be hidden inside a mod patch.** Future bundles keep this ownership split; when a bundle has no core patch, `patches/core/` may simply be empty.

## 0050 split

0050 is intentionally two patch files:

1. `patches/core/0050-core-native-login-query-holder-extract.patch`
   - `src/game/Handlers/CharacterHandler.cpp`
   - `src/game/Handlers/LoginQueryHolder.h`
2. `patches/mod/0050-mod-native-playerbot-login-query-holder.patch`
   - `modules/mod-playerbots/src/playerbot/PlayerbotLoginMgr.cpp`
   - `modules/mod-playerbots/src/playerbot/PlayerbotLoginMgr.h`

Apply the core file first, then the mod file.

0051–0065 are module-only and live exclusively in `patches/mod/`.

## Apply

Apply **only** in the order in `APPLY_ORDER.txt`.

If this bundle is stored as `patches/livingbots-clean-port-bundle-0050-0065-corrected/`:

```bash
while IFS= read -r patch; do
    git apply --check -v "patches/livingbots-clean-port-bundle-0050-0065-corrected/patches/$patch" || exit 1
    git apply -v "patches/livingbots-clean-port-bundle-0050-0065-corrected/patches/$patch" || exit 1
done < patches/livingbots-clean-port-bundle-0050-0065-corrected/APPLY_ORDER.txt
```

The loop is finite and exits after the 17 patch files listed there.

## Validation summary

- Semantic patch numbers: **0050–0065 = 16 numbered steps**
- Physical patch files after ownership split: **17**
- Fresh apply chain: **17/17 CHECK + APPLY PASS**
- Final changed-file identity: **64/64 PASS**
- Full reverse/reapply: **PASS**
- Post-reapply identity: **64/64 PASS**
- Directly changed C++ TUs: **41/41 object PASS**
- Class contexts: **9/9 object PASS**
- Central `AiObjectContext.cpp`: **compiler syntax PASS (RC=0)**
- Every forward patch: `git diff --check` clean

The ownership split changes only patch packaging. The final source tree is **64/64 byte-identical** to the already compiler-validated 0065 freeze.

See `audits/build-audit.md` and `audits/<patch-number>/` for details.
