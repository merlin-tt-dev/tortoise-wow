# Semantic audit — corrected 224–243

## Audit standard

Each change was reviewed for full usage/call-site behavior as applicable, including iteration/order, insertion and overwrite behavior, pointer/iterator lifetime, reloadability, race/ownership surface, failure/allocation timing, RNG-to-result mapping and proprietary-client-observable behavior. Ordered structures were retained whenever equivalence was not provable.

## Patch notes

### 224 — PlayerDump GUID remap hash lookups
Local GUID remap maps are used through exact lookup/insertion only; no iteration-order consumer was found. First-insertion behavior is retained.

### 225 — Quest mail-template hash membership
Loader-local membership/value lookup only. No order consumer was found.

### 226 — Pet learn-cache hash lookup
Local exact-key cache. No iteration-order consumer was found.

### 227 — VMap model-node hash lookup
The hash maps model IDs to their already-assigned build-order indices. Serialization/index identity remains the original index; hash iteration is never used to assign or emit the index.

### 228 — GameEvent pool-link hash lookups
Only exact creature/gameobject event-link maps are converted. `pool2event` remains ordered because its traversal/diagnostic ordering was not treated as disposable.

### 229 — Talent inspect hash lookups
Lookup indices are hashed, while the distinct ordered `TalentBitSize` structure remains unchanged because its order defines client-visible talent bit positions.

### 230 — Spell category outer hash lookup
Only the outer keyed map changes. The inner `std::set` stays ordered.

### 231 — Pet family outer hash lookup
Only the outer keyed map changes. The inner `std::set` remains ordered so the existing AddSpell traversal/side-effect order is retained.

### 232 — Talent spell-position hash lookup
Exact keyed lookup only; no iteration-order consumer was found.

### 233 — Taxi path hash lookups
Nested exact source/destination lookups are hashed. The remaining inner traversal is only the order-independent predicate “does any non-spell path exist?”. Source semantics still come from the explicit numeric source loop, and a selected path still comes from the exact `(source,destination)` key. Differential-tested.

### 234 — WMO area tuple hash lookup
Exact tuple-key lookup only. Direct required standard headers are included.

### 235 — RBAC immutable hash lookups
The audited tree builds these structures during World startup; no runtime reload caller was found. Consumers are keyed lookup/mask operations, not order-sensitive iteration.

### 236 — Opcode dense dispatch table
Normal opcode IDs `< NUM_MSG_TYPES` use dense optional storage; extended IDs retain the ordered-map fallback. Duplicate registration keeps overwrite semantics. Missing/gap and out-of-range behavior remains the same. Only server-side dispatch lookup storage changes; packet/wire representation and opcode values do not.

### 237 — Saved talent-tree fixed array
The structure has exactly three slots. Dynamic storage is replaced with a fixed three-element array without changing slot ordering.

### 238 — Guild member index hash lookup
Uses exact insert/erase/find only and is protected by the pre-existing `shared_mutex`; no iteration consumer was found. The change does not weaken the existing synchronization boundary.

### 239 — Scripted-event single tree search
The ordered tree remains ordered. `find` followed by insertion becomes one `lower_bound` plus `emplace_hint`; duplicate-path constructor-argument evaluation behavior was explicitly tested.

### 240 — Scripted-event data lookup reuse
Caches the existing `operator[]` result. Insert-on-miss semantics are retained; the patch removes a repeated lookup only.

### 241 — Movement slow-map hash aggregation
The temporary aggregation container becomes hashed, but selection no longer depends on container iteration. The old ordered-map result rule was reconstructed explicitly: highest packet count wins; for equal non-zero counts the smallest instance ID wins; all-zero returns zero. Existing `uint32` wrap arithmetic was retained and differential-tested.

### 242–243 — Valentine fixed spell pairs
Only two-element vectors inside still-ordered faction maps become fixed arrays. Outer faction ordering and pair ordering remain unchanged.

## Allocation/failure timing

Some accepted fixed/dense/hash storage changes alter allocation timing or remove small allocations. These are documented as performance-side failure-boundary changes where the old allocation existed solely to represent fixed or lookup-only data. No change was accepted merely because successful return values matched; reload, ordering and mutation behavior were audited separately.

## Proprietary-client boundary

No patch intentionally changes packet field order, widths, GUID packing, opcode values, packet presence, talent bit layout, RNG-to-target mapping, or wire serialization. Client-adjacent structures whose observable ordering could not be fully proven were left ordered.
