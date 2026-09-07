#!/usr/bin/env bash
set -Eeuo pipefail

# Full Core compile audit for the dev-clean baseline.
# Repository modules are disabled. PCH is deliberately disabled to expose
# missing/self-contained headers. Discord/DPP is deliberately enabled because
# it is part of the generic Core dependency audit surface.
#
# If ccache is installed it is used automatically as C/C++ compiler launcher.
# The build directory is still recreated from scratch on every run; only
# compiler outputs are cached outside the build tree.

ROOT="${ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-core-full-audit}"
LOG="${LOG:-$BUILD_DIR/core-full-build-audit.log}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
ACE_PREFIX="${ACE_ROOT:-/usr}"
USE_CCACHE="${USE_CCACHE:-auto}"
CCACHE_BIN="${CCACHE_BIN:-}"
NINJA_VERBOSE="${NINJA_VERBOSE:-off}"
NINJA_STATS="${NINJA_STATS:-on}"
NINJA_STATUS_FORMAT="${NINJA_STATUS_FORMAT:-[%f/%t %p | %e sec | %r running | %o edges/s] }"

if [[ ! -f "$ROOT/CMakeLists.txt" ]]; then
    echo "ERROR: run this from the tortoise-wow repository root (or set ROOT=/path/to/tortoise-wow)." >&2
    exit 2
fi

for tool in cmake ninja; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: $tool not found." >&2
        exit 2
    fi
done

case "$USE_CCACHE" in
    auto|on|off)
        ;;
    *)
        echo "ERROR: USE_CCACHE must be one of: auto, on, off." >&2
        exit 2
        ;;
esac

case "$NINJA_VERBOSE" in
    on|off)
        ;;
    *)
        echo "ERROR: NINJA_VERBOSE must be one of: on, off." >&2
        exit 2
        ;;
esac

case "$NINJA_STATS" in
    on|off)
        ;;
    *)
        echo "ERROR: NINJA_STATS must be one of: on, off." >&2
        exit 2
        ;;
esac

if [[ "$USE_CCACHE" != "off" ]]; then
    if [[ -z "$CCACHE_BIN" ]] && command -v ccache >/dev/null 2>&1; then
        CCACHE_BIN="$(command -v ccache)"
    fi

    if [[ -n "$CCACHE_BIN" ]]; then
        if ! command -v "$CCACHE_BIN" >/dev/null 2>&1 && [[ ! -x "$CCACHE_BIN" ]]; then
            echo "ERROR: configured CCACHE_BIN is not executable: $CCACHE_BIN" >&2
            exit 2
        fi
    elif [[ "$USE_CCACHE" == "on" ]]; then
        echo "ERROR: USE_CCACHE=on but ccache was not found." >&2
        exit 2
    fi
fi

if [[ ! -f "${ACE_PREFIX}/include/ace/ACE.h" && ! -f "${ACE_PREFIX}/include/ace/Basic_Types.h" ]]; then
    echo "WARNING: ACE headers not found below ${ACE_PREFIX}/include/ace/."
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
printf 'Scope      : full Core + scripts + Discord/DPP; repository modules disabled; PCH OFF\n'
printf 'Ninja stats: %s\n' "$NINJA_STATS"
printf 'Ninja verbose: %s\n' "$NINJA_VERBOSE"
printf 'Ninja status: %s\n' "$NINJA_STATUS_FORMAT"
if [[ -n "$CCACHE_BIN" && "$USE_CCACHE" != "off" ]]; then
    printf 'C/C++ cache : %s\n' "$("$CCACHE_BIN" --version | head -n 1)"
else
    printf 'C/C++ cache : disabled'
    if [[ "$USE_CCACHE" == "auto" ]]; then
        printf ' (ccache not installed)'
    fi
    printf '\n'
fi
printf '\n'

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

CMAKE_ARGS=(
    -S "$ROOT"
    -B "$BUILD_DIR"
    -G Ninja
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DMODULES=disabled
    -DUSE_PCH=OFF
    -DUSE_SCRIPTS=ON
    -DUSE_EXTRACTORS=OFF
    -DUSE_DISCORD_BOT=ON
)

if [[ -n "${ACE_ROOT:-}" ]]; then
    CMAKE_ARGS+=( -DACE_ROOT="$ACE_ROOT" )
fi

if [[ -n "$CCACHE_BIN" && "$USE_CCACHE" != "off" ]]; then
    # Keep statistics for this audit run separate without resetting the
    # user's global ccache counters.
    export CCACHE_STATSLOG="${CCACHE_STATSLOG:-$BUILD_DIR/ccache-stats.log}"
    CMAKE_ARGS+=(
        -DCMAKE_C_COMPILER_LAUNCHER="$CCACHE_BIN"
        -DCMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_BIN"
    )
fi

echo '== CMake configure =='
cmake "${CMAKE_ARGS[@]}" 2>&1 | tee "$BUILD_DIR/cmake-configure.log"

echo
echo '== Full Core compile audit =='

run_ninja() {
    if command -v ionice >/dev/null 2>&1; then
        nice -n 10 ionice -c2 -n7 ninja "$@"
    else
        nice -n 10 ninja "$@"
    fi
}

NINJA_ARGS=(
    -C "$BUILD_DIR"
    -j"$JOBS"
    -k0
)

if [[ "$NINJA_STATS" == "on" ]]; then
    NINJA_ARGS+=( -d stats )
fi

if [[ "$NINJA_VERBOSE" == "on" ]]; then
    NINJA_ARGS+=( -v )
fi

set +e
NINJA_STATUS="$NINJA_STATUS_FORMAT" run_ninja "${NINJA_ARGS[@]}" \
    2>&1 | tee "$LOG"
status=${PIPESTATUS[0]}
set -e

echo
if [[ -n "$CCACHE_BIN" && "$USE_CCACHE" != "off" ]]; then
    echo '== ccache statistics for this audit =='
    if ! "$CCACHE_BIN" --show-log-stats 2>/dev/null; then
        echo "(Per-run statistics unavailable; showing cumulative ccache statistics.)"
        "$CCACHE_BIN" --show-stats || true
    fi
    echo
fi

if (( status == 0 )); then
    echo "OK: full Core audit compiled successfully."
else
    echo "FAILED: full Core audit returned exit code $status." >&2
    echo "Full log: $LOG" >&2
    echo >&2
    echo "Compiler error summary:" >&2
    grep -nE '(^|: )(fatal error:|error:|undefined reference|FAILED:)' "$LOG" | tail -n 160 >&2 || true
fi

exit "$status"
