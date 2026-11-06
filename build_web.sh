#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_wasm"
DIST_DIR="${SCRIPT_DIR}/dist_web"

echo "=== Building Ikachan for WebAssembly / HTML5 (Emscripten) ==="

if command -v emcmake >/dev/null 2>&1; then
    echo "[+] Using local Emscripten SDK..."
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j$(nproc)
    cd "${SCRIPT_DIR}"
elif command -v docker >/dev/null 2>&1; then
    echo "[+] Using Docker (emscripten/emsdk:3.1.50)..."
    docker run --rm -v "${SCRIPT_DIR}:/src" -w /src emscripten/emsdk:3.1.50 bash -c \
        "mkdir -p /src/build_wasm && cd /src/build_wasm && emcmake cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . -j\$(nproc)"
else
    echo "[-] Error: Neither emcmake nor docker found."
    exit 1
fi

mkdir -p "${DIST_DIR}"
cp "${BUILD_DIR}/ikachan.html" "${DIST_DIR}/ikachan.html"
cp "${BUILD_DIR}/ikachan.html" "${DIST_DIR}/index.html"
cp "${BUILD_DIR}/ikachan.js" "${DIST_DIR}/"
cp "${BUILD_DIR}/ikachan.wasm" "${DIST_DIR}/"
cp "${BUILD_DIR}/ikachan.data" "${DIST_DIR}/"

echo "=== Web build complete! Files ready in dist_web/ ==="
