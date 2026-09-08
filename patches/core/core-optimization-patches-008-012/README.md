# Core optimization patches 008-012

Base used for audit: `tortoise-wow-dev-clean.tar(4).xz`, after applying `conflict-01.patch` and core optimization patches 001-007.

Apply order:

1. `core-optimization-008-antispam-formatting-without-regex.patch`
2. `core-optimization-009-warden-anticheat-filesystem-scan.patch`
3. `core-optimization-010-warden-md5-evp.patch`
4. `core-optimization-011-pdump-filesystem-scans.patch`
5. `core-optimization-012-mmap-reuse-associative-lookups.patch`

## Intent

### 008 - Antispam formatting normalization without std::regex
Replaces the remaining `NF_CUT_COLOR` regex replacements with in-place parsing while preserving the existing normalization order and greedy hyperlink behavior. Removes `<regex>` from Antispam.cpp.

Validation: old regex implementation vs new implementation matched for 300,000 randomized byte strings plus crafted WoW-link/color cases. A local microbenchmark of this formatting sub-step showed about 7.7x speedup on the test workload; this is not a whole-server performance claim.

### 009 - WardenAnticheat module scan via std::filesystem
Replaces ACE directory iteration with C++17 `std::filesystem::directory_iterator` using `std::error_code`, preserving silent behavior for missing/unreadable module directories. Avoids short-filename pointer arithmetic in the old `.bin` suffix check and uses RAII directory handles.

### 010 - Warden MD5 via OpenSSL EVP
Replaces deprecated `MD5_CTX` / `MD5_Init` / `MD5_Update` / `MD5_Final` calls with one-shot `EVP_Digest(..., EVP_md5(), ...)`, compatible with the project's modern OpenSSL path.

Validation: 10,000 randomized buffers produced byte-identical MD5 output between the old low-level API and EVP.

### 011 - Player dump scans via std::filesystem
Replaces remaining ACE directory scans in pdump list/cleanup paths with C++17 filesystem RAII. Removes the Windows intentional directory-handle leak workaround. Uses a vector for delete candidates because directory entries are already unique and no ordered-set behavior is consumed.

### 012 - Reuse MMap associative lookup results
Avoids repeated `find()` + `operator[]` and key-based erase lookups in MMap unload/query paths. Reuses iterators directly without changing navmesh ownership or query behavior.

## Validation

Each patch was checked individually and as a sequential series with:

- `git apply --check --whitespace=error-all`
- `git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent apply --check --whitespace=error-all`
- the validator-style `git ... apply --numstat --whitespace=error-all` on a clean pre-patch tree
- `git diff --check` after apply
- explicit scan for tabs in added patch lines

The complete 001-012 series was reapplied from the archived base successfully.

A full project build was not completed in this container; the user's normal native/Windows audit remains the build proof.
