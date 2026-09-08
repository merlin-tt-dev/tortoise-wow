# Core optimization patches 013-022

Base: `dev-clean` after `core-optimization-012-mmap-reuse-associative-lookups.patch`.

Apply order:

1. `core-optimization-013-worldsocket-steady-clock.patch`
2. `core-optimization-014-addon-message-allocation-reduction.patch`
3. `core-optimization-015-remove-unary-function-shim.patch`
4. `core-optimization-016-static-gm-spell-list.patch`
5. `core-optimization-017-avoid-countif-element-copies.patch`
6. `core-optimization-018-reuse-associative-lookups.patch`
7. `core-optimization-019-precompile-restore-go-regex.patch`
8. `core-optimization-020-buffer-honor-report-output.patch`
9. `core-optimization-021-threadpool-move-transient-values.patch`
10. `core-optimization-022-discord-log-allocation-reduction.patch`

## Intent

- 013: use monotonic `std::chrono::steady_clock` for ping flood timing and standard C++ sleep for the auth delay.
- 014: remove by-value addon-message parameters/copies and build the wire payload with one reserved string allocation.
- 015: remove the local legacy `unary_function` compatibility shim; `DefaultTargetSelector` does not use its typedefs.
- 016: make the fixed GM spell table static constexpr instead of allocating a linked list on every call.
- 017: stop `std::count_if` lambdas copying `Group::MemberSlot` and `QuestStatusMap` elements.
- 018: reuse associative-container lookup results / use direct emplacement instead of repeated lookup plus `operator[]`.
- 019: compile the invariant lost-GO recovery regex once and enable `std::regex::optimize`.
- 020: stop flushing the honor report once per player and stop copying `WeeklyScore` records.
- 021: use move semantics for transient ThreadPool names and `std::function` values where ownership is transferred.
- 022: remove extra Discord log/message copies and reuse the channel lookup iterator.

## Validation

All ten patches were validated sequentially from the 012 state with:

```bash
git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --check --whitespace=error-all PATCH

git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --numstat --whitespace=error-all PATCH

git apply PATCH
git diff --check
```

Additional check: no newly-added patch line contains tab indentation.

No new full-build result is claimed for 013-022 yet; run the Linux native and Windows-x64 audits after application.
