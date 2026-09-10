# Semantic audit summary — Core Optimization 144-163

## Audit policy

The 144-163 block was reviewed against the exact local 143 tree derived from the user's complete archive. The review was adversarial: each optimization had to survive checks for call-site behavior, insert-on-miss/overwrite semantics, reference and iterator lifetime, ordering/RNG mapping, move-after-use, exception/failure behavior, reentrancy, immutable-store lifetime, and proprietary-client observable behavior.

The proprietary client is a hard boundary. None of these changes touches packet layouts or serialization. For server-side algorithms that can influence client-visible gameplay, ordering and selection behavior were explicitly preserved.

## 144 — Transmog membership without per-call containers

`ObjectMgr::IsItemTypeTransmoggable` and `IsItemSubClassTransmoggable` previously constructed local `std::vector<uint32>` membership tables on every call and then linearly searched them. They now use switch membership tests over exactly the same enum constants.

- Every old accepted enum value is represented exactly once in the new switch; excluded/commented legacy values remain excluded.
- The return value is purely boolean; vector traversal order was never observable.
- No state is read or mutated during the membership comparison.
- The change deliberately removes transient allocator failure as a failure mode for these pure membership checks. This changes only out-of-memory behavior, not valid-input gameplay/client semantics.

## 145 — Transmog nested-container lookup reuse

`AddPossibleTransmog` previously repeated the four-level `operator[]` chain for `begin`, `end`, and the push path.

- The new local `TransmogContainer&` performs the same insert-on-miss materialization before the search.
- Parent containers are `std::map` layers, so the retained reference is not invalidated by unrelated map rebalancing; no parent-map mutation occurs after the reference is obtained anyway.
- The only later mutation is `push_back` on the referenced vector itself; reallocation changes the vector's element storage, not the vector object/reference.
- Duplicate detection and first-insertion order are unchanged.
- The historical early-return/`try_emplace` trap is not introduced: no insertion is moved across a conditional return.

## 146 — Unit model-data DBC lookup reuse

`Unit::UpdateModelData` looked up the same `CreatureModelDataEntry` twice. The first pointer is now reused.

- `DBCStorage::LookupEntry` is a pure bounds check plus indexed pointer read.
- The relevant DBC store is immutable during normal runtime; no store mutation occurs between the historical two reads.
- Intervening `SetFloatValue` calls mutate Unit fields, not the DBC store or the cached entry.
- Null and fallback collision-height behavior are unchanged.

## 147 — Spell range DBC lookup reuse

`EffectTransmitted` now looks up `SpellRangeEntry` once before computing min/max ranges.

- Both old lookups used the same `rangeIndex` with no mutation in between.
- `LookupEntry` is a pure immutable DBC lookup.
- `GetSpellMinRange` and `GetSpellMaxRange` receive the same pointer they would have independently received.
- RNG calls occur after the lookup work in the same order and count; random distance/angle mapping is unchanged.

## 148 — Reuse found erase iterators

Three erase paths reuse an iterator already returned by `find()` rather than hashing/searching the key again.

- Transmog template map is a unique-key robin-hood unordered map; no mutation occurs between `find` and erase.
- VMap `InstanceTreeMap` is a unique-key `std::unordered_map`.
- `StaticMapTree::~StaticMapTree()` only deletes its internal tree-value array; it does not call back into `VMapManager2` or mutate `iInstanceMapTrees`, so deleting the pointed-to object does not invalidate the map iterator.
- Iterator erase removes exactly the same unique key as `erase(key)`.

## 149 — Valentine spell maps in static const storage

Two read-only `std::map<uint32, std::vector<uint32>>` tables were reconstructed for every relevant spell execution. They are now function-local `static const` objects.

- Key/value contents are byte-for-byte equivalent at the value level.
- All uses are const lookup/index reads; there are no mutations.
- C++11+ local-static initialization is thread-safe, and concurrent const lookup is safe after initialization.
- No RNG occurs during table construction; moving construction to first use does not consume or reorder random state.
- Spell choice, aura checks, talked-to sets, and cast ordering are unchanged.
- Per-call allocation failure is intentionally removed after first initialization; normal gameplay semantics are unchanged.

