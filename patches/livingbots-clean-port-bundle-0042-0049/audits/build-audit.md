# Build audit — 0042 through 0049

Baseline remote branch verified before source work:
`livingbots/playerbots-clean-base` @ `9bdbd3fa75887460329a8ce7b57de6d5d4ecfda1`.

Configuration used for fresh validation:
- Debian ACE 8.0.2 sysroot
- Ninja
- `MODULE_MOD_PLAYERBOTS=static`
- `USE_PCH=OFF`
- `USE_SCRIPTS=ON`
- `USE_EXTRACTORS=OFF`
- Debug validation configuration with requested `-O0 -g0` (project appends its own common flags)

Validation results:
- Every exported patch: `git diff --check` PASS.
- Fresh chain from archived 0037 source through 0038 core/mod, 0039, 0040, 0041, 0042...0049: `git apply --check` + apply PASS.
- 24/24 files changed by 0042...0049 byte-identical between fresh chain and frozen worktree.
- Reverse 0049...0042 and reapply 0042...0049: PASS.
- Reapplied fresh result: 24/24 byte-identical.
- Cumulative changed-TU build in working audit tree: 19/19 PASS.
- Fresh changed-TU build after the full patch chain: 19/19 PASS.

During the fresh build, one `-j4` attempt caused the kernel to kill `cc1plus` while compiling `LootAction.cpp` due to memory pressure. This was an infrastructure/OOM failure, not a compiler diagnostic. The failed object and remaining objects were rebuilt serially with `-j1`; all passed.
