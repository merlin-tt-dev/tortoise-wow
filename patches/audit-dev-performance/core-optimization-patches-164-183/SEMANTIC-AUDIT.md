# Semantic Audit — core optimization 164–183

## Audit boundary

Baseline is the complete user-supplied patch-163 branch archive. The source tree was compared directly against the previously reconstructed local patch-163 tree and matched byte-for-byte in all relevant source directories and root files. From this point forward only that complete archive was used as source of truth.

The proprietary game client is a hard semantic boundary. Valid client-observable behavior must remain unchanged, including packet/chat payloads, target/action ordering, visibility, DBC-derived values, and LFT selection behavior. Correctness patches may improve malformed-file or allocation-failure handling where the old behavior was unsafe, but valid-input behavior remains unchanged.

## 164 — buyback contiguous membership

`Player::_SaveInventory()` used `std::set<Item*>` only as membership for at most the 12 buyback slots (`69..80`). It was never iterated for ordering, erased, or exposed. The vector is populated at the same point and membership tests use pointer equality. `reserve(BUYBACK_SLOT_END - BUYBACK_SLOT_START)` is a compile-time semantic upper bound, not an inferred/untrusted size. Item state/queue mutation order is unchanged.

## 165 — direct Alterac Valley monster messages

Removes obsolete local `sprintf` buffers for fixed strings. Constant yells are passed directly to `MonsterYell`. The one formatted player-name message uses the existing `PMonsterSay()` helper, which formats and calls the same `MonsterSay()` path with default language 0 and null target. `MAX_PLAYER_NAME` is 12; the formatted message is at most 37 bytes, well below the former 200-byte buffer. The longest replaced constant message is 168 bytes, also below the old buffer. Therefore valid historical payload bytes are unchanged; the old unbounded `sprintf` primitive is removed.

## 166 — WmoLiquid copy-assignment ownership audit

The old assignment deleted current arrays before allocating/copying replacements. If the second allocation threw, assignment/copy construction could leak or leave a partially modified object. The new implementation allocates/copies into local `unique_ptr<T[]>` owners first, then replaces the object state only after all potentially throwing allocations succeed. Self-assignment remains a no-op. Successful-copy dimensions, corner, type, array element counts and bytes are identical. This intentionally strengthens exception safety without changing valid VMap data semantics or file format.

## 167 — WmoLiquid read ownership audit

The raw temporary `WmoLiquid*` is owned by `unique_ptr` until the entire read succeeds. `out` is still either a fully read object or null. Successful reads consume the same fields and payload in the same order and produce identical arrays. Truncated/error paths no longer leak partially built liquid storage; header-read failure also avoids pointless allocations that were discarded immediately. No VMap binary layout is changed.

## 168 — damage-shield contiguous membership

`TriggerDamageShields()` needs a processed-pointer membership set because `DealDamage()` can mutate the aura list and the algorithm deliberately restarts from `begin()`. The restart point, aura traversal, spell checks, packet send, damage order and mutations are unchanged. Only the processed-pointer workset becomes a vector. It is not iterated for ordering. Pointer identity is the membership contract. A randomized mutation/restart differential model covered remove/insert/replace/reorder cases.

## 169 — group-split contiguous unique allies

The old `std::set<Player*>` both deduplicated and imposed `std::less<Player*>` pointer order before damage splitting. Damage application order may be observable, so the replacement vector is explicitly sorted with `std::less<Player*>` and then deduplicated before iteration. An actual C++17 differential test compared `std::set<P*>` iteration to vector `sort(std::less<P*>) + unique` for 200,000 randomized cases. Order and uniqueness matched.

## 170 — target-icon contiguous group membership

`CreatureAI::ClearTargetIcon()` used `std::set<Group*>` only to detect whether a group had already been acted upon; `ClearTargetIcon()` was invoked immediately on first insertion rather than by later set iteration. The vector preserves exactly that first-encounter action order and pointer-identity membership while avoiding node allocations. No client target-icon payload/order change is introduced.

## 171 — remove dead visibility workset

The complete patch-163 tree was searched for `visibleNow`/`i_visibleNow`. The set was passed through Camera → notifier → Map → Player and its only semantic operation was `visibleNow.insert(target)`; there was no read, iteration, size query, erase, return, or subsequent consumer anywhere in the repository. The patch removes the dead allocation/work and corresponding parameter plumbing only. Visibility predicates, `BuildCreateUpdateBlockForPlayer`, visible-GUID mutation, broadcaster listener changes, out-of-range handling and packet data are untouched. All old-arity call sites were eliminated together.

## 172 — spell-area contiguous action sets

The old two `std::set<uint32>` objects provided uniqueness plus ascending spell-id execution order. The replacement vectors collect in the same requirement-scan order, then `sort + unique` before any remove/cast action. `binary_search` reproduces the old `spellsToCast.find(id)` membership condition. Remove actions remain ascending and happen before ascending cast actions. `HasAura()` and `IsFitToRequirements()` evaluation order/count during collection is unchanged. Differential models confirmed action-sequence equivalence.

## 173 — spell-duration DBC entry reuse

