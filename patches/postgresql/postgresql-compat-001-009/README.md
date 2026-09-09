# PostgreSQL Compatibility 001-009

Baseline: `audits/dev-performance` with core optimization patches through `123` applied.

This series restores the dormant PostgreSQL backend at the C++/build-system layer before the larger SQL dialect/schema port.

## Scope

- modern `SqlConnection(Database&)` and database lifecycle integration
- libpq `PGresult` RAII and connection-aware escaping
- correct PostgreSQL NULL/empty-string and boolean result semantics
- selectable CMake database backend (`MYSQL` default, `POSTGRESQL` on UNIX)
- multiline execution
- PostgreSQL-compatible AutoUpdater migration metadata table
- removal of obsolete backend-specific delay-thread wrappers
- backend-neutral SQL worker thread lifecycle
- one-retry PostgreSQL connection recovery outside transaction control commands

## Important

This does **not** mean the full server SQL is PostgreSQL-compatible yet. Core/game SQL still contains MySQL-specific syntax such as backtick identifiers, `REPLACE INTO`, `ON DUPLICATE KEY`, `UNIX_TIMESTAMP()` and MySQL-oriented base schemas/migrations. Those are the next compatibility phase.

Windows remains MySQL-only in this series because the repository's bundled Windows dependencies do not include libpq.
