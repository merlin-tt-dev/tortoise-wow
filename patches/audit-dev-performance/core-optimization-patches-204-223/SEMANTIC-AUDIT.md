# Semantic Audit — Core Optimization 204-223

The audit target is preservation of successful-path observable behavior: ordering, mutation, lifetime, lookup result identity, RNG mapping, traversal order, packet-visible data and legacy null/dereference behavior. Resource-allocation failure timing is called out separately where allocation is intentionally removed or container strategy changes.

## 204 — item prototype lookup reuse

Repeated `Item::GetProto()` calls are replaced by a local pointer/value only within the same operation. `Item::GetProto()` is a direct lookup of `sObjectMgr.GetItemPrototype(GetEntry())`; the item entry is not changed between the replaced calls.

Audited properties:

- Guild-bank clone fields are copied from the same prototype and in the same field order.
- Trade packet field order and numeric narrowing/casts are unchanged.
- Corpse item display/inventory values are unchanged.
- Combat skill logic keeps the fishing-pole exclusion and branch structure.
- Loot template selection, money values and loot-state mutations retain their original order.
- Swap-item max-stack arithmetic and branch condition are unchanged.
- Armor update keeps shield/class/subclass checks and the same formula.
- Transmog uses the already-fetched destination prototype; the existing null return remains based on that lookup.
- Unexpected null prototypes still dereference/fail in paths that previously dereferenced them; the optimization does not silently turn those cases into successful skips.

The final patch also contains the freeze-time whitespace correction: newly added indentation uses spaces so the patch passes the strict no-new-tab policy. `git diff -w` against the pre-correction 204-223 audited tree was empty.

## 205 — last movement info reference reuse

`GetLastMovementInfo()` returns `_me->m_movementInfo` by reference. The patch binds that same member once per affected function and reuses the reference.

Audited properties:

- No snapshot/copy is introduced; mutations through the reference remain immediately visible.
- All reads after mutations observe the same member object they observed before.
- Movement flag, jump, timing, speed and anticheat logging expressions are otherwise unchanged.
- No lifetime extension or container/reference invalidation issue exists because the referenced object is a `Player` member.

## 206 — broadcast locale lookup reuse

Creature/item locale records are looked up once before each session loop instead of once per localized session.

Audited properties:

- Session iteration order is unchanged.
- Packet field offsets and per-session locale choice are unchanged.
- Only the stable locale-record lookup is hoisted; packet mutation and `SendPacket` remain in their original order.
- ObjectMgr locale storage is treated as stable for the duration of the synchronous broadcast loop, matching existing store-lifetime assumptions elsewhere in the core.

## 207 — invasion zone selection without temporary vector

The old code built a vector of candidate zone IDs in `invasionPoints` order and chose `validZones[urand(0, n-1)]`. The new code counts candidates, performs exactly one `urand(0, n-1)`, then returns the corresponding Nth candidate in the same source order.

Audited properties:

- Empty candidate behavior/log/return value remains unchanged.
- Candidate order is unchanged.
- RNG call count and bounds are unchanged.
- RNG index-to-zone mapping is unchanged.
- Differential simulation over varied exclusion pairs passed.

Resource note: the temporary vector allocation is intentionally removed, so an allocation failure that could previously occur at that point no longer occurs.

## 208 — AB graveyard direct scan

The old code first collected occupied node IDs in ascending node order, then scanned that vector. The new code scans occupied nodes directly in the same ascending order.

Audited properties:

- `BG_AB_NODES_MAX` traversal order remains ascending.
- Missing graveyard records are skipped identically.
- Strict `mindist > dist` comparison is unchanged, so equal-distance ties still select the earliest node.
- Player position is fetched only when at least one occupied node exists; this matches the old `!nodes.empty()` gate.
- Fallback starting graveyard behavior is unchanged.
- Differential simulation of node ownership, missing entries and ties passed.

Resource note: the temporary node vector allocation is intentionally removed.

## 209 — player social lookup reuse

`MasterPlayer::GetSocial()` is an inline return of `m_social`. A local pointer is used for the map lookup and end comparison. No map iterator escapes and no mutation is introduced between lookup and comparison.

## 210 — BigNumber byte length reuse

`GetNumBytes()` is `BN_num_bytes(_bn)`. The value is cached once and reused for output length, zero-fill decision and leading-padding offset.

Audited properties:

- `minSize` selection is unchanged.
- Zero padding remains leading padding.
- `BN_bn2bin` destination offset is unchanged.
- Reverse behavior is unchanged.

## 211 — extended fingerprint hash reuse

`AnalysisInfo::GetHash()` is a pure hash calculation over the current sample fields. The result is calculated once and reused both for membership lookup and the warning message, guaranteeing the same hash value is tested and reported.

