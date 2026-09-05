# WorldOverlay prior art and design notes

This document records the systems reviewed while defining `mod-worldoverlay` and the architectural lessons taken from them. It is not a claim that any one existing emulator feature already implements WorldOverlay as designed here.

## Summary

No reviewed system was found that combines all of the following into one generic Tortoise module:

```text
named logical map instance
+ reusable instance allocation policy
+ overlay-owned spawns
+ overlay state and content variants
+ server-side triggers
+ overlay teleport resolution
+ in-game overlay builder
+ normal dungeon compatibility
```

However, the individual ideas are well established in existing WoW server architecture. WorldOverlay is primarily a composition and generalization of those proven concepts.

## 1. Tortoise / MaNGOS instance identity

Tortoise already separates map copies by:

```text
(mapId, instanceId)
```

`MapManager::FindMap(mapId, instanceId)` exposes that identity directly.

Normal dungeon instance creation is bind-driven. A player/group persistent state is preferred; if none applies, the core generates a fresh instance id.

### WorldOverlay lesson

Do not replace the normal dungeon mechanism. Add a second logical resolution path:

```text
normal portal
  -> ordinary bind-driven dungeon instance

WorldOverlay destination
  -> overlay key
  -> overlay allocation scope
  -> exact runtime instance
```

The two paths must coexist.

## 2. Tortoise MapPersistentState

The existing Tortoise `MapPersistentState` is especially relevant.

It already stores per-map-copy information including:

- creature respawn times;
- GameObject respawn times;
- pool spawn state (`SpawnedPoolData`);
- dynamic grid object GUID sets for creatures and GameObjects.

The source comments explicitly describe the grid object data as map-copy-specific dynamic spawn data, including pool-driven spawns.

A persistent state can also exist while the corresponding `Map` object is not loaded.

### WorldOverlay lesson

Per-instance content/state is not alien to the core. Tortoise already has map-copy-specific runtime state and dynamic grid membership.

WorldOverlay should therefore integrate with those lifecycle concepts instead of trying to build an unrelated second world simulation.

The important distinction is persistence identity:

```text
Tortoise runtime state: map / runtime instance
WorldOverlay content:   logical overlay / allocation scope
```

The module resolves the latter to the former at runtime.

## 3. Dungeon InstanceScript-style state

WoW emulator dungeon scripts commonly expose per-instance logical data through `SetData` / `GetData` style interfaces.

This is used for encounter state, doors, scripted progression and other instance-local logic.

### WorldOverlay lesson

A generic overlay state store is a natural extension of an established pattern:

```text
market_active
tower_door_open
story_stage
season
```

Unlike a dungeon-specific script, WorldOverlay state should be data-driven and explicitly scoped.

Potential state scopes must remain distinct:

```text
OVERLAY       - shared definition/world state
ALLOCATION    - one concrete singleton/player/group runtime world
PLAYER_VIEW   - player-specific content presentation, if later supported
```

Scope must never be inferred ambiguously.

## 4. TrinityCore GarrisonMap / owner-bound maps

Modern TrinityCore contains a dedicated `GarrisonMap` path where a map is created with a map id, runtime instance id and an owner GUID.

This is a strong precedent for the broader idea that an instanced map copy can represent a logical owned world rather than only a traditional dungeon run.

### WorldOverlay lesson

Allocation policies are worth making first-class:

```text
SINGLETON
PER_PLAYER
PER_GROUP
EPHEMERAL
MANUAL
```

A logical instance can therefore be addressed by:

```text
(overlay_key, owner_scope)
```

rather than by a volatile runtime id.

WorldOverlay does not copy the Garrison subsystem; it borrows the architectural idea that a map instance can have a stable logical owner/purpose.

## 5. AzerothCore mod-guildhouse

AzerothCore's guild-house module provides private/shared guild spaces on existing world geometry and allows guilds to acquire functional additions such as trainers, vendors, portals, banks, auctioneers and other NPCs.

The reviewed module uses phasing rather than the exact named-instance model proposed here.

### WorldOverlay lesson

The player-facing concept is validated: existing geometry can host independently managed logical spaces with custom content.

WorldOverlay intentionally prefers real instance separation when using instanced maps:

```text
Map 36 / normal instance
!=
Map 36 / tele_city
!=
Map 36 / guild_hall
```

This avoids relying on one visibility mask to isolate all world state.

## 6. Waypoint systems

Existing emulator waypoint systems demonstrate that NPC movement can be described and edited as persistent data rather than hard-coded into every script.

### WorldOverlay lesson

Overlay NPCs should eventually own overlay-specific waypoint paths rather than writing their movement into unrelated global path data.

Conceptually:

```text
worldoverlay_waypoint_path
worldoverlay_waypoint
```

with an in-game builder that follows familiar GM waypoint workflows.

## 7. Spawn groups

Modern emulator databases commonly support logical spawn groups that enable/disable sets of creature and GameObject spawns together.

### WorldOverlay lesson

