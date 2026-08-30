# Architecture audit 0084-0100

This batch materially shrinks the compatibility surface. It removes the monolithic cmangos compatibility shim, fake stores, dead stub headers, legacy team/spell/map aliases and several fork-only API assumptions. Native Penqle/Tortoise APIs are used at call sites.

The only Core change is `core-0012`, a const read-only getter for ObjectMgr's already-loaded GraveYardMap. It exists solely because DeadValues preserves an alternate-graveyard search that cannot be expressed through the existing closest-graveyard function alone. No bot-specific manager, mutation hook or duplicated DB load is added.
