# Core Optimization 124-143

Baseline: the complete `audits/dev-performance` source archive supplied for this handoff.
The archive itself is the sole source of truth. It contains no `.git` metadata, so no commit/HEAD identity is claimed here.

The supplied source tree was checked directly against the existing 105-123 patch bundle: all 19 patches reverse-apply cleanly, confirming that 105 through 123 are already integrated in the delivered source state. Therefore 124 is the first free patch number for this block.

This handoff contains 20 patches, 124 through 143, in exact numeric apply order. Audit/correctness fixes use the `core-optimization-audit-*` prefix; runtime/modernization work uses `core-optimization-*`.

No apply/worktree script is included. Intended manual integration, one patch at a time:

    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent apply --check --whitespace=error-all PATCH
    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent apply --numstat --whitespace=error-all PATCH
    git apply PATCH
    git diff --check
    git add ...
    git commit

The complete 124-143 sequence was also applied and audited in a disposable 1:1 copy of the exact supplied source tree. After an adversarial second semantic pass, 127/128 were tightened for alias preservation and 32-bit output-size arithmetic, then the full 20-patch sequence was revalidated from the untouched supplied baseline. The final validated tree matches the deep-audited working tree exactly.

## Block overview

- 124-126: world/thread-pool cross-thread correctness
- 127-129: hex/string/LFT allocation reductions with differential tests
- 130-136: MMap value storage and associative lookup/range reuse
- 137-138: contiguous Spell chain/dispel worksets with order/RNG audit
- 139-142: safe capacity reuse in bounded hot-path snapshots
- 143: Detour MMap buffer ownership on failure paths

After integration of this bundle, the next free performance/audit patch number is `144`.

## Build status

Patch-level and semantic validation is green; see `VALIDATION.txt`.
A local Linux CMake configure was attempted, but the sandbox does not have ACE installed and configuration stops before project compilation. Therefore this bundle does **not** claim a Linux full build or MinGW full build. Those remain the external build gates.

No PostgreSQL-specific work is included.
