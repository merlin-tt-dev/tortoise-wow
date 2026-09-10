# Semantic audit summary — Core Optimization 124-143

## Adversarial second-pass policy

After the initial bundle pass, the complete 124-143 series was reviewed again from the delivered 123 baseline with the explicit goal of finding semantic counterexamples rather than confirming the optimization. This second pass included API aliasing, exact associative-container multiplicity, repeated getter side effects, ordering/RNG mapping, allocation-failure state, signal/thread publication, and external-library ownership. Where the first implementation was broader-risk than necessary, it was folded back into the not-yet-integrated patch rather than adding a later corrective patch. In particular, 127/128 were strengthened for alias and 32-bit size arithmetic before this bundle was revalidated.

## audit-124 — World lifecycle atomics

`World::m_stopEvent`, `m_ExitCode`, and the anti-freeze world-loop counter were shared between threads while using plain/`volatile` storage. `volatile` does not provide C++ thread synchronization.

- `m_stopEvent` is now `std::atomic_bool`; stop publication uses release stores and `IsStopped()` uses an acquire load.
- `StopNow()` stores the exit code before publishing the stop flag, preserving the intended exit-code-before-stop relationship.
- `m_ExitCode` is atomic, removing the read/write data race without changing its value semantics.
- The world-loop counter has exactly one writer (`WorldRunnable`) and a read-only anti-freeze observer. It uses relaxed atomic load/store rather than a locked read-modify-write, avoiding an unnecessary `fetch_add` penalty in every world loop.
- All direct source references were traced. `StopNow()` callers include signal/network/CLI/fatal-guid paths; `GetExitCode()` is only consumed by `Master` after the world thread has joined.
- Scheduled shutdown/cancel still use the same exit-code values and ordering relative to their existing hooks. The patch does not invent a new transactional winner policy for rare concurrent shutdown requests; it makes the existing shared state atomic and preserves sequential behavior.

## audit-125 — ThreadPool status atomic

`ThreadPool::m_status` is read and written by controller and worker threads, including shutdown and exception paths. It is now `std::atomic<Status>`.

- Existing status values and transitions are unchanged.
- Work queue type, worker count, condition-variable wakeups, completion promise, and clear modes are unchanged.
- The change only makes the existing cross-thread status contract defined under the C++ memory model.

## audit-126 — ThreadPool error-list synchronization

Multiple workers can enter exception handling concurrently and previously executed unsynchronized `std::vector<std::exception_ptr>::push_back` on `m_errors`.

- A dedicated mutex protects error insertion, `taskErrors()` snapshots, and the existing exception-front read.
- The mutex is touched only on error/reporting paths; normal task execution is unaffected.
- Error values, ordering as observed after serialized insertion, and the public `taskErrors()` return-by-value API are preserved.

## 127 — Direct hexEncodeByteArray buffer

The stream-based nibble formatter now builds one exact-size local output string and move-assigns it only after every input byte has been consumed.

- Empty input still produces an empty string.
- Byte/nibble order and uppercase `0-9A-F` output are unchanged.
- Existing callers were traced in account authentication and Discord authentication code; neither aliases the input buffer with `result`.
- The implementation nevertheless preserves the old broader aliasing property: `result` is not mutated until all bytes have been read.
- The `2 * arrayLen` size calculation is checked before multiplication, preventing a 32-bit `size_t` wrap into an undersized output buffer.
- Allocation/length failure occurs before `result` is modified, retaining the old strong output-state guarantee.
- 300,000 randomized old/new byte arrays matched exactly.

## 128 — Direct ByteArrayToHexStr buffer

The per-byte `sprintf` + stream path now writes directly into one exact-size result string.

- Forward and reverse byte order are preserved.
- Uppercase two-digit formatting per byte is preserved.
- Empty arrays remain empty.
- The `2 * arrayLen` size calculation is checked before multiplication, avoiding 32-bit `size_t` overflow.
- Warden and AutoUpdater call sites were traced.
- 300,000 randomized old/new arrays matched in both forward and reverse modes.

## 129 — LFT direct substring splitting

