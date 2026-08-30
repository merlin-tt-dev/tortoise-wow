# LivingBots Clean Port Bundle 0084-0100

Target repository: `merlin-tt-dev/tortoise-wow`  
Target branch: `livingbots/playerbots-clean-base`  
Required remote baseline: `1534319ee2ecead76482035f7c15be3e3cd5e5ad`  
Base branch `1181dev` must not be modified.

## Scope

- **17 Mod patches:** 0084 through 0100 (Compatibility Pass 55 through 71).
- **1 Core patch:** `core-0012-native-graveyard-map-readonly.patch`.
- 0083 is **not** included because it is already part of the required `1534319ee2ecead76482035f7c15be3e3cd5e5ad` baseline.
- The old `cmangos-compat-shim.h` and empty compatibility stub layer are removed by this batch.
- Values and Generic strategy sweeps are green through this source state; class sweep progress after 0100 contains no additional source changes and is therefore not bundled.

## Build reference

Validated during the source audit against ACE Debian **8.0.2** with Ninja and:

- `MODULE_MOD_PLAYERBOTS=static`
- `USE_PCH=OFF`
- `USE_SCRIPTS=ON`
- `USE_EXTRACTORS=OFF`

The 0084 PCH probe additionally verified the header path directly; after 0084 `botpch.h` went from six hard errors to RC=0, and later shim-removal steps kept `botpch.cpp` building as a real object.

## Apply

From a clean checkout at exactly `1534319ee2ecead76482035f7c15be3e3cd5e5ad`:

```bash
while IFS= read -r patch; do
    git apply --check "patches/livingbots-clean-port-bundle-0084-0100/patches/$patch" || exit 1
    git apply "patches/livingbots-clean-port-bundle-0084-0100/patches/$patch" || exit 1
done < patches/livingbots-clean-port-bundle-0084-0100/APPLY_ORDER.txt

git diff --check
```

`core-0012` appears in `APPLY_ORDER.txt` immediately before Mod 0094, which is the first Mod patch that consumes its read-only accessor.

## Architecture notes

This batch continues the clean-port rule: adapt Playerbot to Penqle/Tortoise native APIs instead of expanding a fork compatibility surface. The compatibility shim is removed rather than replaced. The sole Core addition is a minimal read-only accessor to already-loaded graveyard data; Core and Mod ownership remain physically separate.

Turtle/ZERO LFG remains on native `sLFGMgr`. No WotLK proposal/teleport queue architecture or Shyalya-style `World::GetLFGQueue()` compatibility adapter is introduced here. The later LivingBots LFG work is an AI/dungeon-selection layer above the native Vanilla queue.
