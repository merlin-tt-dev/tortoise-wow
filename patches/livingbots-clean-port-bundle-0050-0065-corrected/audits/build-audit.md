# Build audit — 0050–0065 corrected ownership split

## Baseline and patch chain

- Repository: `merlin-tt-dev/tortoise-wow`
- Branch: `livingbots/playerbots-clean-base`
- Required remote baseline: `851b6d229d9b8a13beefcf669393b4addc6229c9`
- Semantic patch numbers: 16 (`0050` through `0065`)
- Physical patch files: 17 because 0050 is split into one core file and one module file
- Apply order: exactly as listed in `../APPLY_ORDER.txt`

## Ownership verification

- `patches/core/0050-core-native-login-query-holder-extract.patch`: core files only
- `patches/mod/0050-mod-native-playerbot-login-query-holder.patch`: mod-playerbots files only
- `patches/mod/0051...0065`: mod-playerbots files only
- No other 0051–0065 patch contains a `src/` core diff.

## Integrity gates after split

- Fresh `git apply --check` + apply: **17/17 PASS** (`fresh-stack-apply-split.txt`)
- Fresh final tree vs frozen worktree: **64/64 changed files byte-identical** (`fresh-byte-compare-split.txt`)
- Reverse 0065 → 0050-mod → 0050-core + reapply 0050-core → 0050-mod → 0065: **PASS** (`reverse-reapply-split.txt`)
- Post-reapply byte comparison: **64/64 PASS** (`fresh-reapply-byte-compare-split.txt`)
- All forward patch diffs pass `git diff --check`.

## Compiler gates

The split does not change source bytes, so the existing compiler gates remain directly applicable to the corrected package result:

- Directly changed C++ translation units: **41/41 object PASS/present** (`changed-cpp-object-check.txt`).
- Class contexts: **9/9 object PASS/present** (`class-context-object-check.txt`).
- Central `AiObjectContext.cpp`: **compiler syntax PASS, RC=0** (`central-context-syntax.log`).
- Core 0050 `CharacterHandler.cpp`: object PASS.
- Mod 0050 `PlayerbotLoginMgr.cpp`: object PASS.

## Known non-error note

Reverse application can report whitespace warnings because reverse restores pre-existing baseline whitespace. Forward diffs remain clean. See `reverse-whitespace-note.md`.