All three local `SplitPreserveEmpty` implementations now build fields from substring ranges instead of accumulating one character at a time.

- Leading, trailing, adjacent, and entirely empty fields are preserved.
- Embedded NUL bytes remain ordinary `std::string` data.
- Capacity is reserved from the exact delimiter count; the reserve input is derived from the already-owned string, not untrusted serialized length metadata.
- 300,000 randomized old/new cases matched, including embedded NULs and repeated delimiters.

## 130 — MMap filename value storage

Three MMap filename builders replace manual `new char[]` / `delete[]` ownership with a bounded stack suffix plus `std::string` path ownership.

- The suffix buffer is 64 bytes. This covers the full textual width of the actual argument types, including the tile form with one `uint32` and two signed `int32` values.
- `uint32` map/display IDs use `%u`; signed grid coordinates retain `%i`.
- File/log lifetime is tied to the local `std::string`, eliminating manual cleanup and the pre-existing filename leak on some game-object header failure paths.
- File naming for normal map/grid/display IDs is unchanged.

## 131 — Graveyard equal_range

`RemoveGraveYardLink` replaces separate `lower_bound(zoneId)` and `upper_bound(zoneId)` calls with one `equal_range(zoneId)`.

- The iterated half-open range is identical.
- Match filtering, traversal order, erase behavior, and DB behavior are unchanged.

## 132 — Reuse found erase iterators

Two associative-container hot paths reuse the iterator already returned by `find()` when erasing.

- Transport visibility: no container mutation occurs between `find` and `erase`; the same GUID entry is removed.
- Group invite removal: the debug assertion does not mutate the invite set; release/NDEBUG behavior remains remove-if-present.
- `m_invitees` is `std::set<Player*>` and `i_clientGUIDs` is `std::unordered_set<ObjectGuid>`; both are unique-key containers, so changing `erase(key)` to `erase(iterator)` cannot change "erase all equivalent keys" semantics.
- No insert-on-miss or overwrite semantics are introduced.

## 133 — PlayerBot gear-slot single lookup

The best-gear-per-slot map no longer performs `find()` followed by `operator[]`.

The old semantics were reconstructed explicitly:

- missing slot: insert the candidate;
- existing slot with equal or better score: keep it;
- existing slot with lower score: overwrite it.

`GearChoice` is a trivial local aggregate (`ItemPrototype const*`, `uint32`), so replacing the miss-path default-construction-plus-assignment performed by `operator[]` with direct aggregate construction has no hidden constructor/destructor side effects. The same key sequence is inserted and score ties still keep the first candidate.

The patch deliberately does not pre-insert before the score decision, so it does not reproduce the historical `try_emplace`/early-return audit failure.

## 134 — PlayerBot temporary-delay single lookup

`RefreshTempBot` now uses the iterator returned by `find()`.

- Missing accounts still remain missing; no `operator[]` insertion can occur.
- Existing delays below 1000 are still raised to exactly 1000; all others are unchanged.

## 135 — ZoneScript membership bucket reuse

Outdoor-PvP and zone membership checks cache the selected team bucket before `find/end`.

- Buckets are fixed array elements and are not mutated during these checks.
- `Player::GetTeamId()` is a pure inline read of `m_team`; caching the bucket removes a duplicate pure getter call rather than collapsing observable side effects.
- Lookup keys and membership result are unchanged.
- No pointer/reference is retained across mutation.

## 136 — LFT queue lookup and capacity reuse

Queue cleanup now reserves its removal snapshot from `m_queue.size()`, reuses a previously found queue iterator for erase, and caches repeated `GetPlayer` results.

- `m_queue.size()` is a direct upper bound for the removal vector.
- The found queue iterator is not crossed by a queue mutation before erase.
- Missing queue entries remain no-ops exactly as before.
- `LFTManager::GetPlayer()` is a lookup plus `IsInWorld()` check. In each changed site the cached pointer is consumed immediately, with no call in between that mutates the queue or player lifetime.
- Player lookup caching occurs before `SendQueueLeft`; there was no intervening mutation between the two historical lookups.
- Queue traversal/removal order and offer decisions are unchanged.

