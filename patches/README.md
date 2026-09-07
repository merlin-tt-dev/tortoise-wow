# Patch Ownership and Branch Layout

This directory is the source of truth for maintained patch ownership across the repository.
The goal is to keep generic Core maintenance, module-specific Core requirements, module code,
and cross-module integration changes clearly separated so that each layer can be updated,
audited, rebuilt, and tested independently.

## Branch model

```text
1181dev
   |
   +-- dev-clean
          |
          +-- module-<name>
          +-- module-<other>
          +-- ...

module-* branches
       \
        +--> mods-integration
```

### `1181dev`

Upstream/development base. It is not used as a workspace for local patch maintenance.

### `dev-clean`

Reusable clean Core baseline derived from `1181dev`.

Only generic, module-independent Core maintenance belongs here. A change is suitable for
`dev-clean` only when it is useful and correct without requiring any particular module.

### `module-<name>`

Each module branch is derived independently from `dev-clean` and owns only its module-specific
patch namespace.

A module may require both changes inside the Core and changes inside the module itself. Those
are intentionally kept separate, but both remain owned by that module unless the Core change
is proven to be generally useful and is deliberately promoted to `patches/core/`.

Module branches should not accumulate patches owned by other modules.

### `mods-integration`

Integration branch used to combine independently maintained module branches and test their
interaction.

It should not become the primary home of module features or ordinary Core fixes. Changes that
exist only because multiple otherwise-independent modules interact belong to the dedicated
integration patch namespace.

## Patch directory layout

```text
patches/
├── core/
│   └── ...
├── mod-<name>/
│   ├── core/
│   │   └── ...
│   └── mod/
│       └── ...
└── integration/
    └── ...
```

### `patches/core/`

Generic Core fixes and maintenance.

These patches must not depend on a particular module and form the maintained patch set for
`dev-clean`.

Typical examples include compiler compatibility, self-contained headers, build-system fixes,
and generally useful Core corrections.

### `patches/mod-<name>/core/`

Core changes required by exactly one module or primarily owned by that module.

The code touched by these patches may live in Core, but ownership remains with the module.
They are therefore not automatically part of `dev-clean`.

### `patches/mod-<name>/mod/`

Changes to the module itself: ports, fixes, refactors, features, configuration work, and other
module-local maintenance.

### `patches/integration/`

Changes required specifically by the combination of independently working modules.

An integration patch should be used only when the problem does not correctly belong to one
module or to the generic Core baseline.

## Ownership rule

Classify a patch by *why the change exists*, not merely by which source file it touches.

```text
Generic Core problem                    -> patches/core/
Module-specific change inside Core      -> patches/mod-<name>/core/
Change inside the module                -> patches/mod-<name>/mod/
Cross-module interaction only           -> patches/integration/
```

A module-specific Core patch may later be promoted to `patches/core/` when it has been reviewed
and shown to be genuinely module-independent. Promotion should be explicit rather than happening
implicitly through branch history.

## Update workflow

The intended maintenance flow is:

1. Update or select the desired `1181dev` base revision.
2. Build `dev-clean` from that base using only the generic Core patch set.
3. Derive each `module-<name>` branch independently from `dev-clean`.
4. Apply that module's `core/` patches and then its `mod/` patches.
5. Verify each module branch independently.
6. Combine verified module branches in `mods-integration`.
7. Keep any truly integration-only repair in `patches/integration/`.

This makes failures attributable to a specific layer:

```text
dev-clean fails                 -> generic Core baseline
module branch fails             -> that module or its Core requirements
modules pass separately,
integration fails               -> cross-module interaction
```

## Patch hygiene

- Keep patches minimal and single-purpose where practical.
- Keep application order deterministic; numbered filenames are preferred where ordering matters.
- Do not hide module-specific dependencies in the generic Core patch set.
- Do not copy another module's patches into a module namespace merely to make a branch build.
- Preserve a patch as an auditable artifact even when its change is temporarily materialized in a working branch.
- Validate patches before committing them and keep patch files usable with normal Git tooling.
- Prefer structural/refactoring changes separately from behavioral changes so regressions remain attributable.

The objective is a reusable Core baseline, independently maintainable modules, and an integration
branch whose failures describe real module interaction rather than accumulated branch history.
