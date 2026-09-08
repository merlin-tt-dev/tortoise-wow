#!/usr/bin/env bash
set -Eeuo pipefail

EXPECTED_BRANCH="dev-clean"
EXPECTED_BASE_HEAD="59ea6fd092895c919619f47ddebbbe8c8465bd36"
ALLOW_HEAD_DRIFT="${ALLOW_HEAD_DRIFT:-0}"
SKIP_BUILD="${SKIP_BUILD:-0}"
ACE_ROOT="${ACE_ROOT:-/usr/src/ace-mingw64}"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PATCH_DIR="$SCRIPT_DIR/patches"

PATCHES=(
  "core-buildsystem-013-windows-openssl-dpp-link-dependencies.patch"
  "core-buildsystem-014-windows-httplib-crypt32-link.patch"
  "core-vendor-002-g3dlite-mingw64-warning-cleanup.patch"
  "core-vendor-003-libseh-mingw64-disable-unused-seh.patch"
)

die() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 1
}

command -v git >/dev/null 2>&1 || die "git not found"
command -v sha256sum >/dev/null 2>&1 || die "sha256sum not found"

[[ -f "$SCRIPT_DIR/SHA256SUMS" ]] || die "missing SHA256SUMS"
(
  cd "$SCRIPT_DIR"
  sha256sum -c SHA256SUMS
)

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || die "run this from inside the target git repository"
cd "$REPO_ROOT"

BRANCH="$(git branch --show-current)"
[[ "$BRANCH" == "$EXPECTED_BRANCH" ]] || die "expected branch '$EXPECTED_BRANCH', got '${BRANCH:-<detached>}'"

BASE_HEAD="$(git rev-parse --verify HEAD)"
if [[ "$BASE_HEAD" != "$EXPECTED_BASE_HEAD" ]]; then
  if [[ "$ALLOW_HEAD_DRIFT" != "1" ]]; then
    die "expected base HEAD $EXPECTED_BASE_HEAD, got $BASE_HEAD (set ALLOW_HEAD_DRIFT=1 only for an intentional revalidation)"
  fi
  printf 'WARNING: intentional HEAD drift enabled: expected %s, current %s\n' "$EXPECTED_BASE_HEAD" "$BASE_HEAD" >&2
fi

# Require a clean tracked state. Untracked build/audit files do not affect patch application.
git diff --quiet || die "tracked working tree is not clean"
git diff --cached --quiet || die "index is not clean"

printf 'Repository : %s\n' "$REPO_ROOT"
printf 'Branch     : %s\n' "$BRANCH"
printf 'Base HEAD  : %s\n' "$BASE_HEAD"
printf 'ACE_ROOT   : %s\n\n' "$ACE_ROOT"

for patch_name in "${PATCHES[@]}"; do
  patch="$PATCH_DIR/$patch_name"
  [[ -f "$patch" ]] || die "missing patch: $patch_name"

  # Re-check branch HEAD immediately before every write. Applying patches does not move HEAD;
  # a changed HEAD here means another process/user changed repository history mid-run.
  CURRENT_BRANCH="$(git branch --show-current)"
  CURRENT_HEAD="$(git rev-parse --verify HEAD)"
  [[ "$CURRENT_BRANCH" == "$EXPECTED_BRANCH" ]] || die "branch changed before applying $patch_name"
  [[ "$CURRENT_HEAD" == "$BASE_HEAD" ]] || die "HEAD changed before applying $patch_name: $BASE_HEAD -> $CURRENT_HEAD"

  printf '== CHECK %s ==\n' "$patch_name"
  git apply --check -vv --whitespace=error-all "$patch"

  printf '== APPLY %s ==\n' "$patch_name"
  git apply --whitespace=error-all "$patch"

  printf '== DIFF CHECK after %s ==\n' "$patch_name"
  git diff --check
  git diff --stat
  printf '\n'
done

printf '== FINAL PATCH VALIDATION ==\n'
git diff --check
printf 'Applied patch set successfully.\n\n'

git diff --stat

if [[ "$SKIP_BUILD" == "1" ]]; then
  printf '\nSKIP_BUILD=1: full Windows-x64 audit not started.\n'
  exit 0
fi

[[ -x "$REPO_ROOT/core-full-build-audit.sh" ]] || die "core-full-build-audit.sh is missing or not executable"
[[ -d "$ACE_ROOT" ]] || die "ACE_ROOT does not exist: $ACE_ROOT"

printf '\n== WINDOWS X64 FULL CORE AUDIT ==\n'
ACE_ROOT="$ACE_ROOT" "$REPO_ROOT/core-full-build-audit.sh" --target windows-x64

printf '\nSUCCESS: patch validation and Windows-x64 full Core audit completed.\n'
