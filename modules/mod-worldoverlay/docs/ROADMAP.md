# WorldOverlay roadmap

This document turns the current WorldOverlay design discussion into an implementation sequence. The priority is to keep the normal Tortoise world and dungeon flow untouched while adding a reusable server-side overlay system on top of existing instanced maps.

## Design north star

WorldOverlay is not a replacement for maps, dungeons, phasing, or the normal world database. It is a server-side layer that gives a stable logical identity to a specific use of an existing instanced client map.

The fundamental model is:

```text
client geometry
    + map id
    + named overlay definition
    + runtime instance allocation
    + overlay-owned content/state
    = logical server-side world
```

The same client map can therefore serve multiple independent worlds at the same time:

```text
Map 36
|- normal Deadmines instance A
|- normal Deadmines instance B
|- overlay: tele_city
|- overlay: bot_lab
|- overlay: event_world
`- overlay: player_house/<owner>
```

Runtime instance ids remain ephemeral implementation details. Persistent content refers to stable overlay keys and, where needed, a logical owner key.

## Guiding invariants

- `1181dev` and normal Tortoise dungeon behavior remain untouched.
- Normal portals and AreaTriggers keep using the ordinary bind-driven instance path.
- Overlay spawns never silently become base `creature` or `gameobject` rows.
- Overlay identities never depend on a hard-coded runtime instance id.
- New client geometry is not required for server-side world variants that reuse existing terrain.
- Client patches remain optional and only become relevant when genuinely different terrain/assets are required.
- Module policy stays in `mod-worldoverlay`; any unavoidable core change must be generic and minimal.

## Phase 0 - architecture freeze and runtime spike

Goal: prove that the basic named-instance model is safe before expanding the feature surface.

### Deliverables

- Audit the exact `livingbots/playerbots-clean-base` map/instance lifecycle.
- Prove collision-safe runtime instance allocation for a named overlay.
- Prove an instance-aware player transfer into an exact `(mapId, instanceId)` without modifying permanent player/group dungeon binds.
- Prove a dedicated overlay instance can be restored after unload/restart using the same logical overlay key and a newly allocated runtime id.
- Prove one overlay-owned GameObject exists only in the named instance.
- Verify ordinary Deadmines entry still creates/uses normal instances and does not see overlay content.

### Exit criteria

The following must all be demonstrated on map 36:

```text
normal Deadmines entry -> ordinary instance -> no overlay test GO
.wo enter tele_city    -> named instance    -> overlay test GO visible
restart                -> new runtime id     -> same logical content visible
```

No builder expansion should precede this proof.

## Phase 1 - minimal named overlay runtime

Goal: provide the first production-capable singleton overlay.

### Runtime components

- `WorldOverlayManager`
- logical overlay registry
- runtime instance registry
- lifecycle handling
- overlay-aware teleport manager
- module startup/reload validation

### Initial allocation policy

Only one policy is required initially:

```text
SINGLETON
```

Meaning one active logical world exists for an overlay key:

```text
tele_city -> one runtime instance at a time
```

### Initial lifecycle policies

```text
PERSISTENT
WHEN_EMPTY
MANUAL
```

`PERSISTENT` refers to logical/content persistence; the runtime map may still unload and later be recreated.

## Phase 2 - builder and overlay-owned content

Goal: make overlays editable in-game without contaminating the base world.

### Content types

- GameObjects
- Creatures/NPCs
- reusable teleport destinations
- simple teleport bindings

### Commands

Canonical namespace:

```text
.wo
```

Long alias:

```text
.woverlay
```

Initial builder surface:

```text
.wo enter <overlay>
.wo leave
.wo info
.wo where

.wo go add <entry>
.wo go move
.wo go turn
.wo go delete
.wo go info

.wo npc add <entry>
.wo npc move
.wo npc turn
.wo npc delete
.wo npc info

.wo dest add <key>
.wo dest goto <key>
.wo dest delete <key>
```

All builder mutations require an active overlay context and write only module-owned data.

## Phase 3 - allocation policies: use maps as reusable templates

Goal: allow one overlay definition to produce multiple isolated runtime worlds.

Planned policies:

```text
SINGLETON   - one shared instance for the overlay key
PER_PLAYER  - one instance per player/owner
PER_GROUP   - one instance per group
EPHEMERAL   - temporary instance, normally destroyed when empty
MANUAL      - explicitly allocated/released by GM or script
```

Examples:

```text
tele_city
  allocation = SINGLETON

