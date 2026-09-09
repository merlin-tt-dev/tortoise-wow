# dev-clean Core Optimization — audit/performance bundle 075–082

This bundle is for **dev-clean only** and assumes `core-optimization-074` is already applied.
No remote repository state is required or used.

## Naming

Patches `075–080` were produced by the semantic audit of earlier `core-optimization-*` changes and therefore use the dedicated prefix:

`core-optimization-audit-...`

Patches `081–082` are new performance optimizations and retain the normal:

`core-optimization-...`

## Contents

- `075` — defer associative construction so cache hits do not perform unnecessary work and Terrain insertion retains strong ownership/exception semantics.
- `076` — repair two latent compile regressions introduced by the earlier Threat report optimization while retaining its allocation reductions.
- `077` — remove invalid/unsafe generic deserializer `reserve()` behavior while preserving move semantics.
- `078` — prevent `size_t` underflow in the empty MotionMaster recovery path.
- `079` — retain the original malformed-account-warning exception semantics while avoiding duplicate string materialization.
- `080` — restore petition-signature snapshot semantics using contiguous pointer storage, avoiding iterator invalidation/UB and list-node allocation.
- `081` — replace additional short-lived Grid `std::list` snapshots with contiguous vectors where ordering semantics are unchanged.
- `082` — replace ChannelBroadcaster 1 ms polling with producer/consumer condition-variable wakeups.

## Apply

Use `APPLY-ORDER.txt`, or from the bundle root:

```bash
./AUDIT-TEST.sh /path/to/dev-clean
```

`AUDIT-TEST.sh` expects a clean source tree at the `074` source baseline and applies `075–082` in order using the strict whitespace checks.

## Validation performed

A separate fresh extraction of the user-provided source archive was initialized as a local Git repository. The archived `042–062`, existing `063–074`, and this bundle `075–082` were then applied **continuously in numeric order (`042→082`)**.

For every patch:

```bash
git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --check --whitespace=error-all PATCH

git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --numstat --whitespace=error-all PATCH

git apply PATCH
git diff --check
```

Final checks also verified:

- no `<<<<<<< ` / `>>>>>>> ` conflict markers in `src/`
- no newly-added source indentation beginning with tabs
- final `git diff --check` clean

A full Linux/MinGW/MSVC build is **not** claimed by this bundle; external full builds remain the compile/link gate.

See `SEMANTIC-AUDIT.md`, `VALIDATION.txt`, and `SHA256SUMS` for details.
