#!/bin/sh
mydir="$(dirname "$0")"
cd "$mydir"

cleanup() {
    # Restore audio server on exit
    if [ -f "/customer/app/audioserver" ]; then
        /customer/app/audioserver &
    fi
    # Resume MainUI
    kill -CONT $(pidof MainUI) 2>/dev/null
}
trap cleanup EXIT INT TERM

# Pause MainUI so it releases the framebuffer and input
kill -STOP $(pidof MainUI) 2>/dev/null

# Release audio device for SDL2
killall -9 audioserver 2>/dev/null

# Set library search path (local libs + SigmaStar hardware libs)
export LD_LIBRARY_PATH="$mydir/lib:$mydir:/config/lib:/customer/lib:$LD_LIBRARY_PATH"

# Run game with log output for troubleshooting
./ikachan
