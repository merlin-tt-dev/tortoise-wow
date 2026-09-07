#!/usr/bin/env bash
set -Eeuo pipefail

# Isolated compile audit for mod-worldoverlay.
# WorldOverlay is configured dynamically so Ninja exposes the dedicated
# mod_mod_worldoverlay target. Other repository modules remain disabled.

ROOT="${ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-worldoverlay-module-audit}"
LOG="${LOG:-$BUILD_DIR/worldoverlay-module-build-audit.log}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
ACE_PREFIX="${ACE_ROOT:-/usr}"

if [[ ! -f "$ROOT/CMakeLists.txt" || ! -d "$ROOT/modules/mod-worldoverlay/src" ]]; then
    echo "ERROR: run this from a WorldOverlay tortoise-wow tree (or set ROOT=/path/to/tortoise-wow)." >&2
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
printf 'Target     : mod_mod_worldoverlay ONLY\n\n'

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
    -DUSE_DISCORD_BOT=OFF
)

if [[ -n "${ACE_ROOT:-}" ]]; then
    CMAKE_ARGS+=( -DACE_ROOT="$ACE_ROOT" )
fi

echo '== CMake configure =='
cmake "${CMAKE_ARGS[@]}" 2>&1 | tee "$BUILD_DIR/cmake-configure.log"

# Capture the target list first. With pipefail, piping Ninja directly into
# grep -q can otherwise report a false failure when grep exits early and Ninja
# receives SIGPIPE.
TARGETS="$(ninja -C "$BUILD_DIR" -t targets all)"
if ! grep -qE '^mod_mod_worldoverlay:' <<<"$TARGETS"; then
    echo "ERROR: Ninja target mod_mod_worldoverlay was not generated." >&2
    echo "Relevant module targets:" >&2
    grep -i worldoverlay <<<"$TARGETS" >&2 || true
    exit 3
fi

echo
echo '== WorldOverlay module compile audit =='

run_ninja() {
    if command -v ionice >/dev/null 2>&1; then
        nice -n 10 ionice -c2 -n7 ninja "$@"
    else
        nice -n 10 ninja "$@"
    fi
}

set +e
run_ninja -C "$BUILD_DIR" -j"$JOBS" -k0 mod_mod_worldoverlay \
    2>&1 | tee "$LOG"
status=${PIPESTATUS[0]}
set -e

echo
if (( status == 0 )); then
    echo "OK: mod-worldoverlay audit compiled successfully."
    echo "Artifact(s):"
    find "$BUILD_DIR/modules" -maxdepth 3 -type f \
        \( -name '*worldoverlay*.so' -o -name '*worldoverlay*.dylib' \) \
        -print 2>/dev/null || true
else
    echo "FAILED: mod-worldoverlay audit returned exit code $status." >&2
    echo "Full log: $LOG" >&2
    echo >&2
    echo "Compiler error summary:" >&2
    grep -nE '(^|: )(fatal error:|error:|undefined reference|FAILED:)' "$LOG" | tail -n 120 >&2 || true
fi

exit "$status"
