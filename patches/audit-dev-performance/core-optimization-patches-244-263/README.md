# Tortoise Core Performance / Deep Audit — patches 244–263

Branch track: `audits/dev-performance`

This bundle continues from the **corrected** 224–243 series. Do not use the earlier withdrawn 224–243 archive as its parent.

## Parent and result

- Required corrected Patch-243 parent tree: `0008e24c9484620a5ad508fafbf5bd85fbd4a9ca`
- Patch-263 source tree: `41fa75a86d7438b756adf7f5960f14c722198cf1`
- Patch count: 20
- Apply order: `APPLY-ORDER.txt`

## Scope

This block deliberately avoids another sweep of reloadable global hash tables. It concentrates on:

- fixed-size/permanent data represented without heap containers;
- direct dispatch for tiny immutable key sets;
- repeated object-internal accessor/member reuse in Unit/Player/Aura/EventAI hot paths;
- removal of container copies only where the source traversal is proven read-only.

Two visually similar copy-removal candidates were explicitly rejected because the old copies are semantic snapshots: `Player::DismountCheck` mutates its Aura source while iterating, and `Spell::DoSpellHitOnUnit` can mutate the target attacker set through `AttackStop()`.

See `SEMANTIC-AUDIT.md`, `REJECTED-CANDIDATES.md` and `VALIDATION.txt`.

## Integration

Apply manually in exactly the order in `APPLY-ORDER.txt`. No auto-apply script is included.

No full local Linux/Windows/MinGW build was performed; authoritative full builds remain the user's build-server step.
