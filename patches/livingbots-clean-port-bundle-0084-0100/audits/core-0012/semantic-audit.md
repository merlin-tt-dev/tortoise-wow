# core-0012: Read-only native graveyard map accessor

Adds ObjectMgr::GetGraveYardMap() const, exposing already-loaded graveyard data read-only for the bot alternate-graveyard search. No mutation or bot-specific manager is introduced.

Ownership: **CORE**.
