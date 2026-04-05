# arm-none-eabi.cmake - CMake toolchain file for ARM bare-metal targets
#
# Tells CMake to use arm-none-eabi-gcc instead of the host compiler.
# Pass to cmake with: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake ..

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain prefix - assumes arm-none-eabi-gcc is on PATH
# Install on macOS: brew install --cask gcc-arm-embedded
# Or download from: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
set(TOOLCHAIN_PREFIX arm-none-eabi-)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

# Do not try to compile and link a test executable when checking the toolchain.
# Cross-compilers cannot run on the host machine.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search for libraries and headers only in the sysroot, not on the host
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
