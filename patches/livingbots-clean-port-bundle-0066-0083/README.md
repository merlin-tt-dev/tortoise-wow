# LivingBots Clean Port Bundle 0066-0083

Target repository: `merlin-tt-dev/tortoise-wow`  
Target branch: `livingbots/playerbots-clean-base`  
Required remote baseline: `35eaa0cf31a79106ae8af2a872bf2ff03f55534e`  
Base branch `1181dev` must not be modified.

## Contents

- 18 semantic Mod patches: 0066 through 0083.
- 0 Core patches. `patches/core/` is intentionally empty.
- Per-patch change/build/semantic audit notes.
- Fresh apply / reverse / reapply bundle validation.
- Separate Turtle/Penqle LFG semantic audit.
- Shyalya LFG reference audit (reference only; no code copied).
- 0084 PCH/shim follow-up note; 0084 source changes are intentionally excluded.

## Build reference

Validated against ACE Debian **8.0.2** with Ninja and the established audit configuration:

- `MODULE_MOD_PLAYERBOTS=static`
- `USE_PCH=OFF`
- `USE_SCRIPTS=ON`
- `USE_EXTRACTORS=OFF`

Quick compile validation uses the real compile command/defines/includes, `-fsyntax-only -O0 -g0` for syntax and serial `-O0 -g0` object builds for affected TUs. Runtime/tool timeouts are not treated as compiler errors.

## Apply

From a clean checkout of `livingbots/playerbots-clean-base` at exactly `35eaa0cf31a79106ae8af2a872bf2ff03f55534e`:

```bash
while IFS= read -r patch; do
    git apply --check "patches/livingbots-clean-port-bundle-0066-0083/patches/$patch" || exit 1
    git apply "patches/livingbots-clean-port-bundle-0066-0083/patches/$patch" || exit 1
done < patches/livingbots-clean-port-bundle-0066-0083/APPLY_ORDER.txt

git diff --check
```

If applying from this standalone archive rather than after copying it under the repository `patches/` directory, replace the path prefix with the extracted bundle path.

## LFG decision

Turtle/Penqle Vanilla LFG is treated as its own architecture. Active `MANGOSBOT_ZERO` behavior remains based on native `sLFGMgr` Meeting-Stone queue operations. Dormant `MANGOSBOT_TWO` WotLK proposal/role/teleport APIs are **not** mapped onto Vanilla with fake shims. See `audits/LFG-TURTLE-SEMANTIC-AUDIT.md`.

## Excluded follow-up

0084 is deliberately not part of this bundle. A separate header/PCH probe exposed older compatibility-shim structural issues outside the official `USE_PCH=OFF` build. Those require a separate semantic pass before any source patch is frozen.
