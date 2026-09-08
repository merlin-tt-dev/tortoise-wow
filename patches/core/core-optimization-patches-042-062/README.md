# Core optimization patches 042-062

Source of truth used for this bundle:

- `tortoise-wow-dev-clean.tar(5).xz`
- optimization patches `023` through `041` from that archive were applied first
- patches `042` through `062` were then built on top of that reconstructed state

## Status

All patches in this bundle were validated as a complete ordered series from a fresh copy of the archive.

For every patch:

- `git apply --check --whitespace=error-all` passed
- the repository `tab-in-indent` whitespace policy passed
- validator-style `git apply --numstat --whitespace=error-all` passed
- `git diff --check` passed after application

The final full sequence `tar(5) -> 023..041 -> 042..062` also passed.

These patches have **not** been declared full-build green here. Linux/Windows build audits remain the external compile/link gate run by the user.

## Apply

Apply in the exact order in `APPLY-ORDER.txt`.

Recommended strict check for each patch:

```bash
git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --check --whitespace=error-all PATCH

git apply PATCH
git diff --check
```

## Scope

`042-062` continue the `dev-clean` performance/modernization pass. They include queue/lock improvements, broadcaster snapshot/allocation reductions, Warden container/range cleanup, ByteBuffer and AccountMgr copy reductions, associative lookup reductions, addon output allocation cleanup, LFG and threat-report hotpath reductions, inventory-load container improvements, and RAII/value ownership modernization for Weather/Terrain.

The goal for `dev-clean` is the best technically sound implementation while preserving behavior. Upstream-minimal versions can be derived separately later.
