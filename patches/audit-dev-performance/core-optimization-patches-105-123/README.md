# Core Optimization 105-123

Baseline: `audits/dev-performance` with core optimizations through `104` already integrated.
The user's known integration HEAD at the start of this block was `2996939`.

This handoff contains 19 patches, `105` through `123`, in exact numeric apply order.
Audit fixes keep the `core-optimization-audit-*` prefix; performance/modernization patches use `core-optimization-*`.

No apply/audit script is included. The intended workflow is manual integration, one patch at a time:

    git apply --check -vv PATCH
    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent apply --check --whitespace=error-all PATCH
    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent apply --numstat --whitespace=error-all PATCH
    git apply PATCH
    git diff --check
    git add ...
    git commit

The complete 105-123 sequence was also validated in an isolated disposable worktree against the exact source state after 104. No user worktree was modified by that validation.

Full Linux/MinGW build status is intentionally not claimed here; external build audits remain the build gate.
