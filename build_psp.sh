#!/bin/bash
set -e
export PSPDEV=/root/pspdev
export PATH=$PSPDEV/bin:$PATH
cd "$(dirname "$0")/build_psp"
make
