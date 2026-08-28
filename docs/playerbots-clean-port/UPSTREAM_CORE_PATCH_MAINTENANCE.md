# Upstream core-patch maintenance

This clean Playerbot port intentionally keeps the Penqle/Tortoise core as close to
`1181dev` as possible. Most Playerbot integration lives in `modules/mod-playerbots`.
The small patches under `patches/core-*` are exceptions and must be re-audited when
Penqle changes the touched core files.

## Rule for Penqle updates

Do not blindly re-apply a core patch after rebasing or merging a newer Penqle base.
For every touched file:

1. inspect the upstream diff first;
2. check whether Penqle now exposes an equivalent public API;
3. if an equivalent API exists, migrate the module and retire our core patch;
4. otherwise rebase only the minimum semantic delta;
5. build the unmodified core and `mod-playerbots` together;
6. run the Playerbot movement/chat end tests before accepting the update.

Compiler success alone is not sufficient. These patches affect runtime semantics.

## core-0001 / core-0002: chat channel lookup

Files to watch:

- `src/game/Chat/Channel.cpp`
- `src/game/Chat/Channel.h`
- `src/game/Chat/ChannelMgr.cpp`
- `src/game/Chat/ChannelMgr.h`

Purpose:

- expose/use the loaded channel id and exact channel-name semantics required by the
  historical bot command/chat layer.

Upstream retirement condition:

- Penqle provides the same exact channel-id/name lookup semantics through a public
  API used by `mod-playerbots`.

Regression checks:

- normal say/party/guild chat;
- numbered public channels;
- similarly named channels must not alias;
- bot command dispatch must not recurse through bot-generated chat.

## core-0003: map-only PathFinder/nav-area API

Patch:

- `patches/core-0003-pathfinder-map-navarea-api.patch`

Files to watch:

- `src/game/Maps/PathFinder.cpp`
- `src/game/Maps/PathFinder.h`

Purpose:

- allow full navmesh path calculation for travel-graph work without requiring a
  live `Unit*`;
- expose nav-area cost/query/marking needed by Playerbot travel routing.

Why this remains a core patch:

Penqle already exposes the lower-level Detour objects through `MMapManager`, so
simple area/flag queries could be implemented in the module. However, the travel
system also needs Penqle's complete `PathInfo` path-building semantics in map-only
mode. Reimplementing that algorithm inside the module would duplicate core
PathFinder behavior and create an upstream-divergence hazard.

Upstream retirement condition:

- Penqle adds a public map-only `PathInfo`/PathFinder constructor or equivalent
  path-query service with the same filter/path semantics.

Rebase-sensitive areas:

- `PathInfo` member initialization;
- `createFilter()` defaults;
- `BuildPolyPath()` swimming/flying shortcuts;
- `BuildPointPath()` / `BuildShortcut()`;
- Detour navmesh area/flag handling;
- mmap tile availability and path-type flags.

Regression checks:

- ordinary Unit-owned pathfinding must remain behaviorally equivalent where the
  map-only branch is not used;
- travel graph can calculate a path without a live player;
- magma/slime/steep-slope avoidance is preserved;
- dynamic nav-area marking does not downgrade hazardous areas;
- missing mmaps fail safely rather than fabricating a traversable route.

## core-0004: fixed pre-calculated path movement

Patch:

- `patches/core-0004-fixed-path-motion.patch`

Files to watch:

- `src/game/Movement/MotionMaster.cpp`
- `src/game/Movement/MotionMaster.h`
- `src/game/Movement/PointMovementGenerator.cpp`
- `src/game/Movement/PointMovementGenerator.h`

Purpose:

- let Playerbot hand a pre-calculated path to `MotionMaster` as a real movement
  generator;
- preserve MotionMaster ownership/lifecycle and speed-change recalculation.

Why this remains a core patch:

Penqle exposes `Movement::MoveSplineInit::MovebyPath()`, but a module cannot install
its own movement generator through `MotionMaster::Mutate()` because that operation
is private. Calling `MovebyPath()` directly from Playerbot would bypass the
MotionMaster generator lifecycle and lose the current speed-recalculation and
movement-type semantics.

Upstream retirement condition:

- Penqle exposes a native `MotionMaster::MovePath(...)`/equivalent public movement
  generator, or provides a supported extension point for module-owned movement
  generators with identical lifecycle semantics.

Rebase-sensitive areas:

- `MovementGeneratorType` enum values;
- `MotionMaster::Mutate()`/generator-stack behavior;
- `MoveSplineInit::MovebyPath()` path-index semantics;
- transport spline setup;
- walk/run/fly/fall/cyclic options;
- `UnitSpeedChanged()` handling and `currentPathIdx()` behavior.

Regression checks:

- non-cyclic path reaches all supplied nodes in order;
- first caller-supplied node is not silently discarded;
- cyclic paths repeat correctly;
- transport-relative movement remains valid;
- speed changes do not restart a non-cyclic path from node zero;
- interrupt/finalize clears roaming states;
- existing Point/Waypoint/Patrol generators are unchanged.

## Update workflow

Before modifying the working branch, record the Penqle base commit. After applying
an upstream update, compare each watched file against the previous Penqle base and
classify the result as:

- `UNCHANGED`: patch can normally be replayed, then re-tested;
- `CONFLICT`: manually rebase by semantics, never by accepting ours/theirs wholesale;
- `NATIVE_REPLACEMENT`: upstream now provides the required API; migrate module code
  and delete the corresponding core patch;
- `SEMANTICS_CHANGED`: stop and audit the module consumer before building.

After every core-patch write, inspect the exact diff immediately. `1181dev` itself
must never be modified; all rebasing/adaptation belongs on the clean Playerbot
working branch.
