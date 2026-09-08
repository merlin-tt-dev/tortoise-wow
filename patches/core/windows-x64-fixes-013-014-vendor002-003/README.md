# Re-audited patches against current dev-clean archive

Basis: `tortoise-wow-dev-clean.tar(4).xz`, with `conflict-01.patch` applied to emulate the post-rebase conflict cleanup.

## Status

- `core-buildsystem-013-windows-openssl-dpp-link-dependencies.patch`
  - still required
  - regenerated against current CMake files
  - fixes configuration-keyword handling for multi-library OpenSSL lists and DPP Winsock linkage

- `core-buildsystem-014-windows-httplib-crypt32-link.patch`
  - still required
  - old patch already applied cleanly, but regenerated against current context for consistency

- `core-vendor-002-g3dlite-mingw64-warning-cleanup.patch`
  - still required
  - regenerated against current G3D sources

- `core-vendor-003-libseh-mingw64-disable-unused-seh.patch`
  - still required
  - regenerated against current CMake files

- `core-vendor-001-cxx17-warning-cleanup.patch`
  - DO NOT APPLY
  - all intended changes are already present in the current archive:
    - DPP `PipeCloser`
    - RapidJSON iterator traits without `std::iterator`
    - utf8cpp checked iterator traits without `std::iterator`
    - utf8cpp unchecked iterator traits without `std::iterator`

## Validation

The four included patches were tested sequentially against the current archive (after `conflict-01.patch`):

```text
git apply --check --whitespace=error-all  PASS
git apply                                PASS
git diff --check                         PASS after every patch
```

No unresolved Git conflict markers remained after the conflict cleanup.
