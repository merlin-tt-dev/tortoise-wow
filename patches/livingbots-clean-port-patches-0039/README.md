# LivingBots Clean Port — patch 0039

Purpose: native RPG / quest / mount / custom-spell compatibility pass after patch 0038.

Baseline:
- repository: `merlin-tt-dev/tortoise-wow`
- work branch: `livingbots/playerbots-clean-base`
- remote HEAD verified during development: `835f1c0a605da4323220e909ece5e0f081cd520c`
- prerequisite: apply patch package 0038 first.

This package is intentionally module-only. It does not write to the repository's `patches/` registry and does not modify `1181dev`.

Apply from repository root after 0038:

```sh
git apply --check mod/0039-build-compatibility-pass-10-native-rpg-quest-mount-spell.patch
git apply mod/0039-build-compatibility-pass-10-native-rpg-quest-mount-spell.patch
```

See `audits/` for semantic rationale, TU results, and apply/reverse/reapply verification.
