#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build-full-audit}"
ACE_ROOT="${ACE_ROOT:-/usr}"
NPROC="$(nproc)"
JOBS=$((NPROC * 80 / 100))
if (( JOBS < 1 )); then
    JOBS=1
fi
if (( NPROC > 1 && JOBS >= NPROC )); then
    JOBS=$((NPROC - 1))
fi

if [[ ! -f "${ACE_ROOT}/include/ace/Basic_Types.h" ]]; then
    echo "ERROR: ACE headers not found at ${ACE_ROOT}/include/ace/Basic_Types.h" >&2
    echo "Set ACE_ROOT to the ACE installation prefix, e.g. ACE_ROOT=/usr or /opt/ace." >&2
    exit 2
fi

ace_library_available() {
    if command -v ldconfig >/dev/null 2>&1 && \
       ldconfig -p 2>/dev/null | grep -qE 'libACE\.so([.[:space:]]|$)'; then
        return 0
    fi

    compgen -G "${ACE_ROOT}/lib/libACE.so*" >/dev/null ||
    compgen -G "${ACE_ROOT}/lib64/libACE.so*" >/dev/null ||
    compgen -G "${ACE_ROOT}/lib/*/libACE.so*" >/dev/null
}

if ! ace_library_available; then
    echo "WARNING: ACE library not found in the linker cache or below ${ACE_ROOT}/lib{,64} (including multiarch subdirectories)."
fi

if ! command -v cmake >/dev/null || ! command -v ninja >/dev/null; then
    echo "ERROR: cmake and ninja must be installed." >&2
    exit 2
fi

echo "CPU: ${NPROC} logical CPUs; Ninja jobs capped at ${JOBS} (<=80%)."
echo "ACE_ROOT=${ACE_ROOT}"
echo "Build directory: ${BUILD_DIR}"

rm -rf "${BUILD_DIR}"

cmake -S . -B "${BUILD_DIR}" -G Ninja \
  -DACE_ROOT="${ACE_ROOT}" \
  -DMODULES=static \
  -DMODULE_MOD_PLAYERBOTS=static \
  -DMODULE_MOD_WORLDOVERLAY=static \
  -DUSE_PCH=OFF \
  -DUSE_SCRIPTS=ON \
  -DUSE_EXTRACTORS=OFF \
  -DUSE_DISCORD_BOT=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Lower CPU and I/O scheduling priority so interactive work remains responsive.
if command -v ionice >/dev/null; then
    nice -n 10 ionice -c2 -n7 \
      ninja -C "${BUILD_DIR}" -j"${JOBS}" -k0 2>&1 | tee "${BUILD_DIR}/full-build.log"
else
    nice -n 10 \
      ninja -C "${BUILD_DIR}" -j"${JOBS}" -k0 2>&1 | tee "${BUILD_DIR}/full-build.log"
fi
