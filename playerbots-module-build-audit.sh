#!/usr/bin/env bash
set -Eeuo pipefail

# Isolated compile audit for mod-playerbots.
# PlayerBots is intentionally static-only because it has direct Core integration
# points. With all other modules disabled, the `modules` target contains the
# PlayerBots sources plus the Core dependency graph required by that static
# target; it does not build the complete worldserver target.

ROOT="${ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-playerbots-module-audit}"
LOG="${LOG:-$BUILD_DIR/playerbots-module-build-audit.log}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
ACE_PREFIX="${ACE_ROOT:-/usr}"

if [[ ! -f "$ROOT/CMakeLists.txt" || ! -d "$ROOT/modules/mod-playerbots/src" ]]; then
    echo "ERROR: PlayerBots module tree not found. Apply/reconstruct the PlayerBots patch series first, then run this from the repository root (or set ROOT=/path/to/tortoise-wow)." >&2
    exit 2
fi

for tool in cmake ninja; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: $tool not found." >&2
        exit 2
    fi
done

if [[ ! -f "${ACE_PREFIX}/include/ace/ACE.h" ]]; then
    echo "WARNING: ${ACE_PREFIX}/include/ace/ACE.h not found."
    echo "         If ACE is installed in a custom prefix, run e.g.: ACE_ROOT=/opt/ace $0"
fi

ace_library_available() {
    if command -v ldconfig >/dev/null 2>&1 && \
       ldconfig -p 2>/dev/null | grep -qE 'libACE\.so([.[:space:]]|$)'; then
        return 0
    fi

    compgen -G "${ACE_PREFIX}/lib/libACE.so*" >/dev/null ||
    compgen -G "${ACE_PREFIX}/lib64/libACE.so*" >/dev/null ||
    compgen -G "${ACE_PREFIX}/lib/*/libACE.so*" >/dev/null
}

if ! ace_library_available; then
    echo "WARNING: ACE library not found in the linker cache or below ${ACE_PREFIX}/lib{,64} (including multiarch subdirectories)."
fi

NPROC="$(nproc)"
JOBS=$(( NPROC * 80 / 100 ))
(( JOBS < 1 )) && JOBS=1
if (( NPROC > 1 && JOBS >= NPROC )); then
    JOBS=$(( NPROC - 1 ))
fi

BRANCH="$(git -C "$ROOT" branch --show-current 2>/dev/null || true)"
REVISION="$(git -C "$ROOT" rev-parse --verify HEAD 2>/dev/null || true)"

printf 'Repository : %s\n' "$ROOT"
printf 'Branch     : %s\n' "${BRANCH:-<detached/unknown>}"
printf 'Revision   : %s\n' "${REVISION:-<unknown>}"
printf 'Build dir  : %s\n' "$BUILD_DIR"
printf 'Build type : %s\n' "$BUILD_TYPE"
printf 'CPUs       : %s logical\n' "$NPROC"
printf 'Ninja jobs : %s (<=80%%)\n' "$JOBS"
printf 'Target     : modules (PlayerBots static-only; all other modules disabled)\n\n'

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

CMAKE_ARGS=(
    -S "$ROOT"
    -B "$BUILD_DIR"
    -G Ninja
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DMODULES=disabled
    -DMODULE_MOD_PLAYERBOTS=static
    -DUSE_PCH=OFF
    -DUSE_EXTRACTORS=OFF
    -DUSE_DISCORD_BOT=OFF
)

if [[ -n "${ACE_ROOT:-}" ]]; then
    CMAKE_ARGS+=( -DACE_ROOT="$ACE_ROOT" )
fi

echo '== CMake configure =='
cmake "${CMAKE_ARGS[@]}" 2>&1 | tee "$BUILD_DIR/cmake-configure.log"

TARGETS="$(ninja -C "$BUILD_DIR" -t targets all)"
if ! grep -qE '^modules:' <<<"$TARGETS"; then
    echo "ERROR: Ninja target modules was not generated." >&2
    echo "Relevant targets:" >&2
    grep -iE '(^modules:|playerbots)' <<<"$TARGETS" >&2 || true
    exit 3
fi

echo
echo '== PlayerBots static module compile audit =='

run_ninja() {
    if command -v ionice >/dev/null 2>&1; then
        nice -n 10 ionice -c2 -n7 ninja "$@"
    else
        nice -n 10 ninja "$@"
    fi
}

set +e
run_ninja -C "$BUILD_DIR" -j"$JOBS" -k0 modules \
    2>&1 | tee "$LOG"
status=${PIPESTATUS[0]}
set -e

echo
if (( status == 0 )); then
    echo "OK: mod-playerbots static module audit compiled successfully."
    echo "Artifact(s):"
    find "$BUILD_DIR/modules" -maxdepth 3 -type f \
        \( -name 'libmodules.a' -o -name 'modules.lib' \) \
        -print 2>/dev/null || true
else
    echo "FAILED: mod-playerbots audit returned exit code $status." >&2
    echo "Full log: $LOG" >&2
    echo >&2
    echo "Compiler error summary:" >&2
    grep -nE '(^|: )(fatal error:|error:|undefined reference|FAILED:)' "$LOG" | tail -n 120 >&2 || true
fi

exit "$status"
