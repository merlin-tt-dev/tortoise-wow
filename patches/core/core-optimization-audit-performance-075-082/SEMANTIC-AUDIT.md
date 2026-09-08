# Semantic audit notes — core optimization series

## Confirmed regressions corrected by 075–080

### 075 — Weather / Terrain deferred construction
- Earlier Weather `try_emplace` evaluated `GetWeatherChances(zoneId)` even on cache hits.
- Earlier Terrain ownership conversion inserted an empty `unique_ptr` before constructing `TerrainInfo`; with enabled C++ exceptions, a caught construction failure could leave a null cache entry.
- `075` restores deferred construction and strong insertion semantics.

### 076 — Threat report compile semantics
Two latent compile errors from `059` were repaired:
- `hatedPlayer` was referenced before declaration in the tank-mode branch.
- `"#" + tankModePrefix` became invalid after `tankModePrefix` changed from `std::string` to a character array.
The historical target-selection order is preserved.

### 077 — ByteBuffer generic deserialization
- `std::list` has no `reserve()`; the earlier generic change could fail when that template was instantiated.
- Blind `vector::reserve(vsize)` could allocate from an untrusted/unchecked serialized length before ordinary element extraction detected buffer exhaustion.
- Move insertion remains retained.

### 078 — MotionMaster empty recovery path
`reserve(size() - 1)` underflowed when `size()==0`, defeating an existing recovery path. The reserve calculation is now safe for the empty state.

### 079 — Account warning malformed-data behavior
Replacing `substr(5, ...)` with `erase(0, 5)` changed behavior for malformed strings shorter than five characters. The old exception behavior is retained while still materializing the DB string once.

### 080 — Petition signature snapshot lifetime
Changing the petition-signature list from a copy to `const&` was unsafe because `Guild::AddMember()` can erase the current signature while `Guild::Create()` is iterating it. `080` uses a contiguous pointer snapshot, preserving the original snapshot semantics without `std::list` node copies.

## Double-lookup audit
The prior single-lookup changes involving `find`, `count`, `operator[]`, cached ranges, `try_emplace`, and iterator reuse were reviewed for insertion, overwrite, lifetime, and ordering semantics. The remaining reviewed changes were semantically valid.

A deliberate MMap double lookup around a lock remains unchanged because it implements double-check synchronization rather than redundant lookup work.

## Differential checks previously run
- Antispam character filter: all 256 byte values across 16 masks matched the prior regex behavior.
- Antispam formatting: crafted cases plus 500,000 randomized strings matched the prior regex pipeline.
- Warden MD5 EVP migration: boundary cases plus 20,000 randomized buffers were byte-identical to the previous MD5 API.
