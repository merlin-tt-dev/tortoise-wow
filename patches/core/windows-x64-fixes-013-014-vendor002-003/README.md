# Windows x64 Core build fixes bundle

Target repository: `merlin-tt-dev/tortoise-wow`
Target branch: `dev-clean`
Expected base HEAD: `59ea6fd092895c919619f47ddebbbe8c8465bd36`

This bundle contains the logical follow-up set for the Windows x64 MinGW full-Core audit after Linux reached a clean build.

## Apply order

1. `core-buildsystem-013-windows-openssl-dpp-link-dependencies.patch`
   - fixes DPP/OpenSSL/Winsock transitive link dependencies and configuration-aware OpenSSL list handling.
2. `core-buildsystem-014-windows-httplib-crypt32-link.patch`
   - links Windows CryptoAPI (`crypt32`) required by cpp-httplib system certificate loading.
3. `core-vendor-002-g3dlite-mingw64-warning-cleanup.patch`
   - G3D MinGW64 warning cleanup: Winsock include order, const-correct `LPTSTR`, guarded `stat64` alias.
4. `core-vendor-003-libseh-mingw64-disable-unused-seh.patch`
   - stops building/linking 32-bit libseh compatibility code for 64-bit MinGW.

## Recommended use

From the repository root, extract this bundle somewhere outside the repository or into a temporary directory and run:

```bash
/path/to/windows-x64-fixes-013-014-vendor002-003/apply-and-test.sh
```

The script will:

- verify its bundled patch checksums;
- require branch `dev-clean`;
- require base HEAD `59ea6fd092895c919619f47ddebbbe8c8465bd36` unless `ALLOW_HEAD_DRIFT=1` is explicitly set;
- require no tracked working-tree/index changes before applying;
- re-read the repository HEAD before every write;
- run `git apply --check -vv --whitespace=error-all` before every patch;
- apply each patch in the order above;
- run `git diff --check` after every patch;
- print the cumulative diff stat;
- run the Windows-x64 full audit with `ACE_ROOT=/usr/src/ace-mingw64` by default.

To apply and validate patches without starting the full build:

```bash
SKIP_BUILD=1 /path/to/apply-and-test.sh
```

If the branch has moved intentionally but all patches should still be tested against the new HEAD:

```bash
ALLOW_HEAD_DRIFT=1 SKIP_BUILD=1 /path/to/apply-and-test.sh
```

`ALLOW_HEAD_DRIFT=1` does not bypass any `git apply --check` or `git diff --check` validation.

To use another Windows-target ACE prefix:

```bash
ACE_ROOT=/opt/ace-mingw64 /path/to/apply-and-test.sh
```

## Expected next audit state

Before this bundle, the Windows x64 audit reached the final executable link stage but failed linking both `realmd.exe` and `mangosd.exe`. The repo-owned warning classes remaining in that log were separated from the staged ACE warnings. Do not call Windows clean until a fresh audit actually links both executables.
