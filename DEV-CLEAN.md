# dev-clean

`dev-clean` is the maintained, reusable Core baseline for this repository.

It is derived from `1181dev` and exists to keep generic Core maintenance separate from module-specific work. Module branches should be built independently on top of this branch instead of accumulating unrelated module history in one long-lived branch.

## Purpose

`dev-clean` contains only changes that are valid and useful for the Core without requiring a particular module.

Typical examples:

- compiler and toolchain compatibility fixes
- self-contained header fixes
- generic build-system corrections
- generally useful Core bug fixes
- maintenance required to keep the base buildable on supported development systems

The corresponding maintained patch artifacts belong in:

```text
patches/core/
```

The patch files are the preferred auditable description of these changes. Changes may be materialized in the branch for normal development and builds, but the patch series should remain reproducible and independently reviewable.

## Branch relationship

```text
1181dev
   |
   +-- dev-clean
          |
          +-- module-worldoverlay
          +-- module-playerbots
          +-- module-<other>

module-* branches
       \
        +--> mods-integration
```

### `1181dev`

Upstream/development base. Do not use it as the workspace for local Core or module patch maintenance.

### `dev-clean`

Clean reusable Core baseline. Only generic Core changes belong here.

### `module-<name>`

Each module branch is derived independently from `dev-clean` and owns only its own module patch namespace.

Module-specific Core requirements belong under:

```text
patches/mod-<name>/core/
```

Module-local changes belong under:

```text
patches/mod-<name>/mod/
```

These changes do not belong in `dev-clean` merely because they touch Core files.

### `mods-integration`

Combines independently maintained module branches for integration and regression testing.

A failure that appears only after otherwise-working modules are combined is an integration problem and should not be hidden inside one module's ordinary patch series.

Integration-only patches belong under:

```text
patches/integration/
```

## Ownership rule

Classify a patch by why the change exists, not only by which source file it touches.

```text
Generic Core problem                    -> patches/core/
Module-specific change inside Core      -> patches/mod-<name>/core/
Change inside a module                  -> patches/mod-<name>/mod/
Cross-module interaction only           -> patches/integration/
```

A module-specific Core patch may later be promoted into `patches/core/`, but only after it has been reviewed and shown to be genuinely module-independent.

## Maintenance workflow

1. Select or update the desired `1181dev` revision.
2. Rebase or rebuild `dev-clean` from that revision.
3. Apply and validate the ordered patch series from `patches/core/`.
4. Build and test `dev-clean` independently.
5. Derive or update each `module-<name>` branch from `dev-clean`.
6. Apply only that module's `core/` and `mod/` patch series.
7. Build and test each module independently.
8. Merge verified module branches into `mods-integration` for combined testing.

This keeps failures attributable to a specific layer:

```text
dev-clean fails                 -> generic Core baseline
module branch fails             -> that module or its Core requirements
modules pass separately,
integration fails               -> cross-module interaction
```

## Patch discipline

- Prefer one clear reason per patch.
- Keep patch ordering deterministic; numbered filenames are preferred when order matters.
- Keep generic Core fixes out of module namespaces once they are intentionally promoted to Core.
- Keep module-specific Core hooks out of `patches/core/` unless they are genuinely generic.
- Preserve patch artifacts even when their changes are materialized in a working branch.
- Validate patches with normal Git tooling before committing them.
- Keep structural/refactoring changes separate from behavioral changes where practical.

See `patches/README.md` for the repository-wide patch ownership model and directory layout.
