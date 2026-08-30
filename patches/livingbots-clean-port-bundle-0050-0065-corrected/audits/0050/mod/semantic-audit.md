# Semantic audit — 0050 mod

## Playerbot login integration

- PlayerbotLoginMgr consumes the core-provided `LoginQueryHolder` instead of duplicating the type.
- Uses native GUID/player lookup, `WorldLocation::mapId`, `WorldTimer::getMSTime()`, and the existing `WorldSession::HandlePlayerLogin(LoginQueryHolder*)` path.
- Adds the required standard `<future>` and timer includes.
- This patch contains **module files only** and depends on `0050-core-native-login-query-holder-extract.patch` being applied first.
