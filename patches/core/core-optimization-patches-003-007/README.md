# Core optimization patches 003-007

Built against the uploaded `tortoise-wow-dev-clean.tar(4).xz` source snapshot after removing the two residual Player.cpp conflict markers with `conflict-01.patch`.

## Apply order

1. core-optimization-003-precompile-gold-removal-regex.patch
2. core-optimization-004-move-anticheat-transient-inserts.patch
3. core-optimization-005-massmail-move-semantics.patch
4. core-optimization-006-warden-filesystem-module-scan.patch
5. core-optimization-007-antispam-character-filter-single-pass.patch

## Intent

### 003 - Precompile fixed admin-command regex
Makes the fixed gold-removal command regex a function-local static optimized regex, avoiding regex compilation on every command invocation.

### 004 - Move transient anticheat data into owning containers
Moves AddonInfo objects into the temporary addon vector and moves rvalue scan shared_ptrs into the Warden queue instead of copying them and performing avoidable string allocations/shared_ptr refcount operations.

### 005 - Make MassMail ownership explicitly move-only
Replaces the legacy copy constructor that const_casts a const MassMail and steals its unique_ptr with normal deleted copy operations and defaulted move operations.

### 006 - Replace ACE directory enumeration with C++17 filesystem
Replaces ACE opendir/readdir usage in Warden module discovery with std::filesystem::directory_iterator. This removes the Windows-specific intentional directory-handle leak caused by skipping ACE closedir there. The old exact case-sensitive `.bin` suffix rule is preserved.

### 007 - Collapse four Antispam regex filters into one in-place pass
Replaces four independent std::regex_replace passes for control characters, punctuation, whitespace/underscore, and digits with one erase/remove_if pass controlled by the same mask bits.

Validation for 007:
- all 16 combinations of the four mask bits compared old vs new
- 5,000 random byte strings per mask combination
- bytes include full 0x00-0xFF range
- exact output equivalence under C locale
- local synthetic microbenchmark of this filter sub-step: ~14.7x faster (not a whole-server or whole-Antispam benchmark)

## Validation performed

For every patch individually and for the full sequential series:

    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent apply --check --whitespace=error-all PATCH
    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent apply --numstat --whitespace=error-all PATCH
    git diff --check

All added lines were additionally checked for tab indentation.

A native full-build attempt could not configure in the artifact container because ACE is not installed there. Therefore these patches are apply/whitespace/semantic-audit validated, but not claimed as full-build validated in this environment.
