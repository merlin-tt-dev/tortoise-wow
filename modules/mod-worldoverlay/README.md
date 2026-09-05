# mod-worldoverlay

`mod-worldoverlay` is a server-side map and teleport overlay module for Tortoise.

The module treats an existing instanced client map as reusable geometry and places a named, server-managed world layer on top of a dedicated runtime instance. The original dungeon/raid behavior remains available in parallel.

## Status

Design and schema bootstrap. No runtime code is implemented yet.

The first implementation target is a named overlay on map 36 (`tele_city`) without modifying normal Deadmines entry, exits, player/group instance binds, or base spawn tables.

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
`- overlay: event_world
```

## Safety invariants

- Normal Tortoise instance entry and portal behavior must remain unchanged.
- `creature` and `gameobject` remain base-world tables; overlay-only spawns live in module-owned tables.
- Builder commands must never silently write overlay objects into base spawn tables.
- Runtime instance ids are implementation details and must not be used as persistent content identifiers.
- A named overlay must be resolvable independently of player/group dungeon binds.
- Phase 1 supports inherited base spawns plus overlay spawns. A clean/no-base-spawn mode is planned but requires a verified spawn-loading hook.

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

## Data model

Initial world DB tables:

- `worldoverlay_overlay` - named overlay definitions
- `worldoverlay_destination` - reusable teleport destinations
- `worldoverlay_gameobject` - overlay-only GO spawns
- `worldoverlay_creature` - overlay-only creature spawns
- `worldoverlay_teleport_binding` - source entry to destination bindings

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and `data/sql/world/`.

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
