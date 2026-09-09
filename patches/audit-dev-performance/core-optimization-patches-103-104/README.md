# Core Optimization corrected bundle 103-104

Target branch: `audits/dev-performance`

This is the continuation of `core-optimization-corrected-083-102` and contains 2 patchfiles.

Required baseline: the exact user-provided source state with `083-102` integrated in order. Do not apply this bundle directly to the pre-083 source.

Use `APPLY-ORDER.txt` exactly.

## Manual integration

Apply `103`, commit if desired, then `104`:

```bash
git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --check --whitespace=error-all patches/<PATCH>
git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
    apply --numstat --whitespace=error-all patches/<PATCH>
git apply patches/<PATCH>
git diff --check
```

## Isolated audit runner

After `083-102` are committed/applied in your repository:

```bash
./AUDIT-TEST.sh /path/to/tortoise-wow
```

The runner uses a temporary detached worktree and leaves the real integration tree unchanged.

## Build status

Strict patch/sequential validation is documented in `VALIDATION.txt`. Full Linux/MinGW build remains a separate gate.
