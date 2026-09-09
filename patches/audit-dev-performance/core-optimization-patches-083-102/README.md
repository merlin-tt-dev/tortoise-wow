# Core Optimization corrected bundle 083-102

Target branch: `audits/dev-performance`

## Exact baseline

This bundle was validated against the user-provided archive:

`tortoise-wow.tar(1).xz`

SHA256:

`16aef1c17c073d6c1b91a6dc58ec25f833b21039ec3e94f1ff20471c313512e2`

The source contained in that archive already includes the source changes from optimization/audit patches `075-082`. This was verified directly on the extracted source: patches `076-082` all pass `git apply --reverse --check` and fail forward apply, so they MUST NOT be applied again.

This bundle therefore starts at `083`.

## Contents

Exactly 20 patchfiles, respecting the project handoff limit:

- audit/correctness: `083-087`
- performance/modernization: `088-102`

Use `APPLY-ORDER.txt` exactly.

## Manual integration workflow

For each patch, on the real branch:

```bash
git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --check --whitespace=error-all patches/<PATCH>

git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --numstat --whitespace=error-all patches/<PATCH>

git apply patches/<PATCH>
git diff --check

git add -A
git commit
```

You may commit between every patch. `AUDIT-TEST.sh` does NOT apply anything to the real integration tree.

## Isolated audit runner

From this bundle directory:

```bash
./AUDIT-TEST.sh /path/to/tortoise-wow
```

The script creates a temporary detached Git worktree at the repository's current committed HEAD, applies the complete bundle there, runs the strict checks, and removes that temporary worktree afterward. The real repository HEAD and working-tree status must remain unchanged.

## Build status

Patch/apply/whitespace/sequential validation is documented in `VALIDATION.txt`.
A Linux/MinGW full build is still a separate gate and is not claimed by this bundle.
