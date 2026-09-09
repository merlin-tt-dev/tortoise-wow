# Semantic audit summary

## 105 - CLI command value ownership
Manual command `new[]/delete[]` ownership replaced with `std::string`; command and `std::any` payloads are moved at enqueue. All producers, queue lifetime, consumer and shutdown drain were traced.

## 106 - WorldObject vector snapshot
Container-neutral WorldObject searcher retains `std::list` as default; only a snapshot consumer moves to `std::vector`. Grid visitation and script execution order are unchanged.

## 107 - Tokenizer value buffer
Tokenizer owns its mutable split buffer as `std::string`, allowing SSO and RAII. Copy/move remain disabled because stored pointers refer into its own buffer. Old/new tokenization matched over 200,000 randomized inputs including embedded NULs.

## 108 - Prepared statement direct index
Prepared-statement ID -> SQL lookup changes from an O(n) unordered-map scan under mutex to a direct ID index. Pointer/reference stability across unordered_map rehash and insertion rollback on allocation failure were checked.

## 109 - Deprecated format single allocation
Deprecated formatting helper removes the intermediate heap char buffer and formats directly into the result string. Old/new formatted output matched over 200,000 randomized cases.

## audit-110 - LockedQueue empty synchronization
The two external `empty_unsafe()` reads on SQL queues are replaced by synchronized `empty()` checks. No worker scheduling or shutdown semantics are altered.

## audit-111 - SqlDelay running atomic
`volatile bool` worker state becomes `std::atomic_bool`; `volatile` was not thread synchronization. Polling/scheduling behavior is otherwise unchanged.

## audit-112 - Map update state atomic
`asyncMapUpdating` becomes atomic to close a read/write data race while preserving teleport queueing and the existing completion boundary.

## 113 - GridSearch vector overloads
Existing container-neutral searchers are exposed through generic helpers and selected snapshot-only consumers use vectors. Checks, traversal and consumer order remain unchanged.

## 114 - Aura script snapshot capacity
Two pre-population reserve calls are replaced with one combined capacity calculation. Aura collection and callback order are unchanged.

## audit-115 - Denied packet ownership
AntiFlood `Denied` consumed a dequeued packet and then broke without delete or requeue. The patch deletes that uniquely-owned packet before break; requeue rotation semantics are untouched.

## 116 - Associative state single lookups
Insert-on-miss semantics are deliberately preserved while repeated `operator[]` lookups reuse a single state reference.

## 117 - SQL query string value ownership
SQL request strings move from manual C-string ownership to `std::string`; SqlQueryHolder uses `std::optional<std::string>` so an empty SQL string remains distinct from the historical 'result ownership transferred' marker. 200,000 randomized holder state-machine traces matched old/new ownership behavior.

## 118 - UpdateMask reusable vector storage
UpdateMask storage becomes `std::vector<uint32>`, allowing capacity reuse and RAII. Serialized mask bytes and bit operations matched the old implementation over 200,000 randomized operation sequences.

## 119 - Query result row RAII
Shared query-row Field array ownership becomes `std::unique_ptr<Field[]>` for both MySQL and PostgreSQL. `Fetch()` still returns `nullptr` after EndQuery, preserving the old exhausted-result API edge case.

## audit-120 - Warden temporary buffer ownership
Removes an unchecked fixed 24-byte Warden stack buffer, removes a neighboring temporary heap, and logs Lua strings without an allocation. Pad/reverse binary output matched over 200,000 randomized cases. BigNumber consumes SetBinary input synchronously.

## 121 - Nonmutating system message lines
Three remaining mangos_strdup + strtok system-message paths use the centralized non-mutating string_view splitter introduced by the earlier chat packet work. strtok-equivalent line output matched over 250,000 randomized inputs including embedded NULs.

## 122 - SQLStorage buffer RAII
Record storage keeps char-array allocation semantics via `unique_ptr<char[]>`; the lookup index becomes `vector<char*>` and may reuse capacity on reload. Record addresses, raw string-field ownership and index semantics remain unchanged.

## 123 - Named query result move ownership
Both MySQL and PostgreSQL named-query paths move the field-name vector and transfer QueryResult through `unique_ptr`, eliminating the duplicate string/vector copy and making result ownership exception-safe.