## 150 — Capture-point GameObjectInfo lookup reuse

`OPvPCapturePoint::SetCapturePointData` performed a second `GetGameObjectInfo(entry)` immediately after already proving the first result non-null and of capture-point type.

- No call or mutation exists between the first successful check and the redundant second lookup.
- The removed second null-check was unreachable in defined single-threaded ObjectMgr use once the first check passed.
- The original first pointer (`goinfo`) is already used later for capture-point fields, so lifetime assumptions are unchanged.
- Error messages for invalid/missing entries still take the original first error path.

## 151 — Saved-variable single lookup

`_InsertVariable` previously executed `m_SavedVariables[index] = tmp` and then `m_SavedVariables[index]` again for the return.

- The new reference is obtained through the same first `operator[]`, so insert-on-miss/default-construction behavior is preserved.
- Assignment still happens after materialization and overwrites an existing value exactly as before.
- Assignment to the mapped object cannot rehash the unordered map.
- The returned reference denotes the same mapped object.

## 152 — Move fingerprint account name

`World::AddFingerprint` accepts `accountName` by value, making the function the owner of its local string copy. It now moves that local into the target `unordered_set`.

- The parameter is not read after insertion.
- Duplicate-key behavior is unchanged; any state of the local parameter after `insert` is unobservable because it is destroyed on return.
- Fingerprint key, account string value, hashing/equality, and set contents are unchanged.

## 153 — Move CustomMerchant event batch into lambda

Each delayed custom-merchant batch was first copied into a local vector and then copied again into the lambda closure. The closure now move-captures that freshly-created batch.

- `batch` is not used after event registration in the loop iteration.
- `EventProcessor::AddLambdaEventAtOffset` already moves the rvalue lambda into `LambdaBasicEvent`; no new reference lifetime is introduced.
- The closure remains copyable because its captured vector remains copyable; event behavior is unchanged.
- Item order inside every batch, event offsets, player checks, and network update order are unchanged.

## 154 — Keyed lookup for more-powerful active auras

`HasMorePowerfulSpellActive` previously nested every active aura-holder key against every spell ID returned by `ListMorePowerfulSpells`. It now performs `multimap::find(spellId)` for each stronger spell ID.

- `m_spellAuraHolders` is a `std::multimap<uint32, SpellAuraHolder*>` keyed by spell ID.
- The function returns only a boolean and never selects/returns a particular holder, so changing search traversal order cannot affect a selected aura.
- Neither comparison path mutates holders or the multimap.
- Duplicate stronger IDs and multiple holders for one ID remain equivalent to “at least one matching key exists.”
- `ListMorePowerfulSpells` generation and its order are unchanged.
- 300,000 randomized old-vs-new key/membership cases were checked during development.

## 155 — LFG processed-member capacity

`Group::CalculateLFGRoles` reserves `processed` from `GetMembersCount()`.

- `GetMembersCount()` is exactly `m_memberSlots.size()`.
- `processed.push_back` occurs at most once for each group-member GUID allocated a role, so member count is a hard upper bound.
- Reserve does not alter member traversal, role priority checks, or insertion order.
- No iterator/reference into `processed` survives an insertion.

## 156 — Warden return-value elision

Two `return std::move(...)` expressions returning by value were removed so normal return-value optimization/move elision can apply.

- `Warden::SelectScans` returns the prvalue from `GetRandomScans` directly.
- `WardenScanMgr::GetRandomScans` returns its local vector normally; C++ move-on-return/NRVO rules preserve value semantics.
- Scan shuffle order, priority prefix, request/reply size truncation, and client scan ordering are untouched.
- The unrelated `SplitWord(std::string& in)` style of `return std::move(in)` was explicitly not changed because there the move-from side effect on caller-owned state is semantic.

## 157 — Contiguous LFG role-priority storage

`LFGPlayerQueueInfo::rolePriority` changes from `std::list<pair<...>>` to `std::vector<pair<...>>`.

