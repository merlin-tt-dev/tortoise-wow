# Turtle client overlay ideas

This document records a future WorldOverlay research direction for the Turtle client used by this project.

## Important client baseline

The project must **not** reason about client capabilities as if it were using a stock TBC 2.4.3 client.

The relevant client baseline is the **Turtle client (1.8 generation)** used by Tortoise/Turtle WoW. It contains custom client-side work beyond stock Vanilla-era assumptions, so modern-looking capabilities must be verified against the actual Turtle client rather than accepted or rejected from expansion-version folklore.

Any older WorldOverlay documentation that refers to a generic "2.4.3 client" or assumes stock-TBC phasing limitations should be treated as superseded by this rule.

## Why this matters for WorldOverlay

WorldOverlay already has a useful purely server-side model:

```text
existing client geometry
+ map id
+ logical overlay key
+ runtime map/instance allocation
+ overlay-owned content/state
= separate logical world
```

That works without requiring a new terrain implementation.

The Turtle client opens an additional research path:

```text
WorldOverlay server layer
        +
verified Turtle client map/terrain capabilities
        =
possible geometry-aware world variants
```

This must remain optional. WorldOverlay must still work as a server-side overlay system when no client-assisted geometry feature is available.

## Two separate capability layers

### Layer A - server-only overlay

This is the guaranteed design target.

Possible changes include:

- creatures/NPCs;
- GameObjects;
- spawn groups;
- triggers;
- destinations;
- state;
- content variants;
- access rules;
- schedules;
- different named runtime map/instance copies using geometry already known to the client.

Example:

```text
tele_city/default
tele_city/halloween
tele_city/attacked
```

All three can share the same terrain while presenting different server-side content.

### Layer B - client-assisted geometry variant

This is a research idea, not yet a confirmed feature.

If the Turtle client already contains suitable custom support, or exposes a clean mechanism that can be extended without destabilizing compatibility, WorldOverlay could optionally select alternate map/terrain presentations.

Conceptually:

```text
overlay: eastern_old
  server content variant = old
  client geometry variant = old_eastern

overlay: eastern_current
  server content variant = current
  client geometry variant = current_eastern
```

The goal would be a Zidormi-like world-version switch while preserving WorldOverlay's logical overlay model.

## Research questions for the Turtle client audit

Before any client-assisted design is accepted, verify the exact Turtle client implementation and data formats.

Audit at least:

1. Client version/build identity and custom patches relevant to world loading.
2. `Map.dbc` extensions or custom map metadata.
3. WDT/ADT loading behavior and whether one logical map can select alternate terrain data.
4. Whether Turtle added any equivalent of visible-map, terrain-swap, phase-shift, or map-remap behavior.
5. Whether the client can reload or switch terrain presentation while keeping the logical server map stable.
6. Whether map changes require a full `SMSG_NEW_WORLD`-style transition instead.
7. Whether alternate geometry must use a new MapID or can reuse an existing one with additional metadata.
8. Whether VMAP/MMAP/navigation data must be duplicated or can share geometry resources.
9. How collision, height, LOS, pathfinding and transport behavior remain synchronized with the client-visible terrain.
10. Whether Turtle's client patch distribution already has an established mechanism suitable for optional WorldOverlay geometry packs.
11. Whether custom terrain behavior is backward-compatible with players using the expected current Turtle client build.
12. Whether client cache behavior creates stale-world problems after changing an overlay geometry definition.

No capability should be documented as supported until this audit is backed by code/data evidence from the actual Turtle client/toolchain.

## Continent overlays

Tortoise already has server-side support for instanced continent parts for Eastern Kingdoms (Map 0) and Kalimdor (Map 1).

This suggests a future WorldOverlay scope beyond traditional dungeon maps:

```text
Overlay scope
|- INSTANCE_MAP
`- CONTINENT
```

A continent overlay could represent a logical world version such as:

```text
Kalimdor
|- original
|- dream
|- war
`- restored

Eastern Kingdoms
|- original
|- old
|- plagued
`- restored
```

The logical overlay should own the identity. Internal continent-part runtime map IDs remain implementation details.

## Continent-set concept

A continent overlay should not be modeled as one hard-coded runtime instance id.

Instead it should resolve to a **set of runtime map parts** managed under one logical overlay context.

Conceptually:

```text
overlay = eastern_old

