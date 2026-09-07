#!/usr/bin/env bash
set -Eeuo pipefail

BUNDLE_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
REPO="${1:-$PWD}"

cd "$REPO"
git rev-parse --is-inside-work-tree >/dev/null
if [[ -n "$(git status --porcelain)" ]]; then
  echo "ERROR: working tree is not clean." >&2
  echo "       If the series is committed, use git revert in reverse commit order instead." >&2
  exit 2
fi

mapfile -t PATCHES < "$BUNDLE_ROOT/SERIES"
for (( i=${#PATCHES[@]}-1; i>=0; --i )); do
  rel="${PATCHES[$i]}"
  p="$BUNDLE_ROOT/$rel"
  echo "== REVERSE CHECK: $rel =="
  git apply -R --check -vv "$p"
  echo "== REVERSE APPLY: $rel =="
  git apply -R -vv "$p"
  git diff --check
  echo
done

echo "OK: series reverse-applied to working tree. Nothing was committed or pushed."