WorldOverlay should support the same abstraction because it composes naturally with state and variants:

```text
market_active = 1
  -> enable group tele_city_market

variant = attacked
  -> disable group peaceful_guards
  -> enable group invasion_attackers
```

A spawn group is a content-composition mechanism, not a replacement for creature/GO templates.

## 8. Pools

Pool systems provide controlled randomized activation of spawn alternatives and are already established in WoW emulators.

Tortoise `MapPersistentState` already carries per-map-copy pool spawn state.

### WorldOverlay lesson

Overlay pools should reuse compatible pool semantics where possible:

```text
daily_vendor
|- vendor A : 30
|- vendor B : 30
`- vendor C : 40
```

The module should avoid inventing incompatible random-spawn semantics unless the core pool APIs cannot support overlay-owned content safely.

## 9. Creature formations

Creature formation systems group creatures for coordinated movement/behavior.

### WorldOverlay lesson

Formations are useful but not foundational. They should come after overlay waypoint and group semantics are stable.

## 10. Retail Zidormi / content phasing

Retail WoW uses NPCs such as Zidormi to let a player move between logical versions of a place.

At the player-facing level this can look like:

```text
speak to NPC
-> world changes to old/new version
```

Modern Retail can combine several technologies:

- player phase state controlling which NPCs/GameObjects are visible;
- alternate visible-map/terrain presentation for zones whose actual terrain changed;
- phase-aware teleport destinations and scripts.

### Important TBC distinction

The 2.4.3 client must not be assumed to contain modern Retail PhaseShift / visible-map / terrain-swap infrastructure.

WorldOverlay therefore separates two ideas:

```text
CONTENT VARIANT
same client geometry
+ different server-side NPCs/GOs/state/triggers

MAP/INSTANCE VARIANT
separate named runtime world
+ geometry already available to the client for that map
```

If genuinely new terrain/assets do not exist in the client, WorldOverlay cannot manufacture them server-side; a client patch would still be required for that specific case.

### WorldOverlay lesson

The useful part of the Zidormi concept for Tortoise is the logical world-version switch, not the modern implementation details.

A WorldOverlay NPC could eventually perform:

```text
"Show me Tele City before the attack"
  -> set content variant: peaceful

or

"Take me to the old version"
  -> resolve another named overlay/map instance
```

The player-facing experience can be similar while the server implementation remains compatible with TBC-era client capabilities.

## 11. AreaTrigger geometry as a trigger-model precedent

Tortoise world data already represents AreaTrigger geometry using radius and box dimensions/orientation.

That gives WorldOverlay a useful precedent for server-side trigger volumes:

```text
SPHERE
BOX
```

### WorldOverlay lesson

A generic overlay trigger manager can provide invisible proximity behavior without requiring a visible GameObject, a new client-known spell or a repurposed native teleport spell.

Potential trigger actions include:

```text
TELEPORT
SET_STATE
SET_VARIANT
ENABLE_GROUP
DISABLE_GROUP
RUN_SCRIPT
MESSAGE
```

This should be implemented as module-owned logic rather than by overloading normal dungeon AreaTriggers.

## 12. What WorldOverlay should not duplicate

The module should not become a second copy of the world database.

Normal templates should remain authoritative where possible:

```text
creature_template
gameobject_template
item_template
quest_template
vendor/trainer data
```

WorldOverlay should store:

- logical ownership;
- placement;
- allocation scope;
- state/variant membership;
- trigger/group/pool membership;
- per-spawn overrides where required.

Reference templates; do not fork them wholesale.

## 13. Combined architecture

The reviewed precedents combine into the following long-term model:

```text
Client map geometry
        |
        v
Normal Tortoise map/instance system
        |
        +----------------------------+
        |                            |
 normal dungeon path          WorldOverlay path
 player/group binds           overlay key + scope
        |                            |
        v                            v
 ordinary instance            named runtime instance
                                     |
                                     +-- overlay spawns
                                     +-- state
                                     +-- variants
                                     +-- triggers
                                     +-- waypoints
                                     +-- groups/pools
                                     +-- anchors
                                     `-- teleport destinations
```

The key architectural novelty is not any individual mechanism. It is making the mechanisms generic, named, data-driven and reusable across existing instanced maps while preserving the original world in parallel.

## 14. Research cautions

Before implementation, every borrowed concept must be checked against the exact Tortoise branch rather than copied from TrinityCore/AzerothCore APIs verbatim.

In particular:

- modern TrinityCore map APIs are not assumed to exist in Tortoise;
- modern Retail phasing packets are not assumed to exist in the 2.4.3 client;
- existing Tortoise pool/grid APIs must be audited before overlay-owned spawn integration;
- a dedicated dungeon instance must not be reached by corrupting normal player/group binds;
- `CreateTestMap()` remains a test/debug API until proven otherwise;
- instance transfer must respect thread safety, map lifecycle and player transport/pet/state handling.

The roadmap therefore keeps the exact instance-allocation/transfer spike as Phase 0.
