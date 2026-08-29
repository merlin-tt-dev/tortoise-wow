# LivingBots Clean Port — bundle 0042–0049

Target repository: `merlin-tt-dev/tortoise-wow`
Target branch: `livingbots/playerbots-clean-base`
Required baseline: `9bdbd3fa75887460329a8ce7b57de6d5d4ecfda1` (`Patching Build/Compile: Native Trainer RPG`)

This bundle contains eight source patches. Apply them strictly in the order listed in `APPLY_ORDER.txt`.

## Contents
- 0042: native creature-template storage cleanup
- 0043: native Generic/Range trigger APIs
- 0044: native formation location/flight APIs
- 0045: native RTSC spell visual/summon APIs
- 0046: native battleground/AV/WS APIs
- 0047: native active-quest/status model
- 0048: native loot APIs and obsolete loot shim removal
- 0049: module-owned auction price snapshot and native item-usage APIs

## Apply
From the repository root, with this bundle unpacked under `patches/livingbots-clean-port-bundle-0042-0049/`:

```bash
while IFS= read -r p; do
    git apply --check "patches/livingbots-clean-port-bundle-0042-0049/patches/$p" || exit 1
    git apply "patches/livingbots-clean-port-bundle-0042-0049/patches/$p" || exit 1
done < patches/livingbots-clean-port-bundle-0042-0049/APPLY_ORDER.txt
```

The loop is finite; it exits after the eight listed patches.

## Validation
- fresh full apply chain from archived pre-0038 source through 0049: PASS
- reverse/reapply of 0042–0049: PASS
- byte comparison after fresh apply: 24/24 PASS
- byte comparison after reapply: 24/24 PASS
- cumulative changed translation units: 19/19 PASS
- fresh changed translation units: 19/19 PASS

See `audits/` for details. One parallel `-j4` fresh-build attempt hit an OOM kill in `cc1plus`; the affected object was immediately rebuilt serially and passed. This is retained in the audit logs.
