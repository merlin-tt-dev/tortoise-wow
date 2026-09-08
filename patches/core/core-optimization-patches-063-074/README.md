# Core optimization patches 063-074

This bundle continues the `dev-clean` Core Optimization series.

## Source of truth / prerequisite

The sole source of truth used to build and validate this bundle was the uploaded archive:

- `tortoise-wow.tar.xz`
- SHA256: `817b100bd63654ce2f908bb699fa76c071caf421c6e5470e460f7ec441460b9e`
- archive source code contains optimizations `001-041` already applied
- archive contains patch files `042-062`, not yet applied to source

Before applying this bundle, reconstruct the expected baseline by applying `042-062` in numerical order. Then apply `063-074` exactly in `APPLY-ORDER.txt` order.

No GitHub/remote state was used as an implementation baseline and no remote writes were performed.

## Status

`063-074` passed the strict patch audit individually and as an ordered series.

Two independent fresh-archive reconstructions were performed:

1. fresh archive -> strict `042-062` sequence -> committed local `062` baseline -> strict `063-074` sequence
2. second fresh archive -> uninterrupted strict `042-074` sequence

For all 41 source files touched by `042-074`, final SHA256 fingerprints were byte-identical between both independent reconstructions. The same 41-file fingerprint also matches the current local `dev-clean-work` source tree.

No Linux/MinGW/MSVC full build is declared by this bundle. Compile/link remains the external build gate.

## Strict checks used for every patch

```bash
git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --check --whitespace=error-all PATCH

git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --numstat --whitespace=error-all PATCH

git apply PATCH

git diff --check
```

Additional checks:

- no tabs in newly added indentation
- no added conflict markers
- final source-tree scan for real `<<<<<<<` / `>>>>>>>` markers
- full ordered sequence validation
- independent fresh-archive reconstruction
- byte-for-byte SHA256 fingerprint comparison of all touched source files

`AUDIT-TEST.sh` reproduces the strict `063-074` sequential apply audit against a test copy already reconstructed through `062`.

## Patch scope

- `063` — repairs lookup-shortening semantics around battleground active events; adds explicit insert-if-absent boss-expiration semantics and avoids computing the timestamp on cache hits.
- `064` — removes repeated Channel member-map lookups while preserving membership/moderator behavior.
- `065` — reduces AccountMgr loader copies/repeated map access.
- `066` — reuses VMap loaded-spawn iterators for reference-count updates/removal.
- `067` — makes Unit grid-list searching container-generic and converts selected target-selection snapshots from node-based lists to vectors while preserving encounter order.
- `068` — uses `equal_range`, single insert tests and cached spell-group ranges.
- `069` — replaces ShopMgr 20 ms request polling with `condition_variable` wakeups; request work stays outside the queue mutex and shutdown explicitly wakes the worker.
- `070` — converts additional Unit nearby snapshots to vectors using the generic searcher introduced by `067`.
- `071` — generalizes Player/Creature/GameObject list searchers and uses vector snapshots in selected object/grid hotpaths.
- `072` — collapses PlayerSocial map lookup/insertion to one associative operation.
- `073` — uses move-aware associative storage in LFT queue/offer/listing paths while retaining the required post-broadcast listing lookup semantics.
- `074` — removes a full node-by-node `ThreatList` copy in `FindLowestHpHostileUnit()` by using the existing const reference.

## Important semantic audit result

The requested re-audit of earlier `find/count/operator[]` shortening found one real issue in optimization `057`: `BattleGround::SpawnEvent()` could create an `m_ActiveEvents` entry when `event2 == BG_EVENT_NONE`, whereas the historical short-circuit path created none. Patch `063` restores the original no-op/no-insertion behavior.

Other reviewed lookup reductions remained semantically equivalent. A repeated MMap lookup around a lock was intentionally retained because it is a synchronization double-check rather than redundant lookup work.