player_house_small
  allocation = PER_PLAYER

party_scenario
  allocation = PER_GROUP

dream_sequence
  allocation = EPHEMERAL
```

The persistent identity then becomes conceptually:

```text
(overlay_key, owner_scope)
```

rather than a runtime instance id.

This is the point where an instanced map becomes a reusable server-side world template rather than merely a dungeon copy.

## Phase 4 - overlay state and content variants

Goal: support Zidormi-like logical world changes when the underlying client geometry can remain the same.

### State variables

Named overlays should support typed logical state such as:

```text
market_active = 1
story_stage = 3
tower_door_open = 0
season = halloween
```

Planned uses:

- enable/disable spawn groups
- alter script behavior
- select NPC/GO variants
- gate teleports/triggers
- store event/story progression

### Content variants

A variant is a named content presentation within the same overlay geometry:

```text
tele_city/default
tele_city/halloween
tele_city/attacked
```

Variants may switch:

- NPCs
- GameObjects
- spawn groups
- scripted behavior
- destinations
- triggers

without changing terrain.

### Retail comparison: Zidormi/phasing

Retail uses player phase state and, in modern clients, can also use visible-map/terrain-swap systems to present old/new versions of a zone. WorldOverlay must distinguish those concepts:

```text
Retail content phase:
  same logical zone + player phase state + different visible objects

Retail terrain swap:
  same logical zone + alternate client terrain/map presentation

TBC WorldOverlay content variant:
  same client geometry + separate server-side state/content

TBC WorldOverlay map variant:
  separate named runtime map/instance using whatever client geometry that map already contains
```

The 2.4.3 client must not be assumed to support modern PhaseShift/TerrainSwap infrastructure. WorldOverlay therefore provides the server-side isolation/variant layer without depending on those later client systems.

## Phase 5 - trigger volumes, entry/exit and graveyards

Goal: make overlays behave like complete worlds rather than static spawn containers.

### Trigger volumes

Support server-side trigger geometry modeled after established AreaTrigger concepts:

```text
SPHERE
BOX
```

Candidate trigger actions:

```text
TELEPORT
SET_STATE
SET_VARIANT
ENABLE_GROUP
DISABLE_GROUP
RUN_SCRIPT
MESSAGE
```

Example builder flow:

```text
.wo trigger add sphere tower_entry 2.0
.wo trigger action tower_entry teleport tower_top
```

This provides a clean replacement for fragile DB-only custom trap/spell hacks when all that is required is server-side proximity behavior.

### World anchors

Each overlay should eventually support explicit logical anchors:

```text
ENTRY
EXIT
GRAVEYARD
FALLBACK
```

Possible commands:

```text
.wo anchor set entry
.wo anchor set exit
.wo anchor set graveyard
```

## Phase 6 - movement and dynamic spawn composition

Goal: support richer living-world content while retaining overlay isolation.

### Waypoints

Overlay creatures should own overlay waypoint data rather than writing into global movement tables.

Planned commands:

```text
.wo npc wp start
.wo npc wp add
.wo npc wp delete
.wo npc wp show
```

### Spawn groups

Logical groups allow complete sets of objects to be switched together:

```text
tele_city_market
|- vendor A
|- vendor B
|- tent
|- crates
`- fire
```

Actions:

```text
.wo group enable tele_city_market
.wo group disable tele_city_market
```

State integration example:

```text
market_active = 1 -> group tele_city_market enabled
```

### Pools

Pools provide controlled random alternatives:

