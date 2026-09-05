# mod-worldoverlay

`mod-worldoverlay` is a server-side map, instance, content and teleport overlay module for Tortoise.

The module treats an existing instanced client map as reusable geometry and places one or more named, server-managed world layers on top of dedicated runtime instances. The original dungeon/raid behavior remains available in parallel.

## Status

Design and schema bootstrap. No runtime code is implemented yet.

The first implementation target is a named singleton overlay on map 36 (`tele_city`) without modifying normal Deadmines entry, exits, player/group instance binds, or base spawn tables.

The current documentation intentionally describes a larger long-term architecture than the bootstrap schema implements. The next code milestone remains a narrow runtime proof before the broader feature set is added.

## Client baseline

The relevant client for this project is the **Turtle client (1.8 generation)**, not a stock TBC 2.4.3 client.

WorldOverlay's core design remains server-side and must not depend on unverified client features. However, Turtle contains custom client work, so potential map/terrain/phase-like capabilities must be audited against the actual Turtle client before being accepted or rejected.

A dedicated research track now documents the idea of combining WorldOverlay with verified Turtle client geometry/map switching capabilities if such support exists. Older generic references to stock 2.4.3 limitations are superseded by that client-baseline rule.

## Core idea

A named overlay has a stable logical identity such as:

```text
tele_city -> map 36 -> runtime instance 417
```

The runtime instance id is ephemeral. Content and teleports use the stable overlay key, never a hard-coded runtime instance id.

The same client geometry can therefore serve multiple isolated server-side worlds:

```text
Map 36
|- normal Deadmines instance A
|- normal Deadmines instance B
|- overlay: tele_city
|- overlay: bot_lab
|- overlay: event_world
`- overlay template: player_house/<owner>
```

Long-term, an overlay definition may use different allocation policies such as one singleton world, one instance per player, one per group, or an ephemeral scenario instance.

## WorldOverlay is more than a teleport module

Teleport resolution is only one consumer of the overlay identity. The design now covers a staged path toward:

- named runtime instances;
- overlay-only GameObject and creature spawns;
- reusable teleport destinations and source bindings;
- allocation policies (`SINGLETON`, `PER_PLAYER`, `PER_GROUP`, `EPHEMERAL`);
- overlay state variables;
- content variants/phases on unchanged client geometry;
- server-side trigger volumes;
- entry/exit/graveyard anchors;
- overlay-owned NPC waypoints;
- spawn groups, pools and later formations;
- access rules and schedules;
- clone/export/diff tooling;
- a later `base_spawn_policy=NONE` mode for clean geometry-template reuse;
- a future continent-overlay mode for Kalimdor/Eastern Kingdoms;
- an optional client-assisted geometry-variant path, but only if the Turtle client audit proves a safe capability.

The roadmap deliberately stages these features so they do not obscure the first instance-lifecycle proof.

## Retail phasing / Zidormi comparison

Modern Retail can switch a player between logical versions of a place using player phase state and, in newer clients, visible-map/terrain-swap systems.

WorldOverlay does not assume equivalent facilities exist in the Turtle client, but it also must not reject them merely because a stock Vanilla/TBC client lacked them. The actual Turtle 1.8 client implementation is the source of truth.

Instead:

```text
same terrain, different content
    -> WorldOverlay state / content variant

different named server-side world on an existing instanced map
    -> WorldOverlay runtime instance

verified Turtle client geometry/map variant
    -> optional WorldOverlay client-assisted geometry policy

truly different terrain/assets with no usable client support
    -> still requires client-side geometry/assets
```

This gives a Zidormi-like player-facing concept for many server-side use cases while leaving a clean path for richer geometry variants if Turtle's client capabilities support them.

## Safety invariants

- Normal Tortoise instance entry and portal behavior must remain unchanged.
- `creature` and `gameobject` remain base-world tables; overlay-only spawns live in module-owned tables.
- Builder commands must never silently write overlay objects into base spawn tables.
- Runtime instance ids are implementation details and must not be used as persistent content identifiers.
- A named overlay must be resolvable independently of player/group dungeon binds.
- Initial implementation supports inherited base spawns plus overlay spawns. A clean/no-base-spawn mode requires a verified spawn-loading hook.
- Module policy remains in `mod-worldoverlay`; any required core adapter should be generic and minimal.
- Client-assisted geometry features remain optional research until verified against the actual Turtle client and synchronized with server collision/navigation data.

## Command namespace

Primary command namespace:

```text
.wo
```

Long alias:

```text
.woverlay
```

See [`docs/COMMANDS.md`](docs/COMMANDS.md).

## Documentation

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) - current architecture contract and implementation constraints
- [`docs/ROADMAP.md`](docs/ROADMAP.md) - staged feature roadmap, testing criteria, allocation/state/trigger/variant design and long-term direction
- [`docs/PRIOR_ART.md`](docs/PRIOR_ART.md) - researched precedents: Tortoise instance state, Garrison-style ownership, guild houses, spawn groups/pools, waypoints and Retail Zidormi/phasing concepts
- [`docs/COMMANDS.md`](docs/COMMANDS.md) - current and planned `.wo` / `.woverlay` command surface
- [`docs/TURTLE_CLIENT_OVERLAY_IDEAS.md`](docs/TURTLE_CLIENT_OVERLAY_IDEAS.md) - Turtle 1.8 client audit track, continent overlays and optional geometry-variant ideas

## Data model

Initial world DB tables:

- `worldoverlay_overlay` - named overlay definitions
- `worldoverlay_destination` - reusable teleport destinations
- `worldoverlay_gameobject` - overlay-only GO spawns
- `worldoverlay_creature` - overlay-only creature spawns
- `worldoverlay_teleport_binding` - source entry to destination bindings

The bootstrap schema is intentionally smaller than the roadmap. Future migrations may add allocation, state, variants, triggers, anchors, waypoints, groups, pools and access rules only when the corresponding runtime phase is implemented.

See `data/sql/world/`.

## Compatibility model

Normal portal:

```text
portal -> map 36 -> normal Tortoise player/group bind -> ordinary Deadmines instance
```

WorldOverlay teleport:

```text
Chronostone / overlay portal / command
    -> destination key
    -> overlay key
    -> named runtime instance
    -> destination coordinates
```

The two paths are intentionally independent.

## Initial example

The repository contains a non-migration example for the existing Tele City build:

```text
examples/tele_city.sql
```

It defines the logical overlay and Moonwell destination only. It does not create or alter the existing normal Deadmines portals.

## Branch

Development branch: `module-worldoverlay`, based on `livingbots/playerbots-clean-base`.
