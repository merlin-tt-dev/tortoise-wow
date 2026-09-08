# Core optimization patches 023-041

Base: local `tortoise-wow-dev-clean.tar(4).xz` reconstructed with `conflict-01.patch` and core optimization patches 001-022.

Apply order is lexical/numeric order from 023 through 041.

Validation performed:
- sequential `git apply --check --whitespace=error-all`
- `core.whitespace=trailing-space,space-before-tab,tab-in-indent`
- no tabs in added patch lines
- `git diff --check` after every applied patch
- full sequential apply test 023 -> 041 on the reconstructed 022 baseline

Additional semantic checks:
- antispam repeat counting: randomized old-list/count_if model vs direct-count map matched exactly
- CreatureAI target selection: original ordering/reverse/random semantics modeled against vector/stable-sort implementation
- pair hash sample: systematic swapped-pair XOR collisions removed

Build status: patches 023-041 have not yet been proven by the user's full Linux/Windows build audit.
