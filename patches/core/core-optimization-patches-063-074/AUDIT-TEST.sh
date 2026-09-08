#!/usr/bin/env bash
set -euo pipefail

# Reproduces the strict sequential audit for patches 063-074.
# IMPORTANT: this script APPLIES the patches. Run it only against a disposable
# test copy of dev-clean whose source is already reconstructed through 062.

BUNDLE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO="${1:-.}"
REPO="$(cd -- "$REPO" && pwd)"

if [[ ! -d "$REPO/.git" ]]; then
    echo "error: $REPO is not a Git working tree" >&2
    exit 2
fi

cd "$REPO"

while IFS= read -r patch_name; do
    [[ -n "$patch_name" ]] || continue
    patch="$BUNDLE_DIR/patches/$patch_name"
    echo "== $patch_name =="

    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
        apply --check --whitespace=error-all "$patch"

    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
        apply --numstat --whitespace=error-all "$patch"

    if grep -nP '^\+[ \t]*\t' "$patch" | grep -v '^+++ ' >/dev/null 2>&1; then
        echo "error: added indentation contains a tab: $patch_name" >&2
        exit 3
    fi

    if grep -nE '^\+(<<<<<<<|=======|>>>>>>>)' "$patch" >/dev/null 2>&1; then
        echo "error: patch adds a conflict marker: $patch_name" >&2
        exit 4
    fi

    git apply "$patch"
    git diff --check
    echo "PASS"
    echo
done < "$BUNDLE_DIR/APPLY-ORDER.txt"

if git grep -n -E '^(<<<<<<< |>>>>>>> )' -- ':!patches/**'; then
    echo "error: real conflict marker found in reconstructed source tree" >&2
    exit 5
fi

git diff --check

echo "SUCCESS: 063-074 strict sequential audit passed"
