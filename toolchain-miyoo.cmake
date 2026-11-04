# CMake Toolchain file for Miyoo Mini / OnionOS
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set Miyoo identifier
set(MIYOO TRUE)
set(__MIYOO__ TRUE)

# Toolchain location (default for Miyoo Mini Docker buildroot)
if(NOT DEFINED MIYOO_TOOLCHAIN)
    set(MIYOO_TOOLCHAIN "/opt/mmiyoo")
endif()

if(NOT DEFINED MIYOO_SYSROOT)
    set(MIYOO_SYSROOT "${MIYOO_TOOLCHAIN}/arm-buildroot-linux-gnueabihf/sysroot")
endif()

set(CMAKE_C_COMPILER "${MIYOO_TOOLCHAIN}/bin/arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${MIYOO_TOOLCHAIN}/bin/arm-linux-gnueabihf-g++")
set(CMAKE_SYSROOT "${MIYOO_SYSROOT}")

set(CMAKE_FIND_ROOT_PATH "${MIYOO_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Miyoo Mini target architecture flags (SigmaStar SSD202D: ARM Cortex-A7, NEON VFPv4, hard float)
set(MIYOO_ARCH_FLAGS "-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 ${MIYOO_ARCH_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 ${MIYOO_ARCH_FLAGS}" CACHE STRING "" FORCE)
