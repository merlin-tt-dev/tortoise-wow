# 0043 semantic audit
- Migrates Generic/Range trigger spell helpers to native `Spells::*` APIs.
- Uses native target GUID/group/combat-reach/melee-reach APIs instead of fork-only helpers.
- Movement comparison uses Penqle's movement-flag-aware speed path rather than a fixed run-speed approximation.
- Spell readiness is routed through the existing `ServerFacade::IsSpellReady` abstraction.
- Keeps attack-on-sight semantics on the native host-core capability rather than introducing a shim.
- `GenericTriggers.cpp` (including header-instantiated Range triggers) compile: PASS.