`CalculateDuration()` previously called `GetDuration()` and, when applicable, `GetMaxDuration()`, each performing `sSpellDurationStore.LookupEntry(DurationIndex)`. Both helper formulas were inlined around one cached `SpellDurationEntry const*`: missing entry → 0; `-1` remains `-1`; otherwise `abs(Duration[n])`. The DBC store is loaded during DBC initialization and has no runtime mutation/reload path between these calls. Aura-script, combo-point and spell-mod ordering is unchanged.

## 174 — LFT selected contiguous membership

`selected` contains at most five `ObjectGuid`s and is only queried for membership and size; its set iteration order was never observed. Vector membership preserves the selected set. Selection insertion still occurs only after a whole block fits. The final externally used `selectedRoles` remains `std::map<ObjectGuid,uint8>` and therefore retains ordered offer processing. Patch 182 separately preserves the old block-role map iteration order.

## 175 — allocation-free enemy-radius count

`GetEnemyCountInRadiusAround()` previously collected every accepted `Unit*` into a vector solely to return `targets.size()` as `uint8`. `UnitListSearcher<Check, Container>` was audited: both Player and Creature visits use the supplied container only through `push_back(Unit*)`; they never iterate or index it. The local counting sink therefore invokes the same check for every visited object in the same traversal order and increments once for every formerly pushed pointer. Final `size_t`→`uint8` conversion behavior is unchanged.

## 176 — whisper-target single insert

Immediately before insertion, `can_whisper()` proves the target GUID absent with `find()` and performs no intervening mutation of `targets_`. `targets_[target_guid] = time` therefore had only insert-on-miss behavior on this path. `emplace(target_guid, time)` is equivalent while avoiding a second hash/default construction. Existing-target early return, decay, capacity rejection and timestamp acquisition are unchanged.

## 177 — broadcast-text locale lookup reuse

`LoadBroadcastTextLocales()` already obtains a successful iterator to `m_BroadcastTextLocaleMap`. Reusing `bct->second` avoids rehashing the same key through `operator[]`. The complete intervening call graph was checked: `GetOrNewIndexForLocale()` mutates only `m_LocalForIndex`; `maleText/femaleText.resize()` mutate vectors inside the found `BroadcastText` value. Nothing inserts/erases/rehashes `m_BroadcastTextLocaleMap`, so the iterator/reference remains valid. Missing-entry logging/continue semantics are unchanged.

## 178 — quest start-item single insert

`m_QuestStartingItemsMap` is a `robin_hood::unordered_map<uint32,uint32>`. The old code performed `find`, inserted `(StartQuest, ItemId)` only on miss, and logged on duplicate. `emplace(...).second` preserves exactly that decision for primitive key/value arguments. No existing value is overwritten, and the same duplicate log executes.

## 179 — MMap data single insert

The initial shared-lock fast path remains unchanged. A new `MMapData` is built outside the lock as before. Under the exclusive `loadedMMaps_lock`, the old second `find` + insert/delete race-resolution is replaced by one `emplace`. If another thread inserted the map while this thread initialized its local mesh, insertion fails and the local `MMapData` is deleted exactly as before. The required shared→exclusive double-check semantics are preserved; only the second hash lookup is removed.

## 180 — guild GM-listener single insert

`m_GmListeners` is an unordered set. The old `find` followed by `insert` returned true only for a missing GUID. `insert(...).second` is exactly that contract. `GetObjectGuid()` is a pure value accessor; listener membership and return value are unchanged.

## 181 — GM survey fixed membership

The packet format admits at most ten sub-survey iterations. The old `std::set<uint32>` was used only to suppress duplicate IDs; its ordering was never consumed. The fixed `std::array<uint32,10>` plus used-prefix count performs the same duplicate test with no heap allocation. Zero still terminates before rank/comment reads exactly as before; duplicate nonzero entries still consume rank/comment and skip only the DB insert. The count cannot exceed 10 by construction. Differential models verified accepted-ID sequences.

## 182 — LFT block-role contiguous workset

The old `std::map<ObjectGuid,uint8> blockRoles` accumulated role choices and later iterated by ascending `ObjectGuid`. `block` is derived from `GetQueueOrder()`, which contains each unique `QueueMap` iterator once; therefore a block cannot contain duplicate queue keys. Role selection is still performed in queue/block order before sorting. The vector is sorted by `ObjectGuid` only after the block is fully validated, reproducing the former map iteration order before updating `selected`/`selectedRoles`. No RNG is involved. Differential container models were run jointly with patch 174.

## 183 — guild account hash membership

`GetAccountsNumber()` observes only the number of unique `uint32 accountId`s; the temporary set is never iterated for order or exposed. `unordered_set<uint32>` has the same equality contract for these IDs and changes the uniqueness pass from tree O(n log n) to expected hash O(n). Reserving `members.size()` uses an already-materialized guild-member container size, not untrusted serialized data. The cached `m_accountsNumber` result and invalidation mechanism are unchanged. This is a CPU/allocation tradeoff rather than a client-visible change.

## Explicitly preserved legacy behavior

- No packet structure, opcode, GUID serialization, UpdateData ordering, or spell target RNG mapping is changed.
- Pointer order in group damage splitting is preserved rather than replaced with encounter order.
- Visibility decisions and visible GUID state changes are untouched; only a provably write-only set is removed.
- LFT offer/member processing remains ordered through the existing `selectedRoles` map; block role insertion order is reconstructed explicitly.
- MMap shared/exclusive double-check synchronization is retained.
- No untrusted serialized length is used as a new reserve bound.
