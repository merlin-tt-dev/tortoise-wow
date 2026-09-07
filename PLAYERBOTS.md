# module-playerbots

`module-playerbots` is the dedicated maintenance branch for the PlayerBots module.

It is derived from `dev-clean` and must remain independent from other module branches. The branch owns PlayerBots-specific Core hooks and PlayerBots module changes, but it must not absorb unrelated Core maintenance or changes owned by other modules.

## Purpose

Use this branch to maintain PlayerBots as an independently buildable and reviewable module layer on top of the clean Core baseline.

The intended patch namespaces are:

```text
patches/mod-playerbots/core/
patches/mod-playerbots/mod/
```

- `patches/mod-playerbots/core/` contains Core changes that exist specifically because PlayerBots requires them.
- `patches/mod-playerbots/mod/` contains PlayerBots module changes, ports, fixes, refactors, configuration work, and features.

Generic Core fixes do not belong here; they belong in `patches/core/` and should be carried by `dev-clean`.

## Branch relationship

```text
1181dev
   |
   +-- dev-clean
          |
          +-- module-playerbots
          +-- module-worldoverlay
          +-- module-<other>

verified module branches
          |
          +--> mods-integration
```

`module-playerbots` should be rebuildable from `dev-clean` plus its own ordered patch series.

## Source and maintenance boundary

`livingbots/playerbots-clean-base` is treated as a historical/reference source for the PlayerBots port. It is not the maintenance target for the new patch-owned branch structure and should remain unchanged during this cleanup.

The target maintenance model is:

```text
dev-clean
   + patches/mod-playerbots/core/*
   + patches/mod-playerbots/mod/*
   -> module-playerbots
```

This makes PlayerBots-specific dependencies explicit and prevents generic Core maintenance from becoming entangled with the module history.

## Configuration cleanup

Configuration cleanup belongs to the PlayerBots module layer. Structural cleanup should preserve existing keys, defaults, and load/override semantics unless a separate behavioral change is intentional and documented.

Large configuration files may be split by domain, for example:

```text
Core / General
AI / Behavior
Combat
Movement / Travel
Loot / Economy
Group / Raid
PvP
Performance / Limits
Debug / Logging
```

Do structural separation first; remove or rename options only in separate, reviewable changes.

## Documentation

Repository-wide patch ownership and directory layout are documented in:

- [`DEV-CLEAN.md`](DEV-CLEAN.md)
- [`patches/README.md`](patches/README.md)
- [`modules/README.md`](modules/README.md) for the repository's generic module system

The historical PlayerBots module tree currently does not provide a dedicated module-specific README. As the new PlayerBots patch series is reconstructed, module-specific documentation should live with the module and this file should remain the branch-level maintenance contract.

## Rules

- Derive PlayerBots work from `dev-clean`, not from another module branch.
- Keep generic Core fixes in `patches/core/`.
- Keep PlayerBots-only Core requirements in `patches/mod-playerbots/core/`.
- Keep PlayerBots module changes in `patches/mod-playerbots/mod/`.
- Do not copy WorldOverlay or other module patches into this branch.
- Keep patch ordering deterministic and patches individually reviewable.
- Validate every patch before committing it.
- Build and test PlayerBots independently before combining it with other modules.
- Put true cross-module repairs in `patches/integration/`, not in the PlayerBots patch namespace.

The objective is that a PlayerBots failure can be attributed to PlayerBots or its explicit Core requirements, while a failure that appears only after combining independently working modules can be attributed to integration.