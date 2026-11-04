#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "${SCRIPT_DIR}/build_psp"

if command -v psp-cmake >/dev/null 2>&1; then
    echo "[+] Using local PSP toolchain..."
    cd "${SCRIPT_DIR}/build_psp"
    rm -rf CMakeCache.txt CMakeFiles
    psp-cmake ..
    make -j$(nproc)
elif command -v docker >/dev/null 2>&1; then
    echo "[+] Using Docker (pspdev/pspdev:latest)..."
    docker run --rm -v "${SCRIPT_DIR}:/src" -w /src pspdev/pspdev:latest bash -c \
        "mkdir -p /src/build_psp && cd /src/build_psp && rm -rf CMakeCache.txt CMakeFiles && psp-cmake .. && make -j\$(nproc)"
else
    echo "[-] Error: Neither psp-cmake nor docker found."
    exit 1
fi

echo "=== PSP build complete! EBOOT.PBP created in build_psp/ ==="
