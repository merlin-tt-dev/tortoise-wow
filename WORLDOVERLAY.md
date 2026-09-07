# module-worldoverlay

`module-worldoverlay` is the dedicated maintenance branch for the WorldOverlay module.

Its target form is an independently buildable WorldOverlay layer on top of `dev-clean`, without PlayerBots or unrelated module history. WorldOverlay-specific Core hooks and module changes belong to this branch; generic Core maintenance belongs to `dev-clean`.

## Purpose

WorldOverlay provides the server-side infrastructure for named runtime overlays and WorldRouting while keeping logical overlay identity separate from ephemeral runtime instance IDs.

This file is the branch-level maintenance contract. Detailed module behavior, architecture, commands, roadmap, and prior-art analysis live with the module itself and should not be duplicated here.

## Target branch relationship

```text
1181dev
   |
   +-- dev-clean
          |
          +-- module-worldoverlay
          +-- module-playerbots
          +-- module-<other>

verified module branches
          |
          +--> mods-integration
```

`module-worldoverlay` should ultimately be reproducible from `dev-clean` plus only the WorldOverlay-owned patch series.

## Patch ownership

The intended WorldOverlay patch namespaces are:

```text
patches/mod-worldoverlay/core/
patches/mod-worldoverlay/mod/
```

- `patches/mod-worldoverlay/core/` contains Core changes that exist specifically because WorldOverlay requires them.
- `patches/mod-worldoverlay/mod/` contains the WorldOverlay module itself and module-local fixes, features, SQL, documentation, examples, and build integration.

Generic Core fixes belong in:

```text
patches/core/
```

and should be carried by `dev-clean` instead of being duplicated in the WorldOverlay namespace.

## Current migration note

The historical `module-worldoverlay` branch was developed while PlayerBots was also present in the working tree and full-build audits. The new maintenance model deliberately separates those concerns.

During cleanup, the branch should be reconstructed or rebased so that:

```text
dev-clean
   + patches/mod-worldoverlay/core/*
   + patches/mod-worldoverlay/mod/*
   -> module-worldoverlay
```

PlayerBots code, PlayerBots-specific Core hooks, and PlayerBots patch history do not belong in the final WorldOverlay branch. They belong in `module-playerbots` and are combined with WorldOverlay only in `mods-integration`.

This separation allows failures to be classified cleanly:

```text
dev-clean fails                     -> generic Core baseline
module-worldoverlay fails           -> WorldOverlay or its Core requirements
module-playerbots fails             -> PlayerBots or its Core requirements
both pass separately,
integration fails                   -> cross-module interaction
```

## Module documentation

The WorldOverlay module has dedicated documentation and that documentation remains the source of truth for module semantics:

- [`modules/mod-worldoverlay/README.md`](modules/mod-worldoverlay/README.md) — module overview and current behavior
- [`modules/mod-worldoverlay/docs/ARCHITECTURE.md`](modules/mod-worldoverlay/docs/ARCHITECTURE.md) — architecture and subsystem boundaries
- [`modules/mod-worldoverlay/docs/ROADMAP.md`](modules/mod-worldoverlay/docs/ROADMAP.md) — implementation phases and gates
- [`modules/mod-worldoverlay/docs/COMMANDS.md`](modules/mod-worldoverlay/docs/COMMANDS.md) — developer/runtime commands
- [`modules/mod-worldoverlay/docs/PRIOR_ART.md`](modules/mod-worldoverlay/docs/PRIOR_ART.md) — prior-art and design research
- [`modules/mod-worldoverlay/docs/TURTLE_CLIENT_OVERLAY_IDEAS.md`](modules/mod-worldoverlay/docs/TURTLE_CLIENT_OVERLAY_IDEAS.md) — client-side and future overlay ideas

Repository-wide branch and patch ownership are documented in:

- [`DEV-CLEAN.md`](DEV-CLEAN.md)
- [`patches/README.md`](patches/README.md)

When this branch is rebased onto the new `dev-clean` lineage, those repository-level files become inherited documentation rather than module-owned content.

## Maintenance rules

- Derive the clean WorldOverlay branch from `dev-clean`, not from PlayerBots or another module branch.
- Keep WorldOverlay-specific Core hooks in `patches/mod-worldoverlay/core/`.
- Keep WorldOverlay module changes in `patches/mod-worldoverlay/mod/`.
- Promote a Core fix to `patches/core/` only when it is genuinely module-independent.
- Do not place PlayerBots-specific changes in this branch.
- Keep patch ordering deterministic and changes independently reviewable.
- Treat runtime IDs as implementation details; persistent overlay identity is logical/module-owned state.
- Preserve architectural separation between WorldOverlay runtime management and WorldRouting destination/teleport logic.
- Keep existing unrelated custom portals or gameplay systems outside WorldOverlay unless they are intentionally migrated through a separate reviewed change.
- Build and test WorldOverlay independently before combining it with other modules.
- Put true cross-module repairs in `patches/integration/`, not in the WorldOverlay patch namespace.

## Validation philosophy

A WorldOverlay branch is not considered proven merely because its module target compiles. The intended progression is:

```text
clean Core build
    -> WorldOverlay-only build
    -> WorldOverlay runtime proof
    -> independent module validation
    -> mods-integration regression test
```

The detailed runtime proof and current phase gates remain documented in the module README and roadmap.

The objective is a WorldOverlay branch whose history describes WorldOverlay itself, not the accidental presence of other modules during development.