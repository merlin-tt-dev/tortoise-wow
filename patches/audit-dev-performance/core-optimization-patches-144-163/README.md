# Core Optimization 144-163

Baseline: the local 143 state obtained by applying the already deep-audited 124-143 bundle to a fresh 1:1 copy of the complete `audits/dev-performance` source archive supplied by the user. The supplied archive remains the ultimate source of truth; no source-remote state was used to reconstruct code.

This handoff contains 20 patches, 144 through 163, in exact numeric apply order. Audit/correctness fixes use the `core-optimization-audit-*` prefix. No automatic apply/worktree scripts are included.

The proprietary game client is treated as a hard compatibility boundary. No patch in this block intentionally changes packet/wire layout, serialization, GUID/update-mask ordering, RNG-to-target mapping, gameplay selection order, or observable completion/timing contracts.

## Block overview

- 144-145: Transmog membership and nested-container lookup allocation/lookup reductions
- 146-151: immutable DBC/ObjectMgr lookup reuse and associative lookup/erase reuse
- 152-153: ownership-aware move propagation for already-owned strings/event batches
- 154-158: Aura/LFG/proc hot-path keyed/contiguous workset improvements
- 159-161: DBC row-view reuse, summon map accessor reuse, single-pass C-string duplication
- audit-162: DBC reload field-offset ownership leak fix with conservative replacement ordering
- audit-163: bounded formatting for legacy worker/database thread-name buffers

After integration of this bundle, the next free patch number is `164`.

## Build gate

No full project build is claimed here. Per project workflow, Linux/Windows full builds are run by the user on the build server. This bundle was instead subjected to patch-level, source-contract, and differential validation; see `VALIDATION.txt` and `SEMANTIC-AUDIT.md`.