## 212 — static playerbot combat action tables

Per-call `std::vector<Action>` construction is replaced by fixed `static constexpr Action[]` tables selected by class.

Audited properties:

- Every class retains exactly the same action tuples, values and source order.
- Class switch selection is unchanged.
- First matching/castable action still wins.
- Unknown/default class still executes zero actions and returns 0.
- Automated tuple extraction compared old per-class initializer lists against new arrays: all nine class tables matched exactly.

Resource note: the old per-call vector allocation/failure point is intentionally removed. Successful-path gameplay semantics are preserved; `std::bad_alloc` timing under extreme memory exhaustion is not preserved.

## 213 — direct playerbot spell-chain recursion

The two local `std::function` recursive closures are replaced by C++17 self-recursive generic lambdas.

Audited properties:

- Same first spell ID is used.
- Same `SpellChainMapNext::equal_range` iteration order is used.
- `GetHighestKnownSpell` updates `best` at the same traversal points.
- `TargetHasAuraFromChain` keeps the same early-return behavior.
- Existing behavior for malformed cyclic spell-chain data is not deliberately changed; both implementations recurse according to the same graph traversal.

Resource note: type-erasure/possible `std::function` allocation is removed, so allocation-failure timing can differ.

## 214 — config include membership

`loadedIncludeFiles` changes from `std::set<std::string>` to `std::unordered_set<std::string>`.

Usage sweep: one declaration and one `insert(...).second`; there is no iteration, `begin()`, ordering, RNG indexing, logging from container order, or iterator escape. Include processing order continues to come from `includeFiles`.

## 215 — event script membership

Both event-ID worksets and `CollectPossibleEventIds` use `std::unordered_set<uint32>`.

Usage sweep: the sets are populated by insertion and consumed only by `find/end` membership tests. They are never iterated. The initial implementation missed the second caller; freeze/audit corrected the signature and both callers in the same patch before release.

## 216 — CreatureEventAI action membership

`actionIds` changes to `std::unordered_set<uint32>`. Usage is insertion plus `find/end` only; no ordered observation exists.

## 217 — trainer validation membership

`skip_trainers` and `talentIds` change to `std::unordered_set<uint32>`. Both are validation/dedup worksets used only through membership checks/insertion. Loader row processing and diagnostics remain driven by database/result iteration order, not set order.

## 218 — creature spell-script validation membership

The full spell-script membership copy becomes `std::unordered_set<uint32>` constructed from the existing ordered source set. The copied container is only queried with `find/end`; it is not iterated.

## 219 — config key hash tracking

`knownKeys` changes from `std::map<std::string, ConfigKeyDefinition>` to `std::unordered_map`.

Audited properties:

- Metadata key iteration drives warning order and remains unchanged.
- The map is only `find`-queried and overwritten/inserted by key; it is never iterated.
- The previous definition is read before assignment. For an existing key, assignment does not invalidate the already-used iterator; for a new key the prior iterator is `end()` and is not used after insertion.
- Duplicate override semantics remain “latest processed definition wins”.

## 220 — faction enemy membership

`factionsWithEnemies` changes to `std::unordered_set<uint32>`. It is only populated and queried for membership; faction processing order remains external to the container.

## 221 — taxi spell-path membership

`spellPaths` changes to `std::unordered_set<uint32>`. It is only populated from spell effects and queried with `find/end` while taxi nodes/paths are iterated in their existing order.

## 222 — checked trainer-template membership

`checkedTrainerTemplates` changes to `std::unordered_set<uint32>`. It is only used to suppress already-processed trainer templates through `find`/`insert`; the surrounding creature/trainer traversal order remains unchanged.

## Hash-container resource/failure note (214-222)

For all ordered-to-hash conversions, no order is observed by successful-path program logic. The allocation strategy, bucket growth and exception timing under allocation failure necessarily differ from tree containers; this is an intentional performance/resource characteristic and is not claimed to be failure-timing equivalent.

## 223 — Warden pattern range construction

The temporary `std::vector<uint8>` is constructed directly from the `turtle_vector<uint8>` range instead of `resize` followed by `memcpy`.

Audited properties:

- Source and destination element type are both `uint8`.
- Destination size and byte sequence are unchanged.
- `WindowsCodeScan` still receives the same lvalue vector by const reference and performs its existing ownership/copy behavior; this patch deliberately does not introduce a move or alter scan ownership.
- No client/protocol layout or Warden scan matching semantics are changed.

## Explicitly outside this audit

The battleground/arena queue world-thread ownership patches already present in the Patch-203 baseline were not semantically re-audited. This bundle does not modify that ownership design.
