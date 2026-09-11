# Core Optimization Patchset 204-223

This bundle continues the `audits/dev-performance` deep/performance audit from the exact user-supplied Patch-203 archive.

## Baseline

- Source-of-Truth archive: `tortoise-wow-audits-dev-performance-patch203.tar.xz`
- Archive SHA256: `12d97c57b551f0251cf5421ef072797068acbc0172db4fd2583b22d46565c891`
- Baseline Git tree used for validation: `a94d011979f4232ec00723e5125ce2d3a3c2f574`
- Audited final Git tree after 204-223: `f29d40a6c3fcfd88c1064c1f7b252780be93ca8d`

The baseline already contains the user's battleground/arena world-thread ownership work (`core-only-011` plus `011-1`). That ownership design was intentionally **not re-audited** in this patchset; it was only treated as part of the exact baseline.

## Scope

The 20 patches remove repeated stable lookups, avoid temporary containers in bounded hot paths, remove type-erased recursion from playerbot spell-chain traversal, make playerbot combat action tables static, and convert several proven membership-only ordered containers to hash containers.

Highlights:

- 204: reuse stable `ItemPrototype` lookups in item/player/stat/transmog paths.
- 205: cache the stable `MovementInfo` member reference in movement-anticheat hot paths.
- 206: hoist immutable locale lookups out of per-session broadcast loops.
- 207-208: remove transient selection/graveyard vectors while preserving candidate order, RNG-index mapping and strict tie behavior.
- 212-213: remove per-call playerbot action-vector construction and `std::function` recursion overhead while preserving action/traversal order.
- 214-222: hash membership/tracking containers only where no ordered iteration, `begin()`, RNG indexing, or order-dependent side effect exists.
- 223: construct the Warden scan pattern directly from the source range.

## Application

Apply patches strictly in the order listed in `APPLY-ORDER.txt`.

No automatic apply script is included.

## Build policy

No full local build was run, by project policy/user request. Validation here is source/semantic/static plus strict sequential patch application. The user performs authoritative Linux/Windows/MinGW builds externally.
