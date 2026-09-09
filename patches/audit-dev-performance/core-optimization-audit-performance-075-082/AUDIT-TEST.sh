#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 /path/to/dev-clean-at-074-baseline" >&2
    exit 2
fi

repo=$(cd "$1" && pwd)
bundle=$(cd "$(dirname "$0")" && pwd)
order="$bundle/APPLY-ORDER.txt"

cd "$repo"

if [[ ! -d .git ]]; then
    echo "ERROR: target must be a Git working tree" >&2
    exit 2
fi

if [[ -n $(git status --porcelain --untracked-files=no) ]]; then
    echo "ERROR: target has tracked modifications; use a clean 074 baseline" >&2
    exit 2
fi

while IFS= read -r rel; do
    [[ -z "$rel" || "$rel" == \#* ]] && continue
    patch="$bundle/$rel"
    echo "== $(basename "$patch") =="
    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
        apply --check --whitespace=error-all "$patch"
    git -c core.whitespace=trailing-space,space-before-tab,tab-in-indent \
        apply --numstat --whitespace=error-all "$patch"
    git apply "$patch"
    git diff --check
done < "$order"

git diff --check

if grep -R -n -E '^(<<<<<<< |>>>>>>> )' src 2>/dev/null; then
    echo "ERROR: conflict marker found in src" >&2
    exit 1
fi

if git diff --unified=0 -- src | grep '^+' | grep -v '^+++' | grep -P '^\+\t+'; then
    echo "ERROR: newly-added source indentation begins with a tab" >&2
    exit 1
fi

echo "075-082 strict sequential audit passed"
