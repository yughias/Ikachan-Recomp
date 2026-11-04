#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_miyoo"
DIST_DIR="${SCRIPT_DIR}/dist_miyoo"
GAME_DIR="${DIST_DIR}/Roms/PORTS/Games/Ikachan"
SHORTCUTS_DIR="${DIST_DIR}/Roms/PORTS/Shortcuts/Action"
PORTS_IMGS="${DIST_DIR}/Roms/PORTS/Imgs"

echo "=== Building Ikachan for Miyoo Mini / OnionOS (Ports) ==="

if command -v docker >/dev/null 2>&1; then
    echo "[+] Using Docker (bqcuongas/sdl2-miyoo)..."
    docker run --rm -v "${SCRIPT_DIR}:/src" -w /src bqcuongas/sdl2-miyoo:latest sh -c \
        "rm -rf build_miyoo && mkdir -p build_miyoo && cd build_miyoo && cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-miyoo.cmake .. && make -j\$(nproc)"
elif [ -d "/opt/mmiyoo" ]; then
    echo "[+] Using local toolchain in /opt/mmiyoo..."
    rm -rf "${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-miyoo.cmake ..
    make -j$(nproc)
    cd "${SCRIPT_DIR}"
else
    echo "[-] Error: Neither Docker nor /opt/mmiyoo toolchain found."
    echo "    Please install Docker or install the Miyoo Mini toolchain."
    exit 1
fi

echo "[+] Assembling OnionOS Ports package at ${DIST_DIR}..."
rm -rf "${DIST_DIR}"
mkdir -p "${GAME_DIR}/lib"
mkdir -p "${SHORTCUTS_DIR}/Imgs"
mkdir -p "${PORTS_IMGS}"

# 1. Main game files inside Games/Ikachan/
cp "${BUILD_DIR}/ikachan" "${GAME_DIR}/"
cp "${SCRIPT_DIR}/miyoo/launch.sh" "${GAME_DIR}/"
cp "${BUILD_DIR}/icon.png" "${GAME_DIR}/"
cp -r "${BUILD_DIR}/Pbm" "${GAME_DIR}/"
cp -r "${BUILD_DIR}/Pmd" "${GAME_DIR}/"
cp -r "${BUILD_DIR}/Resource" "${GAME_DIR}/"
cp "${BUILD_DIR}/NPChar.dat" "${GAME_DIR}/"
cp "${BUILD_DIR}/Words.ptx" "${GAME_DIR}/"
cp "${BUILD_DIR}/Event.ptx" "${GAME_DIR}/"
cp "${BUILD_DIR}/Staff.ptx" "${GAME_DIR}/"
cp "${BUILD_DIR}/Ika.rec" "${GAME_DIR}/"
cp -r "${BUILD_DIR}/lib/"* "${GAME_DIR}/lib/"

# Download Steward Fu hardware-accelerated SDL2/EGL/GLESv2 if not cached
CACHE_DIR="${SCRIPT_DIR}/.miyoo_cache"
mkdir -p "${CACHE_DIR}"
STEWARD_URL="https://raw.githubusercontent.com/steward-fu/sdl2/master/prebuilt/640x480"
for lib in libSDL2-2.0.so.0 libEGL.so libGLESv2.so; do
    if [ ! -f "${CACHE_DIR}/${lib}" ]; then
        echo "[+] Downloading Steward Fu hardware library: ${lib}..."
        if command -v curl >/dev/null 2>&1; then
            curl -sSL "${STEWARD_URL}/${lib}" -o "${CACHE_DIR}/${lib}"
        else
            wget -q "${STEWARD_URL}/${lib}" -O "${CACHE_DIR}/${lib}"
        fi
        if command -v docker >/dev/null 2>&1; then
            docker run --rm -v "${CACHE_DIR}:/cache" bqcuongas/sdl2-miyoo:latest /opt/mmiyoo/bin/arm-linux-gnueabihf-strip "/cache/${lib}" 2>/dev/null || true
        elif [ -x "/opt/mmiyoo/bin/arm-linux-gnueabihf-strip" ]; then
            /opt/mmiyoo/bin/arm-linux-gnueabihf-strip "${CACHE_DIR}/${lib}" 2>/dev/null || true
        fi
    fi
    cp "${CACHE_DIR}/${lib}" "${GAME_DIR}/lib/"
done

# 2. OnionOS Shortcuts (Shortcuts/Action/)
cp "${SCRIPT_DIR}/miyoo/Ikachan.notfound" "${SHORTCUTS_DIR}/Ikachan.notfound"
cp "${SCRIPT_DIR}/miyoo/Ikachan.notfound" "${SHORTCUTS_DIR}/Ikachan.port"
cp "${BUILD_DIR}/icon.png" "${SHORTCUTS_DIR}/Imgs/Ikachan.png"
cp "${BUILD_DIR}/icon.png" "${PORTS_IMGS}/Ikachan.png"

# Ensure all scripts have Unix (LF) line endings regardless of host OS
sed -i 's/\r$//' "${GAME_DIR}/launch.sh" "${SHORTCUTS_DIR}/Ikachan.notfound" "${SHORTCUTS_DIR}/Ikachan.port"
chmod +x "${GAME_DIR}/ikachan" "${GAME_DIR}/launch.sh" "${SHORTCUTS_DIR}/Ikachan.notfound" "${SHORTCUTS_DIR}/Ikachan.port"

echo "=== Build complete! ==="
echo "Output package is ready in: ${DIST_DIR}"
echo ""
echo "To install on Miyoo Mini (OnionOS):"
echo "  Copy the 'Roms' folder from 'dist_miyoo/' directly to the root of your SD card."
echo "  The resulting structure on your SD card will be:"
echo "    SDCARD/Roms/PORTS/Games/Ikachan/..."
echo "    SDCARD/Roms/PORTS/Shortcuts/Action/Ikachan.notfound (and .port)"
echo "    SDCARD/Roms/PORTS/Shortcuts/Action/Imgs/Ikachan.png"
echo ""
echo "  Then on the Miyoo Mini, go to Ports -> Action -> Ikachan (or run '~Import ports')."
