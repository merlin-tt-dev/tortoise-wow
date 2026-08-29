# 0039 build audit

Toolchain: CMake/Ninja, `MODULES=disabled`, `MODULE_MOD_PLAYERBOTS=static`, `USE_PCH=OFF`, `USE_SCRIPTS=ON`, Debian ACE 8.0.2. Direct TU checks use the generated Ninja compile command with `-O0 -g0` for audit speed.

Green affected TUs:
- `RpgSubActions.cpp`
- `ChooseRpgTargetAction.cpp`
- `ItemForSpellValue.cpp`
- `DropQuestAction.cpp`
- `CheckMountStateAction.cpp`
- `MountValues.cpp`
- `CastCustomSpellAction.cpp`

`DebugAction.cpp` remains red from the pre-existing debug compatibility backlog (81 compiler errors in this sweep), but the changed `ManualChat(..., requester)` consumer produces zero ManualChat/signature errors.

`git diff --check`: PASS.
Remaining `GetRandomGenerator(` references in module: 0.
Remaining `GetSpellRecoveryTime(` references in module: 0.
