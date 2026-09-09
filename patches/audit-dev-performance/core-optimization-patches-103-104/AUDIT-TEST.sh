#!/usr/bin/env bash
set -euo pipefail

BUNDLE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO="${1:-$PWD}"
REPO="$(git -C "$REPO" rev-parse --show-toplevel)"
TMPROOT="$(mktemp -d "${TMPDIR:-/tmp}/tortoise-core-audit.XXXXXX")"
WORKTREE="$TMPROOT/worktree"

cleanup() {
    git -C "$REPO" worktree remove --force "$WORKTREE" >/dev/null 2>&1 || true
    rm -rf "$TMPROOT"
}
trap cleanup EXIT INT TERM

ORIGINAL_HEAD="$(git -C "$REPO" rev-parse HEAD)"
ORIGINAL_STATUS="$(git -C "$REPO" status --porcelain=v1 --untracked-files=all)"

git -C "$REPO" worktree add --quiet --detach "$WORKTREE" "$ORIGINAL_HEAD"

while IFS= read -r patch_name; do
    [[ -n "$patch_name" ]] || continue
    patch="$BUNDLE_DIR/patches/$patch_name"
    echo "==> $patch_name"

    git -C "$WORKTREE" -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
        apply --check --whitespace=error-all "$patch"
    git -C "$WORKTREE" -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
        apply --numstat --whitespace=error-all "$patch"
    git -C "$WORKTREE" apply "$patch"
    git -C "$WORKTREE" diff --check

done < "$BUNDLE_DIR/APPLY-ORDER.txt"

# Scan only files changed by this bundle for real conflict markers.
while IFS= read -r file; do
    [[ -f "$WORKTREE/$file" ]] || continue
    if grep -nE '^(<<<<<<< |>>>>>>> )' "$WORKTREE/$file"; then
        echo "ERROR: conflict marker in $file" >&2
        exit 1
    fi
done < <(git -C "$WORKTREE" diff --name-only "$ORIGINAL_HEAD" --)

# No newly-added C/C++ indentation may contain a tab.
if git -C "$WORKTREE" diff --no-color -U0 "$ORIGINAL_HEAD" -- \
        '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' | \
    awk '
        /^\+\+\+/ { next }
        /^\+/ {
            line = substr($0, 2)
            if (line ~ /^ *\t/) {
                print "ERROR: added indentation contains tab: " $0 > "/dev/stderr"
                bad = 1
            }
        }
        END { exit bad ? 1 : 0 }
    '; then
    :
else
    exit 1
fi

git -C "$WORKTREE" diff --check

# The real integration tree must remain untouched by the audit.
[[ "$(git -C "$REPO" rev-parse HEAD)" == "$ORIGINAL_HEAD" ]]
[[ "$(git -C "$REPO" status --porcelain=v1 --untracked-files=all)" == "$ORIGINAL_STATUS" ]]

echo "PASS: strict sequential audit completed in isolated temporary worktree."
echo "PASS: integration tree HEAD/status unchanged."
