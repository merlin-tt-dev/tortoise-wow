# Core Performance / Deep Audit patches 184–203

Baseline: exact audited source state after `core-optimization-183`.

The baseline was produced from the user-supplied complete patch-163 archive plus the already frozen 164–183 bundle. No GitHub/source chunks were used to reconstruct source files.

This block continues the C++17/performance audit while preserving observable legacy behavior, including proprietary-client-facing ordering and gameplay semantics.

## Scope

- membership-only tree sets converted to hash sets where no iteration/order is observed;
- per-object node containers replaced with contiguous storage only where iterator/lifetime/order contracts permit it;
- DBC string-pool ownership list made contiguous;
- Unit prohibit-spell timer workset made contiguous with stable survivor ordering;
- legacy fixed-size `sprintf` calls bounded without changing valid output;
- dead per-object state removed only after whole-tree use checks;
- VMap assembler miss lookup reduced while protecting allocation ownership;
- Discord command-link registration reduced to a single associative lookup;
- spell-script condition validation changed to membership-oriented storage.

No BG queue / replacement core-only-011 work is included in this bundle.

## Integration

Apply files in `APPLY-ORDER.txt` order. The user normally applies/commits patches manually between logical groups.

Full Linux/MinGW builds are intentionally left to the user's build server. Local validation here covers patch applicability, source semantics, differential tests where appropriate, and tree identity.
