# WorldOverlay command draft

Primary namespace:

```text
.wo
```

Long alias:

```text
.woverlay
```

All commands in this document are a design contract, not yet implemented. Commands are grouped by roadmap phase; later sections must not be interpreted as v1 implementation requirements.

## General

```text
.wo list
.wo info
.wo where
.wo reload [overlay-key]
.wo validate <overlay-key>
```

`list` shows configured overlays.

`info` shows the current overlay context, map id, allocation scope, active variant and current runtime instance id when the player is inside a named overlay.

`where` is the concise runtime diagnostic form.

`reload` reloads module-owned definitions/content without reloading ordinary base-world spawns.

`validate` should eventually check at least:

- map id exists and is suitable for the configured allocation mode;
- destination coordinates are valid;
- destination overlay keys exist;
- spawn template entries exist;
- no duplicate logical keys exist;
- runtime map association is internally consistent;
- referenced variants/groups/pools/waypoints exist when those systems are enabled.

Suggested `where` output:

```text
WorldOverlay: tele_city
Map: 36
Allocation: SINGLETON
Runtime instance: 417
Variant: default
Base spawns: INHERIT
Overlay GO spawns: 42
Overlay creature spawns: 18
```

## Overlay lifecycle

Initial form:

```text
.wo create <overlay-key> <map-id> [inherit|none]
.wo enable <overlay-key>
.wo disable <overlay-key>
.wo enter <overlay-key> [destination-key]
.wo leave
.wo runtime <overlay-key>
```

Phase 1 should accept `inherit` only. `none` is reserved until instance-specific base-spawn suppression is implemented and verified.

`enter` resolves the named overlay to its current runtime instance, creating it when required.

`leave` returns through the module's saved entry point when available. Exact persistence semantics for the return point are deferred until runtime implementation.

Longer-term allocation-aware creation may become:

```text
.wo create <overlay-key> <map-id> --allocation singleton
.wo create <overlay-key> <map-id> --allocation per-player
.wo create <overlay-key> <map-id> --allocation per-group
.wo create <overlay-key> <map-id> --allocation ephemeral
```

The exact parser syntax should follow existing Tortoise command conventions rather than forcing GNU-style options if those are unnatural for the core command system.

## Destinations

```text
.wo dest list [overlay-key]
.wo dest add <destination-key>
.wo dest delete <destination-key>
.wo dest goto <destination-key>
```

When `dest add` is executed inside a named overlay, it stores the player's current map, coordinates, orientation and overlay key with instance policy `OVERLAY`.

Outside an overlay, the command should require an explicit policy instead of guessing.

Destinations remain logical and must never persist the current runtime instance id as their identity.

## GameObject builder

```text
.wo go add <entry>
.wo go move
.wo go turn
.wo go delete
.wo go info
```

Builder invariants:

- Requires an active named overlay context.
- Writes only to `worldoverlay_gameobject` or its future normalized replacement.
- Never updates ordinary `gameobject` rows.
- `delete`, `move` and `turn` must reject selected objects not owned by WorldOverlay.
- Runtime changes should be reflected immediately in the active overlay and persisted to the module table.

The exact selection mechanism should follow the existing Tortoise GM object-selection conventions where possible rather than inventing a second incompatible targeting model.

Future per-spawn override commands may include:

```text
.wo go scale <value>
.wo go enable
.wo go disable
```

Any override must remain spawn-local and must not mutate `gameobject_template` globally.

## Creature builder

```text
.wo npc add <entry>
.wo npc move
.wo npc turn
.wo npc delete
.wo npc info
```

Builder invariants mirror the GO commands:

- Requires an active overlay context.
- Writes only to `worldoverlay_creature` or its future normalized replacement.
- Never updates ordinary `creature` rows.
- Destructive operations reject non-overlay creatures.

Future per-spawn overrides may include:

```text
.wo npc scale <value>
.wo npc faction <id>
.wo npc display <id>
.wo npc emote <id>
.wo npc enable
.wo npc disable
```

These operations must not rewrite the global creature template.

## Teleport bindings

```text
.wo bind item <item-entry> <destination-key>
.wo bind go <gameobject-entry> <destination-key>
.wo bind npc <creature-entry> <destination-key>
.wo unbind item <item-entry>
.wo unbind go <gameobject-entry>
.wo unbind npc <creature-entry>
```

These commands manage the simple one-source/one-destination binding model.

A future gossip/menu system should use a separate multi-option model rather than weakening this table's semantics.

## Overlay allocation commands - planned

These commands belong to the reusable-template phase, not the first runtime spike.

Possible administrative surface:

```text
.wo allocation show <overlay-key>
.wo allocation set <overlay-key> singleton
.wo allocation set <overlay-key> per-player
.wo allocation set <overlay-key> per-group
.wo allocation set <overlay-key> ephemeral
```

Runtime diagnostics may additionally accept an owner/scope key when an overlay can have multiple active instances.

The persistent identity is the logical overlay plus allocation scope, never a runtime instance id.

## State commands - planned

Overlay logical state provides InstanceScript-like data without embedding every state transition in one hard-coded instance script.

