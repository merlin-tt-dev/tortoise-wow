# 0041 build/apply audit

## Build environment

- Target: Tortoise/Classic (`MANGOSBOT_ZERO`)
- CMake generator: Ninja
- `MODULE_MOD_PLAYERBOTS=static`
- `USE_PCH=OFF`
- Debian ACE 8.0.2 bundle supplied by repository owner
- Fresh stack source: 0037 archive + 0038 core + 0038 mod + 0039 + 0040 + 0041

## Changed translation units

Fresh stack object compilation:

- `TrainerAction.cpp.o` — PASS
- `RpgTriggers.cpp.o` — PASS
- `TrainerValues.cpp.o` — PASS
- `AutoLearnSpellAction.cpp.o` — PASS

Result: **4/4 PASS**.

See `fresh-changed-tu-build.log` and `fresh-cmake.log`.

## Apply/reversibility

Local audited WIP:

- patch SHA256 equals `git diff --binary --full-index` SHA256 — PASS
- reverse-check on patched tree — PASS
- reverse to clean reconstructed remote baseline — PASS
- forward-check on clean baseline — PASS
- reapply — PASS
- reapplied diff byte-identical to frozen patch — PASS
- five patched result files byte-identical — PASS

Fresh requested stack:

- 0038 core — apply/check PASS
- 0038 mod — apply/check PASS
- 0039 — apply/check PASS
- 0040 — apply/check PASS
- 0041 — apply/check PASS
- 0041 reverse-check with full stack — PASS
- 0040 reverse-check while 0041 remains present — PASS
- 0041 reverse/reapply on top of 0040 — PASS
- five 0041 files byte-identical to audited WIP — PASS

## Known unrelated next blocker

A dependency probe of `TravelValues.cpp` with PCH disabled still fails at its pre-existing use of `sCreatureStorage` because that compatibility object is only made visible through the PCH/shim inclusion path. This file is not changed by 0041, and its blocker predates 0041. It is a candidate for the next compatibility sweep rather than a reason to broaden the trainer patch.