- Full source search found only `CalculateRoles` append and `GetRolePriority` forward iteration; no iterators, references, or element pointers escape.
- Append order is unchanged: tank, healer, DPS.
- `GetRolePriority` still returns the first matching role.
- Repeated `CalculateRoles` calls historically append another three entries without clearing. The vector preserves that legacy behavior; the reserve is only performed while empty, so earlier entries remain first and observable results stay unchanged.
- No list-specific splice/erase/stability behavior was used.
- This struct is internal queue state and is not packet-serialized by object layout; wire ABI is unaffected.

## 158 — Contiguous removed-proc-spell workset

`Unit::HandleTriggers` changes its local removed-spell workset from `std::list<RemovedSpellData>` to `std::vector<RemovedSpellData>`.

The comparator contract is intentionally unusual and was preserved exactly:

- `operator<` compares only `spellId`.
- `operator==` compares both `spellId` and `Unit*`.
- Old `list::sort()` is stable, so the replacement uses `std::stable_sort`, not `std::sort`.
- Old `list::unique()` removes only adjacent exact-equality duplicates; vector `std::unique` + erase performs the same adjacent equality operation after the stable sort.
- No iterator/reference into the workset escapes or survives sort/erase.
- Removal iteration order therefore remains identical, including order among equal spell IDs for different units.
- 500,000 randomized `(spellId, unit-id)` sequences matched old-vs-new sorted/deduplicated output exactly.

## 159 — Reuse DBC row Record views

`AutoProduceData` and `AutoProduceStrings` now obtain one `DBCFileLoader::Record` view per row and reuse it across that row's fields.

- `Record` contains only the row byte pointer plus a reference to the loader.
- `getFloat/getUInt/getUInt8/getString` are read-only operations on loader data.
- The destination `dataTable` is separate from the source DBC byte buffer, so writes performed during production cannot invalidate/change the cached source row.
- Field traversal, endian conversion, output offsets, index table assignment, and string-pool offsets are unchanged.

## 160 — Reuse WorldObject map pointer in summon counters

Several summon limit/count helpers now retain the result of `FindMap()` rather than reading it repeatedly.

- `WorldObject::FindMap()` is an inline pure accessor: `return m_currMap`.
- `Map::Get/Set/Increment/DecrementSummon*` only operate on the corresponding counter/limit associative containers and do not call back into the WorldObject or change its map association.
- Therefore the cached pointer is valid across the local counter operation under the same defined threading assumptions as the original code.
- Missing-map logging/fallbacks and summon-alert clearing behavior are unchanged.

## 161 — Single-pass `mangos_strdup`

The helper previously executed `strlen`, allocated `length+1`, then `strcpy`, which scans for the terminator a second time. It now computes the same byte count once and copies exactly that count with `memcpy`.

- Source is a C string; copied bytes include the terminating NUL.
- Destination allocation size and `new[]` ownership contract are unchanged.
- The only current runtime caller stores the result in the legacy creature-aura field and later releases it with `delete[]`, matching the allocation family.
- Null-source behavior remains outside the helper's contract and would be invalid for both implementations.

## audit-162 — DBC field-offset reload ownership

`DBCFileLoader::Load` overwrote `fieldsOffset` on a later successful load without freeing the prior array.

The repair deliberately preserves failure ordering:

- A new raw array is allocated and completely populated first.
- If that allocation throws, the previous `fieldsOffset` member remains untouched, matching the old pre-assignment state.
- Only after successful construction is the old member array deleted and the new pointer installed.
- Existing behavior on earlier file/header failures remains unchanged because replacement is not attempted until the same point as before.
- Subsequent data-buffer allocation/read semantics are otherwise untouched; this patch does not broaden into a loader-state redesign.

## audit-163 — Bounded legacy thread-name formatting

Three fixed 128-byte buffers used unbounded `sprintf` for ThreadPool, SQL callback pool, and SQL delay thread names. They now use `snprintf(..., sizeof(buffer), ...)`.

- For every name whose formatted output fit the historical buffer, bytes are identical.
- Format strings, `%s/%d` argument interpretation, and call order to `thread_name` are unchanged.
- Overlong names previously caused a buffer overflow and undefined behavior; they are now safely NUL-terminated/truncated. There is no defined old overflow behavior to preserve.
- 200,000 randomized fitting-name cases matched `sprintf` and `snprintf` byte-for-byte; an additional long-name test confirmed termination at the buffer boundary.
