# 0045 semantic audit
- Replaces obsolete temporary-spawn API names with Penqle native summon semantics.
- Recreates the old requester-local spell-visual behavior by constructing the native visual packet and sending it only to the requester's session; avoids broadcasting the debug visual to all nearby clients.
- Migrates remaining WorldLocation fields in the action.
- `SeeSpellAction.cpp` compile: PASS.
