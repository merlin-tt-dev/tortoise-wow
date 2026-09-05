# WorldOverlay command draft

Primary namespace:

```text
.wo
```

Long alias:

```text
.woverlay
```

All commands in this document are a design contract, not yet implemented.

## General

```text
.wo list
.wo info
.wo reload [overlay-key]
```

`list` shows configured overlays.

`info` shows the current overlay context, map id and current runtime instance id when the player is inside a named overlay.

`reload` reloads module-owned definitions/content without reloading ordinary base-world spawns.

## Overlay lifecycle

```text
.wo create <overlay-key> <map-id> [inherit|none]
.wo enable <overlay-key>
.wo disable <overlay-key>
.wo enter <overlay-key> [destination-key]
.wo leave
```

Phase 1 should accept `inherit` only. `none` is reserved until instance-specific base-spawn suppression is implemented and verified.

`enter` resolves the named overlay to its current runtime instance, creating it when required.

`leave` returns through the module's saved entry point when available. Exact persistence semantics for the return point are deferred until runtime implementation.

## Destinations

```text
.wo dest list [overlay-key]
.wo dest add <destination-key>
.wo dest delete <destination-key>
.wo dest goto <destination-key>
```

When `dest add` is executed inside a named overlay, it stores the player's current map, coordinates, orientation and overlay key with instance policy `OVERLAY`.

Outside an overlay, the command should require an explicit policy instead of guessing.

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
- Writes only to `worldoverlay_gameobject`.
- Never updates ordinary `gameobject` rows.
- `delete`, `move` and `turn` must reject selected objects not owned by WorldOverlay.
- Runtime changes should be reflected immediately in the active overlay and persisted to the module table.

The exact selection mechanism should follow the existing Tortoise GM object-selection conventions where possible rather than inventing a second incompatible targeting model.

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
- Writes only to `worldoverlay_creature`.
- Never updates ordinary `creature` rows.
- Destructive operations reject non-overlay creatures.

## Teleport bindings

```text
.wo bind item <item-entry> <destination-key>
.wo bind go <gameobject-entry> <destination-key>
.wo bind npc <creature-entry> <destination-key>
.wo unbind item <item-entry>
.wo unbind go <gameobject-entry>
.wo unbind npc <creature-entry>
```

These commands manage the simple one-source/one-destination table `worldoverlay_teleport_binding`.

A future gossip/menu system should use a separate multi-option model rather than weakening this table's semantics.

## Diagnostic commands

```text
.wo where
.wo runtime <overlay-key>
.wo validate <overlay-key>
```

Suggested output for `where`:

```text
WorldOverlay: tele_city
Map: 36
Runtime instance: 417
Base spawns: INHERIT
Overlay GO spawns: 42
Overlay creature spawns: 18
```

`validate` should eventually check at least:

- map id exists and is instanceable for named-instance mode;
- destination coordinates are valid;
- destination overlay keys exist;
- spawn template entries exist;
- no duplicate logical keys;
- runtime map association is internally consistent.

## Permissions

Initial implementation should place mutating builder/lifecycle commands at developer-level security or higher.

Read-only diagnostics may be administrator-level if desired, but permissions must be explicit in the final command table.

## Alias policy

`.wo` is the canonical short form used in documentation and examples.

`.woverlay` should resolve to the same command table as a discoverable long alias, not a separate implementation.
