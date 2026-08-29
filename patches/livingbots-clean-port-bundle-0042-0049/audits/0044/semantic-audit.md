# 0044 semantic audit
- Migrates formation `WorldLocation` access to Penqle fields/API.
- Uses native LOS hit-position functionality.
- Preserves the distinction between movement-flag flying and Player free-flight/aura state; it does not collapse both old checks into one.
- `Formations.cpp` compile: PASS.
