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
AUDIT_TARGET="${AUDIT_TARGET:-native}"
MINGW_TRIPLET="${MINGW_TRIPLET:-x86_64-w64-mingw32}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
USE_CCACHE="${USE_CCACHE:-auto}"
CCACHE_BIN="${CCACHE_BIN:-}"
CCACHE_BASE_DIR="${CCACHE_BASE_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/ccache/tortoise-wow}"
NINJA_VERBOSE="${NINJA_VERBOSE:-off}"
NINJA_STATS="${NINJA_STATS:-on}"
NINJA_STATUS_FORMAT="${NINJA_STATUS_FORMAT:-[%f/%t %p | %e sec | %r running | %o edges/s] }"

usage() {
    cat <<'EOF'
Usage: ./core-full-build-audit.sh [--target native|windows-x64]

Targets:
  native       Build for the host platform (default).
  windows-x64  Cross-compile a 64-bit Windows Core with MinGW-w64.

The target can also be selected with AUDIT_TARGET=windows-x64.
Windows cross-builds require a Windows-target ACE build via ACE_ROOT and
MinGW-w64 tools named from MINGW_TRIPLET (default: x86_64-w64-mingw32).

When ccache is enabled, native and windows-x64 use separate cache directories
under CCACHE_BASE_DIR by default. Set CCACHE_DIR explicitly to override this.
EOF
}

while (( $# > 0 )); do
    case "$1" in
        --target)
            if (( $# < 2 )); then
                echo "ERROR: --target requires a value." >&2
                exit 2
            fi
            AUDIT_TARGET="$2"
            shift 2
            ;;
        --target=*)
            AUDIT_TARGET="${1#*=}"
            shift
            ;;
        --windows-x64)
            AUDIT_TARGET="windows-x64"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "ERROR: unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

case "$AUDIT_TARGET" in
    native)
        DEFAULT_BUILD_DIR="$ROOT/build-core-full-audit"
        DEFAULT_CCACHE_DIR="$CCACHE_BASE_DIR/native"
        ;;
    windows-x64)
        DEFAULT_BUILD_DIR="$ROOT/build-core-full-audit-windows-x64"
        DEFAULT_CCACHE_DIR="$CCACHE_BASE_DIR/windows-x64"
        ;;
    *)
        echo "ERROR: AUDIT_TARGET must be one of: native, windows-x64." >&2
        exit 2
        ;;
esac

BUILD_DIR="${BUILD_DIR:-$DEFAULT_BUILD_DIR}"
CCACHE_DIR="${CCACHE_DIR:-$DEFAULT_CCACHE_DIR}"
LOG="${LOG:-$BUILD_DIR/core-full-build-audit.log}"
ACE_PREFIX="${ACE_ROOT:-/usr}"

if [[ ! -f "$ROOT/CMakeLists.txt" ]]; then
    echo "ERROR: run this from the tortoise-wow repository root (or set ROOT=/path/to/tortoise-wow)." >&2
    exit 2
fi

REQUIRED_TOOLS=(cmake ninja)
if [[ "$AUDIT_TARGET" == "windows-x64" ]]; then
    MINGW_CC="${MINGW_CC:-${MINGW_TRIPLET}-gcc}"
    MINGW_CXX="${MINGW_CXX:-${MINGW_TRIPLET}-g++}"
    MINGW_RC="${MINGW_RC:-${MINGW_TRIPLET}-windres}"
    REQUIRED_TOOLS+=("$MINGW_CC" "$MINGW_CXX" "$MINGW_RC")
fi

for tool in "${REQUIRED_TOOLS[@]}"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: $tool not found." >&2
        if [[ "$AUDIT_TARGET" == "windows-x64" ]]; then
            echo "       Install a MinGW-w64 x86_64 toolchain or override MINGW_TRIPLET/MINGW_CC/MINGW_CXX/MINGW_RC." >&2
        fi
        exit 2
    fi
done

if [[ "$AUDIT_TARGET" == "windows-x64" ]]; then
    MINGW_MACHINE="$("$MINGW_CC" -dumpmachine)"
    if [[ "$MINGW_MACHINE" != x86_64-* ]]; then
        echo "ERROR: windows-x64 target requires an x86_64 MinGW compiler; $MINGW_CC reports: $MINGW_MACHINE" >&2
        exit 2
    fi