## 137 — Spell chain-target vector worksets

The three local chain-target candidate worksets move from `UnitList` (`std::list<Unit*>`) to `std::vector<Unit*>`.

Deep checks:

- `UnitListSearcher` is templated on the destination container and only calls `push_back`; both list and vector therefore receive the exact same visit sequence from `Cell::VisitAllObjects`.
- Only the temporary workset type changes; no reference/iterator to a container element survives a vector reallocation, erase, or sort.
- Stored values are `Unit*`; vector reallocation does not affect pointed-to Unit lifetime.
- `TargetDistanceOrderNear` compares squared distance through `WorldObject::GetDistanceOrder`. `std::list::sort` is stable, so the replacement uses `std::stable_sort`, preserving the pre-sort visitation order for equal-distance candidates.
- Initial random selection uses the same sorted prefix and the same index, so a given RNG result selects the same target.
- `erase` preserves the relative order of remaining vector elements; every selecting erase is followed by the same re-sort/reset-to-begin behavior as before.
- LOS-skip and player-only filtering do not mutate the workset while an iterator is retained.
- 250,000 randomized list-vs-vector differential traces matched, including deliberate distance ties, LOS skips, player filtering, erases, re-sorts, and identical RNG indices.

## 138 — Spell dispel vector workset

The temporary dispel candidate list becomes a reserved vector.

- Aura-map traversal and candidate insertion order are unchanged.
- Capacity uses `auras.size()`, an exact upper bound over the already-existing map.
- The special `priority_dispel` index is consumed on the first selection before any prior erase can shift it.
- Random selection uses the same `[0, size-1]` index mapping.
- Vector erase preserves the relative order of surviving candidates, matching list semantics for subsequent random indices.
- The selected `SpellAuraHolder*` is copied before vector erase. The source aura map is not mutated during candidate construction/selection; actual `RemoveAuraHolderDueToSpellByDispel` calls remain in the later success-log phase, after the workset is no longer used.
- 300,000 randomized stack/priority/index traces matched.

## 139 — Kel'Thuzad threat-target capacity

The temporary viable-player vector for Chains of Kel'Thuzad reserves `ThreatList::size()`.

- Threat-list size is a direct upper bound.
- Threat traversal, maintank removal, RNG selection, and target order are unchanged.

## 140 — Online guild-roster capacity

`BuildOnlineRosterPacket` gives its temporary online-member cache the same conservative half-member-count capacity heuristic already used by the full roster path.

- Member discovery and online filtering are unchanged.
- Stored member pointers still refer to the guild member map, which is not mutated during construction.
- Sorting and packet ordering are untouched.

## 141 — Group-loot recipient capacity

The temporary nearby-player vector reserves `group->GetMembersCount()`.

- Group member count is a direct upper bound for references visited.
- Distance filtering, money division, script callbacks, notification order, and packet contents are unchanged.

## 142 — Hostile HP target capacity

`FindLowestHpHostileUnit` reserves from the current threat-list size before collecting candidates.

- Threat-list size is a direct upper bound.
- Threat traversal, health/range filtering, `except` removal, and first-candidate return semantics are unchanged.

## audit-143 — MMap read-buffer ownership

`dtAlloc` buffers were leaked when tile/model reads failed, and the game-object path also leaked the buffer when `dtNavMesh::init(data, ..., DT_TILE_FREE_DATA)` failed.

Detour ownership was traced in the bundled `DetourNavMesh.cpp`:

- `dtNavMesh::init(data, ...)` validates the data, initializes mesh parameters, then delegates to `addTile`.
- `addTile` can return failure before assigning `tile->data` / `tile->flags`.
- `DT_TILE_FREE_DATA` therefore becomes Detour-owned only after successful tile insertion; failure leaves ownership with the caller.

The patch frees the buffer on failed `fread` and failed game-object navmesh initialization. Existing successful paths still transfer ownership to Detour, and the existing tile `addTile` failure path continues to free caller-owned data. No success-path double-free is introduced.
