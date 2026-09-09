# Semantic audit notes 083-102

## Audit/correctness patches

### 083 - PlayerBroadcaster listener completion
Restores the old completion guarantee for listener removal/clearing while retaining the performance benefit of not holding the listener mutex during network sends. Snapshot batches are tracked explicitly.

### 084 - Movement broadcaster removal completion
Adds per-broadcaster processing lifecycle synchronization so logout cannot race a snapshotted worker still executing `ProcessQueue()`.

### 085 - ChannelBroadcaster producer serialization
`moodycamel::ReaderWriterQueue` is SPSC. `AsyncSay()` has multiple producer contexts, including normal chat/channel paths and the asynchronous antispam path. Producer-side enqueue is serialized through the existing state mutex while consumer processing remains separate.

### 086 - Exact MovementBroadcaster RemovePlayer completion
Moves the processing stop barrier to `RemovePlayer()` itself so its return again means no worker can subsequently begin or continue processing that broadcaster, matching the pre-snapshot synchronization boundary.

### 087 - Threat report string/name semantics
Fixes the Linux compile error introduced by the earlier threat optimization and removes a hidden `const char* + char` pointer-arithmetic bug. Name comparison is allocation-light while `BuildChatPacket` receives the expected pointer/string contract at this point in the series.

## Performance/modernization patches

### 088 - Contiguous runtime grid snapshots
Converts additional short-lived grid result lists to vectors while preserving visitor order and selection semantics.

### 089 - Map instance workload reuse
Avoids repeatedly copying the same per-tick vector of instance update callables into the dedicated thread pool during repeated processing rounds.

### 090 - Spell power-chain range lookups
Replaces repeated complete multimap scans with key-local `equal_range()` traversal while retaining the direct `mSpellGroupSpell` ordering/semantics. The transformation was previously compared old-vs-new over 100,000 randomized cases.

### 091 - Destroy packet buffer reuse
Reuses the same `WorldPacket` object for repeated destroy notifications after synchronous `SendPacket()` calls, avoiding per-GUID packet-object allocation while retaining send order/timing.

### 092 - LFT queue iterator order
Carries stable `QueueMap::const_iterator` values through sorting and matching to avoid repeated map lookups during comparator and nested matching loops. Queue mutation is absent during this selection phase.

### 093 - MMap loader RAII
Uses RAII for file handles, filename storage, Detour buffers and navmesh ownership/error paths. Detour buffers are released to Detour only after successful ownership transfer (`DT_TILE_FREE_DATA`).

### 094 - MMap model query locking
Protects per-MMap query container access with the existing per-MMap synchronization and removes the final redundant lookup.

### 095 - Async packet worker wakeup
Replaces an unsynchronized boolean plus 20 ms polling with mutex/condition-variable wakeup while deliberately preserving the prior non-barrier semantics for an already-running packet-processing iteration.

### 096 - Data path/GridMap filenames
Returns the immutable global data path by const reference after call-site lifetime/mutation audit and removes remaining manual GridMap filename heap buffers.

### 097 - Database escape buffer
Escapes directly into appropriately sized `std::string` storage and resizes from the database API's returned escaped length, avoiding the extra raw heap buffer/copy.

### 098 - WorldSession string accessors
`GetUsername()` / `GetEmail()` return const references after call-site audit showed no required snapshot/mutation contract.

### 099 - ScriptedAI friendly vector snapshots
Uses contiguous snapshots for friendly CC/missing-buff searches while retaining visit order and random-index semantics at callers.

### 100 - AreaAura vector snapshot
AreaAura target collection is append-then-iterate only, so vector replaces list without iterator-stability or ordering changes.

### 101 - Associative insertion single lookups
Collapses provable `find()` + insertion paths. Assert-protected duplicate cases preserve both debug assertions and release-build overwrite behavior rather than naively changing semantics to insert-only.

### 102 - WorldText contiguous packet cache
Stores localized broadcast packets as `WorldPacket` values in vectors instead of individually heap-allocating packet objects; packets are only sent after cache construction, so pointer stability is not required.