fi

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

if [[ "$AUDIT_TARGET" == "windows-x64" ]]; then
    if [[ -z "${ACE_ROOT:-}" ]]; then
        echo "ERROR: windows-x64 cross-build requires ACE_ROOT pointing to a MinGW-w64/Windows x64 ACE build." >&2
        echo "       A native Linux libACE installation cannot be linked into the Windows target." >&2
        exit 2
    fi

    if [[ ! -f "${ACE_ROOT}/include/ace/ACE.h" && ! -f "${ACE_ROOT}/include/ace/Basic_Types.h" ]]; then
        echo "ERROR: Windows-target ACE headers not found below ${ACE_ROOT}/include/ace/." >&2
        exit 2
    fi

    ace_cross_library_available() {
        compgen -G "${ACE_ROOT}/lib/libACE*.a" >/dev/null ||
        compgen -G "${ACE_ROOT}/lib/ACE*.lib" >/dev/null ||
        compgen -G "${ACE_ROOT}/lib64/libACE*.a" >/dev/null ||
        compgen -G "${ACE_ROOT}/lib64/ACE*.lib" >/dev/null ||
        compgen -G "${ACE_ROOT}/lib/*/libACE*.a" >/dev/null ||
        compgen -G "${ACE_ROOT}/lib/*/ACE*.lib" >/dev/null ||
        compgen -G "${ACE_ROOT}/libACE*.a" >/dev/null ||
        compgen -G "${ACE_ROOT}/ACE*.lib" >/dev/null
    }

    if ! ace_cross_library_available; then
        echo "ERROR: Windows-target ACE library not found below ACE_ROOT=${ACE_ROOT}." >&2
        exit 2
    fi
else
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
printf 'Target     : %s\n' "$AUDIT_TARGET"
if [[ "$AUDIT_TARGET" == "windows-x64" ]]; then
    printf 'MinGW target: %s\n' "$MINGW_MACHINE"
    printf 'MinGW C/C++ : %s / %s\n' "$MINGW_CC" "$MINGW_CXX"
    printf 'ACE target  : %s\n' "$ACE_ROOT"
fi
printf 'CPUs       : %s logical\n' "$NPROC"
printf 'Ninja jobs : %s (<=80%%)\n' "$JOBS"
printf 'Scope      : full Core + scripts + Discord/DPP; repository modules disabled; PCH OFF\n'
printf 'Ninja stats: %s\n' "$NINJA_STATS"
printf 'Ninja verbose: %s\n' "$NINJA_VERBOSE"
printf 'Ninja status: %s\n' "$NINJA_STATUS_FORMAT"
if [[ -n "$CCACHE_BIN" && "$USE_CCACHE" != "off" ]]; then
    printf 'C/C++ cache : %s\n' "$("$CCACHE_BIN" --version | head -n 1)"
    printf 'Cache dir   : %s\n' "$CCACHE_DIR"
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

if [[ "$AUDIT_TARGET" == "windows-x64" ]]; then
    CMAKE_ARGS+=(
        -DCMAKE_SYSTEM_NAME=Windows
        -DCMAKE_SYSTEM_PROCESSOR=x86_64
        -DCMAKE_C_COMPILER="$MINGW_CC"
        -DCMAKE_CXX_COMPILER="$MINGW_CXX"
        -DCMAKE_RC_COMPILER="$MINGW_RC"
    )
fi

if [[ -n "${ACE_ROOT:-}" ]]; then
    CMAKE_ARGS+=( -DACE_ROOT="$ACE_ROOT" )
fi

if [[ -n "$CCACHE_BIN" && "$USE_CCACHE" != "off" ]]; then
    # Keep native and MinGW outputs in independent caches so alternating
    # audit targets cannot evict each other's compiler results.
    mkdir -p "$CCACHE_DIR"
    export CCACHE_DIR

    # Keep statistics for this audit run separate without resetting the
    # selected target cache counters.
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
