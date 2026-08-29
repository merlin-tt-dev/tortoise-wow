# 0046 semantic audit
- Migrates battleground type/team identifiers to Penqle native APIs.
- Uses `Player::CanInteractWithGameObject()` so alive/control/range/spawn checks come from the host core.
- Uses native AV team indices and native WSG flag-state/carrier access.
- Migrates locations to the native `WorldPosition`/WorldLocation representation.
- `BattleGroundTactics.cpp` and `BattleGroundTacticsAV.cpp` compile: PASS.
