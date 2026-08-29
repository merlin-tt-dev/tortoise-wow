# LivingBots clean port — 0038

Apply after source patch 0037 / remote HEAD:

`835f1c0a605da4323220e909ece5e0f081cd520c`

Apply in the order listed in `APPLY_ORDER.txt`.

## Scope

0038 completes the first native Movement/Follow compatibility block:

- exposes read-only target/angle/offset from Penqle targeted movement generators;
- replaces the temporary `ServerFacade` chase defaults with real generator state;
- restores historical playerbot follow catch-up as an explicit opt-in on Penqle's existing MoveSpline path, without mutating persistent unit speed and without teleport boosting;
- migrates `MovementActions.cpp` from obsolete CMaNGOS APIs to current Penqle/Tortoise movement, LOS, combat-reach, taxi, transport, ground, liquid, fall and packet semantics;
- migrates the directly related `FollowActions.cpp` WorldLocation/target checks;
- adjusts the existing DebugAction follow callsite to the native Penqle signature.

No fake compatibility APIs, no parallel movement stack, and no changes under repository `patches/` are included.

## Build status

Primary audit target: Debian ACE 8.0.2.

Green translation units:

- `src/game/Movement/TargetedMovementGenerator.cpp`
- `src/game/Movement/MotionMaster.cpp`
- `modules/mod-playerbots/src/playerbot/ServerFacade.cpp`
- `modules/mod-playerbots/src/playerbot/strategy/actions/MovementActions.cpp`
- `modules/mod-playerbots/src/playerbot/strategy/actions/FollowActions.cpp`
- `modules/mod-playerbots/src/playerbot/strategy/actions/TaxiAction.cpp`

`DebugAction.cpp` still has 83 pre-existing independent port errors; the changed `MoveFollow` call itself produces no compiler error. Those errors are intentionally not folded into 0038.
