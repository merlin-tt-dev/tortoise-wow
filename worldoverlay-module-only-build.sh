#!/usr/bin/env bash
set -Eeuo pipefail

# Compile-audit ONLY mod-worldoverlay on Tortoise.
# The module is configured as a dynamic module so Ninja gets a dedicated
# target (mod_mod_worldoverlay) instead of compiling the full static modules
# aggregate / game core.
#
# ACE may live in distro-specific multiarch library directories.
# CMake's FindACE.cmake remains authoritative; the checks below only provide
# early diagnostics without assuming a single /usr/lib layout.

ROOT="${ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-worldoverlay-module-audit}"
LOG="${LOG:-$BUILD_DIR/worldoverlay-module-build.log}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

if [[ ! -f "$ROOT/CMakeLists.txt" || ! -d "$ROOT/modules/mod-worldoverlay/src" ]]; then
    echo "ERROR: run this from the tortoise-wow repository root (or set ROOT=/path/to/tortoise-wow)." >&2
    exit 2
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "ERROR: cmake not found." >&2
    exit 2
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "ERROR: ninja not found. On Manjaro: sudo pacman -S ninja" >&2
    exit 2
fi

# ACE system sanity check. CMake's own FindACE.cmake remains authoritative.
ACE_PREFIX="${ACE_ROOT:-/usr}"

if [[ ! -f "${ACE_PREFIX}/include/ace/ACE.h" ]]; then
    echo "WARNING: ${ACE_PREFIX}/include/ace/ACE.h not found."
    echo "         If ACE is installed in a custom prefix, run e.g.:"
    echo "         ACE_ROOT=/opt/ace $0"
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
# Do not consume every logical CPU even on very small systems when avoidable.
if (( NPROC > 1 && JOBS >= NPROC )); then
    JOBS=$(( NPROC - 1 ))
fi

printf 'Repository : %s\n' "$ROOT"
printf 'Build dir  : %s\n' "$BUILD_DIR"
printf 'Build type : %s\n' "$BUILD_TYPE"
printf 'CPUs       : %s logical\n' "$NPROC"
printf 'Ninja jobs : %s (<=80%%)\n' "$JOBS"
printf 'Target     : mod_mod_worldoverlay ONLY\n\n'

# Fresh build directory avoids stale per-module cache variables from earlier
# full/static builds.
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

CMAKE_ARGS=(
    -S "$ROOT"
    -B "$BUILD_DIR"
    -G Ninja
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DMODULES=disabled
    -DMODULE_MOD_WORLDOVERLAY=dynamic
    -DUSE_PCH=OFF
    -DUSE_EXTRACTORS=OFF
)

if [[ -n "${ACE_ROOT:-}" ]]; then
    CMAKE_ARGS+=( -DACE_ROOT="$ACE_ROOT" )
fi

echo '== CMake configure =='
cmake "${CMAKE_ARGS[@]}" 2>&1 | tee "$BUILD_DIR/cmake-configure.log"

# Verify the intended dedicated target exists before compiling anything.
if ! ninja -C "$BUILD_DIR" -t targets all | grep -qE '^mod_mod_worldoverlay:'; then
    echo "ERROR: Ninja target mod_mod_worldoverlay was not generated." >&2
    echo "Relevant module targets:" >&2
    ninja -C "$BUILD_DIR" -t targets all | grep -i worldoverlay >&2 || true
    exit 3
fi

echo
echo '== WorldOverlay module-only compile =='
echo "nice -n 10 ionice -c2 -n7 ninja -C '$BUILD_DIR' -j$JOBS -k0 mod_mod_worldoverlay"

set +e
nice -n 10 ionice -c2 -n7 \
    ninja -C "$BUILD_DIR" -j"$JOBS" -k0 mod_mod_worldoverlay \
    2>&1 | tee "$LOG"
status=${PIPESTATUS[0]}
set -e

echo
if (( status == 0 )); then
    echo "OK: mod-worldoverlay compiled successfully."
    echo "Artifact(s):"
    find "$BUILD_DIR/modules" -maxdepth 3 -type f \
        \( -name '*worldoverlay*.so' -o -name '*worldoverlay*.dylib' \) \
        -print 2>/dev/null || true
else
    echo "FAILED: mod-worldoverlay compile returned exit code $status." >&2
    echo "Full log: $LOG" >&2
    echo >&2
    echo "Compiler error summary:" >&2
    grep -nE '(^|: )(fatal error:|error:|undefined reference|FAILED:)' "$LOG" | tail -n 120 >&2 || true
fi

exit "$status"