```text
daily_vendor
|- vendor A : weight 30
|- vendor B : weight 30
`- vendor C : weight 40
```

Pools should reuse established MMO-server concepts rather than inventing unrelated semantics.

### Formations

Creature formations are a later extension for guards, patrols, escorts and grouped movement.

## Phase 7 - clean template mode

Goal: allow a named overlay to reuse only the map geometry while suppressing ordinary base spawns.

Current/initial policy:

```text
INHERIT
```

Future policy:

```text
NONE
```

`NONE` must not delete or mutate ordinary spawn rows. It requires a verified instance-specific base-spawn loading hook/filter.

This is a high-value capability because it turns an existing instanced map into a clean reusable template:

```text
client geometry
+ no base dungeon content
+ overlay-only content
= completely different server-side world
```

## Phase 8 - access, scheduling and reusable tooling

Goal: make WorldOverlay a maintainable content platform rather than a collection of one-off scripts.

### Access rules

Potential conditions:

- GM/security level
- faction/team
- level
- quest state
- item possession
- group/raid membership
- owner scope
- scripted predicate

### Schedules

Allow overlay states/groups/variants to change on a schedule, for example:

```text
night guards
daily market
seasonal event
temporary invasion
```

### Clone/export/diff

Useful builder/admin operations:

```text
.wo clone <source> <target>
.wo export <overlay>
.wo validate <overlay>
.wo diff <overlay-a> <overlay-b>
```

A clone should duplicate module-owned logical content, never a live runtime instance id.

## Phase 9 - inheritance and composable overlays

Goal: reduce duplication for related world variants.

Possible model:

```text
tele_city
  `- tele_city_halloween
       inherits base Tele City content
       overrides/adds Halloween content
```

Inheritance should be considered only after state/groups/variants are stable because premature inheritance can complicate spawn identity, deletion semantics and builder behavior.

## Feature boundary: reference templates, do not fork the world DB

WorldOverlay should reference normal world templates wherever practical:

```text
creature_template
gameobject_template
item_template
quest_template
vendor/trainer definitions
```

Overlay tables should describe placement, ownership, state and per-spawn overrides rather than cloning entire template definitions.

Per-spawn overrides may later include:

- scale
- faction
- display id
- emote/stand state
- aura set
- movement/path binding
- script/behavior metadata

The underlying template remains authoritative unless an overlay-specific override is explicitly configured.

## Prior art and design influences

No single reviewed system currently provides the complete WorldOverlay model, but several established systems validate individual parts:

- normal WoW dungeon instances already separate runtime worlds by `(mapId, instanceId)`;
- dungeon `InstanceScript`-style state demonstrates per-instance logical state;
- owner-bound/garrison-style maps demonstrate logically owned map instances;
- guild-house/phasing systems demonstrate private/shared world content built on existing geometry;
- waypoint systems demonstrate database-driven NPC movement;
- spawn groups and pools demonstrate grouped and randomized content activation;
- modern Retail phasing/Zidormi demonstrates the player-facing concept of switching between logical versions of a place.

WorldOverlay combines these ideas into one Tortoise-specific server-side abstraction while deliberately avoiding dependence on modern client phasing/terrain-swap features.

## Proposed long-term module components

```text
WorldOverlayManager
|- OverlayRegistry
|- RuntimeInstanceRegistry
|- AllocationManager
|- LifecycleManager
|- SpawnManager
|- TeleportManager
|- StateManager
|- VariantManager
|- TriggerManager
|- WaypointManager
|- SpawnGroupManager
|- PoolManager
|- AccessManager
`- BuilderCommandScript
```

Not all components belong in v1. The roadmap intentionally stages them so the instance lifecycle is proven first.

## Schema evolution direction

The existing bootstrap schema remains intentionally small. Future migrations are expected to add or normalize concepts such as:

```text
worldoverlay_overlay
  + allocation_policy
  + lifecycle configuration
  + default_variant

worldoverlay_runtime         # normally runtime/character-side persistence, if needed
worldoverlay_state
worldoverlay_variant
worldoverlay_trigger
worldoverlay_anchor
worldoverlay_waypoint_path
worldoverlay_waypoint
worldoverlay_spawn_group
worldoverlay_spawn_group_member
worldoverlay_pool
worldoverlay_pool_member
worldoverlay_access_rule
```

These names are design placeholders until the corresponding implementation phase is reached. The initial schema should not be expanded merely to reserve every possible feature.

## Testing strategy

Every implementation phase should preserve a standard regression matrix:

```text
1. enter named overlay
2. verify correct runtime instance
3. verify overlay-only content visible
4. enter same map through normal dungeon path
5. verify overlay-only content absent
6. restart/reload
7. verify logical overlay survives without relying on old runtime id
8. verify no base world rows were modified by overlay builder operations
```

Allocation phases add isolation tests between owners/groups. State/variant phases add simultaneous-player tests to ensure one player's logical view does not unintentionally alter another scope.

## Immediate next step

Do not implement the expanded feature set yet. The next code milestone remains the Phase 0 runtime spike on map 36. Once exact instance allocation and targeted transfer are proven safe, implement the minimum singleton `tele_city` runtime and only then grow the builder and content model.
