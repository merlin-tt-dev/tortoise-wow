#!/usr/bin/env bash
set -Eeuo pipefail

BUNDLE_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
REPO="${1:-$PWD}"
BASE="d856cfe89b9dba0a0c147bf5a38018df2cbd1cca"

cd "$REPO"
git rev-parse --is-inside-work-tree >/dev/null
if [[ -n "$(git status --porcelain)" ]]; then
  echo "ERROR: working tree is not clean." >&2
  exit 2
fi

HEAD="$(git rev-parse HEAD)"
if [[ "$HEAD" != "$BASE" ]]; then
  echo "WARNING: HEAD is $HEAD, expected exact base $BASE." >&2
  echo "         Continuing only if every git apply --check succeeds." >&2
fi

mapfile -t PATCHES < "$BUNDLE_ROOT/SERIES"
for rel in "${PATCHES[@]}"; do
  p="$BUNDLE_ROOT/$rel"
  echo "== CHECK: $rel =="
  git apply --check -vv "$p"
  echo "== APPLY: $rel =="
  git apply -vv "$p"
  git diff --check
  echo
done

echo "OK: series applied to working tree. Nothing was committed or pushed."
