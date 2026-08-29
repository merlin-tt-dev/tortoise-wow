# 0042 semantic audit
- Replaces the obsolete `sCreatureStorage` compatibility path in `TravelValues.cpp` with Penqle's native `sObjectMgr.GetCreatureInfoMap()` access.
- Removes the now-unused compatibility shim block instead of adding another adapter.
- `TravelValues.cpp` compile: PASS.