runtime set:
  top_north
  middle_north
  ironforge_area
  middle
  stormwind_area
  south
```

When a player crosses a Tortoise continent-part boundary, the player must remain in the same logical WorldOverlay context while the server switches to the corresponding runtime part for that overlay.

This keeps the user-facing model simple:

```text
player is in overlay eastern_old
```

rather than exposing internal continent segmentation.

## Content-only Zidormi model

Even if no client terrain swap exists, a Zidormi-style interaction is still valuable.

Example:

```text
NPC: "Show me the world before the invasion."

server action:
  current overlay/variant -> eastern_old
```

Possible implementation strategies depend on scope:

```text
same terrain, per-player content only
  -> player-view/content variant

same terrain, fully isolated world state
  -> alternate WorldOverlay runtime set

alternate terrain supported by Turtle client
  -> alternate WorldOverlay runtime set + client geometry variant
```

The player-facing interaction can remain the same while the implementation becomes more capable as client support is verified.

## Resource model hypothesis

Do not assume a complete second continent necessarily duplicates every terrain resource in memory.

Tortoise `Map` instances currently reference terrain through the shared terrain manager, while runtime map state, active grids, creatures, GameObjects, scripts, weather, visibility and update work belong to each map context.

Therefore the likely cost model for a server-only continent overlay is driven primarily by **active runtime content**, especially:

```text
active map parts
active grids
players/bots
creatures and AI
GameObjects and dynamic collision
scripts/events
visibility and movement updates
```

rather than by the mere existence of an overlay definition in the database.

This must be benchmarked on the actual WorldOverlay implementation before assigning MB/CPU figures.

## Lazy continent allocation

A future continent overlay should be lazy by default.

Do not create every logical continent version and every runtime part on server startup.

Preferred model:

```text
overlay definition exists
  -> no runtime cost beyond metadata

first player enters eastern_old/stormwind
  -> create required runtime part

player crosses into another part
  -> create/resolve that part lazily

part remains empty long enough
  -> allow safe unload according to overlay lifecycle policy
```

This makes the number of configured overlays much less important than the number of concurrently active overlay parts.

## Geometry synchronization rule

If client-assisted terrain variants are ever implemented, the server and client must never disagree about geometry.

The following must match the selected variant:

- terrain height;
- collision/VMAP;
- pathfinding/MMAP;
- liquid data;
- transport paths where applicable;
- area/zone assumptions where relevant.

A visually changed client terrain with unchanged server collision is not an acceptable WorldOverlay feature.

## Architecture extension

A future overlay definition may therefore gain an optional geometry strategy:

```text
geometry_policy
  SHARED_BASE       # current server-only model
  CLIENT_VARIANT    # only after Turtle client capability is proven
  DISTINCT_MAP      # use another existing client MapID
```

Possible metadata:

```text
overlay_key
overlay_scope
map_id
content_variant
geometry_policy
geometry_key
```

`geometry_key` must remain logical metadata. It must not encode volatile runtime instance IDs.

## Compatibility principle

WorldOverlay should never require a client patch for features that can be implemented safely server-side.

Preferred order:

```text
1. server-only overlay
2. use geometry already present in the Turtle client
3. use verified Turtle client-native/custom switching capability
4. optional new client assets only when genuinely necessary
```

This preserves the original reason WorldOverlay is attractive: maximum world-design flexibility with minimal client coupling.

## Roadmap placement

This idea belongs after the server-side WorldOverlay runtime is proven.

Suggested research milestone:

```text
Phase X - Turtle client geometry capability audit

- identify exact Turtle 1.8 world-loading extensions
- verify map/terrain switching capabilities
- prototype one harmless alternate geometry selection if supported
- prove server/client collision synchronization
- decide whether CLIENT_VARIANT becomes a supported WorldOverlay geometry policy
```

It must not block the current Phase 0 Map36 runtime proof.

## Status

**Idea / research track only.**

Confirmed today:

- WorldOverlay's server-side architecture does not require modern Retail phasing.
- Tortoise already has relevant server-side map/instance mechanisms, including continent instancing infrastructure.

Not yet confirmed:

- what terrain/map/phase switching capabilities the Turtle 1.8 client exposes;
- whether those capabilities can be reused cleanly by WorldOverlay;
- the exact runtime/resource cost of continent-scale overlay sets.

These questions require a dedicated Turtle client audit before implementation decisions are made.
