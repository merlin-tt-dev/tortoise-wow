# Semantic audit — core optimization 184–203

## Audit standard

The old Core is treated as a behavioral contract. A proprietary game client is attached, so server-internal cleanup is accepted only where packet/wire data, target/order selection, RNG mapping, timing/completion, and gameplay-visible side effects remain unchanged.

### 184 — remove dead BG AFK state
Removed `BGData::bgAfkReporter`, `bgAfkReportedCount`, and `bgAfkReportedTimer`.

Whole-tree search on the exact patch-163 baseline found no read/write of these members outside their declarations. `BGData` is not raw-copied or serialized by `sizeof`; battleground persistence serializes named fields individually. Result: object size/allocation overhead drops without runtime behavior change.

### 185 — MasterPlayer whisper hash membership
`m_allowedWhispers` is used only for insert, clear, and membership (`AcceptsWhispersFrom`). No iteration, `begin`, ordering, iterator escape, or RNG indexing exists. Hash storage preserves the complete observed contract.

### 186 — world locked-character hash membership
`m_lockedCharacterGuids` is only insert/erase/find and every access is under `m_autoPDumpMutex`. It is distinct from `m_autoPDumpPendingGuids`, whose ordered iteration determines dump processing order and therefore intentionally remains `std::set`.

### 187 — disabled content hash membership
Both disabled-spell and disabled-map-loot containers are clear/insert/count only. Their order is never exposed. Reload/read synchronization is not changed; this patch only changes membership complexity/storage.

### 188 — database identifier hash membership
The ten identifier containers are loaded as unique IDs and queried only through `IsExisting*` membership accessors. No iteration or order-dependent consumer was found. No references/iterators escape.

### 189 — exclusive-visible contiguous storage
The old list supported `push_back`, linear membership and `list::remove(guid)`. No iterators/pointers to elements escape. `vector + erase(remove())` removes all equal GUIDs and preserves survivor order, exactly matching `list::remove`. Randomized differential test: 100,000 sequences PASS.

### 190 — existing spell hash membership
`mExistingSpellsSet` is clear/insert/find only; no iteration or ordering. The hot `IsExistingSpellId()` check retains the same boolean result.

### 191 — channel-ban hash membership
Channel bans are insert/erase/find only. Ban order is neither enumerated nor serialized. Player-list ordering remains untouched.

### 192 — GameObject GUID membership split
Only `m_SkillupSet` and `m_allowedLooters` were changed to `ObjectGuidSet` (unordered membership). `m_UniqueUsers` deliberately remains ordered because ritual selection indexes its iterator with RNG; changing that container would remap an RNG result to a different target.

### 193 — ObjectMgr runtime hash membership
Tavern-area-trigger and quest-gameobject sets are loader-filled and queried only by membership. No iteration/order consumer exists.

### 194 — reserved-name hash membership
Reserved names are normalized to lowercase, inserted, and queried by exact membership only. No enumeration or ordering is observed.

### 195 — obtained-item hash membership
The temporary obtained-item collection only deduplicates IDs and answers membership while loading item prototypes. Neither traversal order nor insertion order is consumed. Function signature and local storage were changed together; no external caller exists.

### 196 — DBC string pool contiguous storage
`DBCStorage::m_stringPoolList` only receives pools via `push_back` and releases every pointer in insertion order during `Clear()`. No iterator/reference escapes. `std::vector<char*>` preserves pointer values and insertion/deletion order while avoiding one list-node allocation per loaded locale. Reallocation moves only raw pointer values, never the pointed-to character arrays. `<vector>` is included explicitly rather than relying on transitive headers.

Failure-path check: `AutoProduceStrings` can return null for format mismatch; `delete[] nullptr` remains valid. Allocation exceptions remain exceptions; no ownership is released prematurely.

### 197 — prohibit-spell contiguous storage
`ProhibitSpellInfo` is a trivial `{SpellSchoolMask,uint32}` value. The container is private to `Unit`, receives only `push_back`, gets timers updated in place, is linearly queried, and removes expired entries after the timer pass. No callbacks or escaping iterators exist.

The historical erase loop restarted at `begin()` after every zero-timer removal; its net result is “remove every zero entry, preserving all surviving entries in original order.” `vector::erase(remove_if())` has exactly that survivor order. Randomized differential test over timer/update/removal states: 150,000 cases PASS.

### 198 — bounded fixed date formatting
Three fixed-format date/time buffers change `sprintf` to `snprintf(buffer,sizeof(buffer),...)`. For valid `tm` ranges these strings are far below their 128-byte buffers. A C++17 differential test over 250,000 randomized date/time/value cases confirmed byte-identical output. Only previously undefined overflow behavior would differ.

### 199 — bounded pdump filename formatting
`Char%u-%u.bak` in a 64-byte buffer and `Char%u-` in a 32-byte buffer change to bounded formatting. Two decimal `uint32` values cannot approach 64 bytes; one cannot approach 32 bytes. Differential testing is included with patch 198 and is byte-identical on valid values.

### 200 — remove dead ticket reload set
`_reloadTicketsSet` is referenced only inside a block-commented obsolete reload implementation and has no active-code consumer. Removing it reduces `TicketMgr` state without active API/runtime change. The commented code would require restoration work before it could be re-enabled regardless.

### 201 — VMap map-spawn single lookup + ownership guard
The old miss path performs `find(mapID)` and then `operator[](mapID)` (second tree lookup) after `new MapSpawns`. The new path uses `lower_bound` to determine both membership and insertion position, then `emplace_hint`.

A local `unique_ptr` owns the newly allocated `MapSpawns` until map insertion succeeds. If tree-node allocation/comparison throws, the object is freed rather than leaked; after successful insertion ownership is released to the unchanged raw-pointer `MapData`, preserving the existing later cleanup contract. Existing-hit behavior and map key ordering are unchanged. `<memory>` is explicit. Randomized key-sequence differential model: 100,000 cases PASS.

### 202 — Discord command-link single lookup
Registration still invokes `_commandHandler->add_command(...)` before checking/linking, preserving its historical side-effect order. `find + operator[]` becomes `try_emplace(command,this)`. On duplicate, the existing handler remains unchanged and the same error path runs; on miss, the same key/value is inserted without a second lookup. No iterator escapes. Randomized duplicate/first-wins model: 100,000 cases PASS.

### 203 — spell-script condition hash membership
`conditions` is a local loader-validation workset. It is only populated from condition IDs and queried with `find` while validating `spell_script_target`; it is never iterated or serialized. Therefore changing `set` to `unordered_set` cannot change diagnostic row processing order or accepted/rejected target rows.

## Explicitly rejected nearby changes

- `m_autoPDumpPendingGuids`: ordering determines pdump processing order.
- `m_fingerprintAutoban`: iteration order determines ban-operation order.
- `m_temporaryAtWarFactions`: iteration sends faction-at-war updates; order is observable.
- `GameObject::m_UniqueUsers`: RNG indexes set iteration and therefore target identity.
- `World::m_disconnectedSessions`: iteration runs session updates/destructors; order can carry side effects.
- persistent grid `CellGuidSet`: iteration controls object spawn/load order and can become client-visible.
- `ViewPoint::m_cameras`: callbacks may detach cameras; list iterator stability is part of the reentrancy contract.
- loot `m_playersLooting`: notification callbacks may mutate membership during traversal; set iterator stability and ordering are not mechanically replaceable.
