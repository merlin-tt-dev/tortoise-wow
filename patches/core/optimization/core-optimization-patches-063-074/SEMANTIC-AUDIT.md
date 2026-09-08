# Semantic audit notes — core optimization 063-074

## Lookup-shortening re-audit

The previous associative lookup optimizations were rechecked with emphasis on transformations of these shapes:

- `find()` followed by `operator[]`
- `count()` followed by `operator[]`
- repeated `find()` calls
- lower/upper-bound pairs
- insert/update paths whose default insertion behavior may be observable

### Corrected in 063

`core-optimization-057-single-lookup-associative-updates.patch` changed `BattleGround::SpawnEvent()` by performing `try_emplace(event1)` before testing `event2 == BG_EVENT_NONE`.

Historical behavior used short-circuiting and did not touch `m_ActiveEvents` for `BG_EVENT_NONE`. The shortened version could create a default entry. `063` restores the original semantic boundary by returning before insertion.

The PTR world-boss expiration path is also expressed as an explicit class operation, `TryAddBossExpiration()`: insert only if absent, and evaluate `time(nullptr)` only for a real insertion.

### Intentionally not shortened

The repeated MMap lookup associated with a lock was retained. The second lookup is a synchronization double-check and cannot be removed as if it were an ordinary redundant associative lookup.

### Other checked reductions

The reviewed earlier single-lookup/range reductions did not show an observable semantic change. In particular, `068` intersects the already normalized direct spell-group ranges; `IsSpellMemberOfSpellGroup()` itself tests that same direct mapping after chain normalization, so the cached-range form preserves that behavior.

## Container-order audit

`067`, `070` and `071` replace selected `std::list` snapshots with `std::vector` while leaving the existing grid visitation sequence intact. Filtering uses stable `erase/remove(_if)` behavior, preserving the relative order of surviving candidates. Random target selection therefore maps an unchanged random index to the same candidate ordering.

## Threading audit

`069` changes ShopMgr from timed polling to condition-variable notification. The queue mutex protects only request-queue inspection/swap. Request processing and DB/shop work remain outside the queue lock. Shutdown explicitly notifies the worker before joining it.

## LFT iterator/lifetime audit

`073` intentionally does not retain a listing iterator across `BroadcastGroupsList()`: that path can call `EnsureListingsLoaded()` and reload/clear the listings map. The required post-broadcast lookup remains, while insertion/storage itself is move-aware.
