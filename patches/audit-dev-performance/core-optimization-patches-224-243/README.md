# Tortoise Core Performance / Deep Audit — corrected patches 224–243

Branch track: `audits/dev-performance`

This is the **corrected and authoritative 224–243 bundle** produced after an adversarial second audit.

## Important supersession notice

An earlier 224–243 bundle was withdrawn after the second audit found that several global `std::map -> std::unordered_map` conversions had runtime-reload and/or concurrency semantics that were not proven equivalent. **Do not use the earlier bundle.** This corrected bundle was rebuilt from the exact locally established Patch-223 parent instead of adding follow-up fixes on top of the withdrawn series.

Removed/rejected from the withdrawn series were runtime-reloadable SpellMgr hashes, AccountMgr hashes with a changed rehash/race surface, and other structures whose ordering, iterator stability, reload behavior or side-effect order was not sufficiently proven.

## Parent and result

- Exact local Patch-223 parent source tree: `eefa858a77e695c83c51842be675727a42fcbd11`
- Corrected Patch-243 source tree: `0008e24c9484620a5ad508fafbf5bd85fbd4a9ca`
- Patch count: 20
- Apply order: `APPLY-ORDER.txt`

The parent was created from the user-supplied complete `audits/dev-performance` archive by first verifying that 204–223 were present as patch files but unapplied in the source, then applying them sequentially with the project validation rules.

## Scope

The corrected series focuses on exact-key local/startup indices, immutable lookup tables, fixed-size data, a dense opcode dispatch table, an already-locked Guild member index, and lookup/aggregation reductions whose ordering rules could be explicitly reconstructed.

Notable semantic constraints preserved include:

- client-visible talent bit packing remains driven by the original ordered `TalentBitSize` structure;
- inner Pet-family and Spell-category sets remain ordered;
- taxi source/path semantics remain driven by the original numeric source traversal and exact `(source,destination)` lookup;
- the MovementBroadcaster slow-map aggregation explicitly preserves the old `std::map` tie rule: for equal non-zero packet counts, the smallest instance ID wins;
- `pool2event` remains ordered;
- no runtime-reloadable SpellMgr global map is converted merely for lookup speed.

See `SEMANTIC-AUDIT.md`, `REJECTED-CANDIDATES.md` and `VALIDATION.txt` for the proof notes.

## Integration

Apply manually, in exactly the order listed in `APPLY-ORDER.txt`, committing between patches if that is the normal branch workflow. No automatic apply script is included.

No full local Linux/Windows/MinGW build was performed; authoritative full builds remain the user's build-server step.