```text
.wo state list [overlay-key]
.wo state get <key>
.wo state set <key> <value>
.wo state unset <key>
```

Examples:

```text
.wo state set market_active 1
.wo state set story_stage 3
.wo state set tower_door_open 0
```

The final implementation should define state scope explicitly: overlay-wide, allocation-instance-wide, or player-view-specific. It must not silently mix those scopes.

## Content variants / phases - planned

Content variants switch server-side presentation while keeping the same client geometry.

```text
.wo variant list [overlay-key]
.wo variant create <variant-key>
.wo variant set <variant-key>
.wo variant info
.wo variant delete <variant-key>
```

Example:

```text
tele_city/default
tele_city/halloween
tele_city/attacked
```

Variants can later control overlay NPCs, GOs, groups, triggers and scripted behavior.

This is a server-side content-layer concept. It must not be documented or implemented as if the 2.4.3 client supported modern Retail PhaseShift/TerrainSwap features.

## Trigger volume builder - planned

Trigger volumes provide server-side proximity/area behavior without requiring a custom client spell or visible GO.

Initial geometry types:

```text
SPHERE
BOX
```

Possible builder surface:

```text
.wo trigger list
.wo trigger add sphere <trigger-key> <radius>
.wo trigger add box <trigger-key> <x> <y> <z> [orientation]
.wo trigger info <trigger-key>
.wo trigger delete <trigger-key>
```

Action binding examples:

```text
.wo trigger action <trigger-key> teleport <destination-key>
.wo trigger action <trigger-key> set-state <key> <value>
.wo trigger action <trigger-key> set-variant <variant-key>
.wo trigger action <trigger-key> enable-group <group-key>
.wo trigger action <trigger-key> disable-group <group-key>
```

One trigger may eventually support multiple ordered actions, but v1 of the trigger subsystem should begin with a deliberately small model.

## World anchors - planned

Named anchors define canonical positions independently of arbitrary runtime instance ids.

```text
.wo anchor list
.wo anchor set entry
.wo anchor set exit
.wo anchor set graveyard
.wo anchor set fallback
.wo anchor goto <anchor-key>
```

Anchors may later participate in death handling, return behavior and safe fallback recovery.

## Waypoint builder - planned

Overlay NPC movement should remain overlay-owned rather than writing paths into unrelated global world tables.

Possible commands:

```text
.wo npc wp start
.wo npc wp add
.wo npc wp delete
.wo npc wp show
.wo npc wp clear
```

The command behavior should mirror familiar Tortoise/MaNGOS waypoint editing conventions where practical.

## Spawn groups - planned

Groups allow multiple NPC/GO spawns to be enabled or disabled as one logical unit.

```text
.wo group list
.wo group create <group-key>
.wo group add-selected <group-key>
.wo group remove-selected <group-key>
.wo group enable <group-key>
.wo group disable <group-key>
.wo group delete <group-key>
```

A group can later be driven by state, variant, schedule or trigger actions.

## Pools - planned

Pools provide controlled randomized selection among overlay content.

Possible administrative surface:

```text
.wo pool list
.wo pool create <pool-key>
.wo pool add-selected <pool-key> <weight>
.wo pool remove-selected <pool-key>
.wo pool reroll <pool-key>
.wo pool info <pool-key>
```

Pool behavior should align with established server pool semantics where that reduces duplication and avoids surprising content authors.

## Formations - later

Creature formations are a later extension for patrols, guards and grouped movement.

Possible commands are intentionally not fixed yet; formation behavior should be designed only after waypoint and group semantics are stable.

## Access rules - later

Potential administrative commands:

```text
.wo access list <overlay-key>
.wo access add <overlay-key> <rule...>
.wo access delete <overlay-key> <rule-id>
```

Potential rule inputs include security level, faction/team, level, quest state, item, group status, owner scope or script predicate.

The final grammar should use structured rule types rather than arbitrary SQL-like expressions entered in chat.

## Clone/export/diff tooling - later

```text
.wo clone <source-overlay> <target-overlay>
.wo export <overlay-key>
.wo diff <overlay-a> <overlay-b>
```

`clone` duplicates logical module-owned content and definitions. It never clones a live runtime instance id.

`export` should eventually produce reproducible module SQL/data suitable for review and version control.

`diff` should compare logical overlay definitions/content, not volatile runtime state unless explicitly requested.

## Permissions

Initial implementation should place mutating builder/lifecycle commands at developer-level security or higher.

Read-only diagnostics may be administrator-level if desired, but permissions must be explicit in the final command table.

Future player-facing interactions such as Chronostone use, NPC gossip, guild/player housing entry or scripted variant switching are not GM commands and should use dedicated script/access paths.

## Alias policy

`.wo` is the canonical short form used in documentation and examples.

`.woverlay` should resolve to the same command table as a discoverable long alias, not a separate implementation.

## Implementation order

The existence of a command in this draft does not determine implementation priority. Runtime work must follow [`ROADMAP.md`](ROADMAP.md): instance lifecycle proof first, then singleton overlay runtime, builder, allocation policies, state/variants/triggers, and finally richer dynamic-world tooling.
