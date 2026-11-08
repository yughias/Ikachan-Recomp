# Ikachan Recomp

Recompilation of the Ikachan game for PC, Linux, Emscripten, PSP, and Miyoo Mini (OnionOS).

All calls to `windows.h` and, more generally, the Windows API have been replaced with SDL2, a cross-platform library. This made compiling for each platform straightforward.

An autosave support has been made, however you can disable it inside the config.ini file.

## Dependencies

This project would not have been possible without this repository: [Ikachan decompilation](https://github.com/cuckydev/Ikachan).

The other libraries used are [SDL2](https://github.com/libsdl-org/SDL), [SDL_ttf](https://github.com/libsdl-org/SDL_ttf), and [SDL_mixer](https://github.com/libsdl-org/SDL_mixer).

## Building

### Build for Windows/Linux

To build on PC (MinGW-w64 on Windows or Linux), simply run:

```bash
cmake -B build
cmake --build build
```

Dependencies are automatically downloaded using FetchContent.

### Build for Emscripten

```bash
emcmake cmake -B build
cmake --build build
```

### Build for PSP

```bash
mkdir build
cd build
psp-cmake ..
make
```

Alternatively, you can use the `build_psp.sh` script. If the PSP development toolchain is not installed but Docker is available, the project will be built inside the pspdev container.

### Build for Miyoo Mini

Use the preconfigured `build_miyoo.sh` script.

## Installation

You can download the compiled binaries from the Releases page of this repository.

For the Miyoo Mini, simply extract the `.zip` file to the root of your SD card.
