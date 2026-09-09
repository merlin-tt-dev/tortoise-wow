# Semantic audit

## 001 Connection/core interface

- PostgreSQL connection now calls the current `SqlConnection(Database&)` constructor.
- `m_use_socket` replaces the removed `m_socket` member.
- prepared statement cache is released before connection destruction/reconnect.
- `DatabasePostgre` mirrors the existing `DatabaseMysql` StopServer/db_count lifetime pattern.

## 002 libpq result ownership

- every temporary `PGresult*` returned by `PQexec` is owned by a local RAII handle.
- successful SELECT ownership is released exactly once to `QueryResultPostgre`.
- command/transaction error and success paths no longer leak results.
- `PQescapeStringConn` binds escaping to the active connection/encoding.

## 003 result value semantics

- `PQgetisnull()` distinguishes SQL NULL from the empty string.
- `Field::GetBool()` retains numeric behavior and additionally accepts PostgreSQL boolean text `t` / `true` case-insensitively only for `DB_TYPE_BOOL`.

## 004 CMake backend selection

- MySQL remains the default.
- PostgreSQL is selectable on UNIX via `-DDATABASE_BACKEND=POSTGRESQL`.
- invalid backend values fail configuration.
- Windows PostgreSQL is rejected explicitly rather than partially configuring with missing dependencies.

## 005 multiline execution

- libpq `PQexec` accepts multi-statement strings synchronously.
- both command and tuple result statuses are accepted, matching the existing MySQL behavior where returned resultsets are drained rather than treated as failure.

## 006 AutoUpdater metadata dialect

- only the updater's internal `migrations` metadata DDL/introspection is backend-specific.
- PostgreSQL uses BIGSERIAL, TIMESTAMP and `information_schema.columns`.
- MySQL SQL remains unchanged.

## 007/009 delay-thread cleanup

- unused stale MySQL/PostgreSQL delay-thread subclasses were removed.
- the real shared worker now calls the existing virtual `Database::ThreadStart/ThreadEnd` hooks.
- MySQL still executes `mysql_thread_init/end`; PostgreSQL uses the base no-op hooks.

## 008 connection recovery

- normal query/execute/multiline paths may reconnect once on `CONNECTION_BAD`.
- transaction control commands intentionally do not reconnect because reconnecting destroys transaction state.
