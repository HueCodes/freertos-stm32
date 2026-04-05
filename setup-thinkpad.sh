#!/usr/bin/env bash
# freertos-stm32: ThinkPad setup and build
# Prerequisites: arm-none-eabi-gcc, cmake, st-flash (from thinkpad-setup.sh)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CUBE_DIR="$HOME/STM32Cube/Repository/STM32Cube_FW_F4"
FREERTOS_REPO="https://github.com/FreeRTOS/FreeRTOS-Kernel.git"
THIRD_PARTY="$SCRIPT_DIR/third_party"

echo "=== freertos-stm32 setup ==="

# Check toolchain
for tool in arm-none-eabi-gcc cmake st-flash; do
    if ! command -v "$tool" &>/dev/null; then
        echo "ERROR: $tool not found. Run ../thinkpad-setup.sh first."
        exit 1
    fi
done

# Check STM32CubeF4
if [ ! -d "$CUBE_DIR" ]; then
    echo "ERROR: STM32CubeF4 not found at $CUBE_DIR"
    echo "Run ../thinkpad-setup.sh first."
    exit 1
fi

# Set up third_party/ with symlinks to shared deps
echo "Setting up third_party/ dependencies..."
mkdir -p "$THIRD_PARTY"

# FreeRTOS kernel
if [ ! -d "$THIRD_PARTY/FreeRTOS/Source" ]; then
    echo "  Cloning FreeRTOS kernel..."
    mkdir -p "$THIRD_PARTY/FreeRTOS"
    if [ -d "$HOME/STM32Cube/FreeRTOS-Kernel" ]; then
        ln -sf "$HOME/STM32Cube/FreeRTOS-Kernel" "$THIRD_PARTY/FreeRTOS/Source"
        echo "  Symlinked from $HOME/STM32Cube/FreeRTOS-Kernel"
    else
        git clone --depth 1 -b V11.1.0 "$FREERTOS_REPO" "$THIRD_PARTY/FreeRTOS/Source"
        echo "  Cloned FreeRTOS V11.1.0"
    fi
else
    echo "  FreeRTOS already present."
fi

# STM32F4xx HAL Driver
if [ ! -d "$THIRD_PARTY/STM32F4xx_HAL_Driver" ]; then
    ln -sf "$CUBE_DIR/Drivers/STM32F4xx_HAL_Driver" "$THIRD_PARTY/STM32F4xx_HAL_Driver"
    echo "  Symlinked HAL driver from STM32CubeF4"
else
    echo "  HAL driver already present."
fi

# CMSIS
if [ ! -d "$THIRD_PARTY/CMSIS" ]; then
    ln -sf "$CUBE_DIR/Drivers/CMSIS" "$THIRD_PARTY/CMSIS"
    echo "  Symlinked CMSIS from STM32CubeF4"
else
    echo "  CMSIS already present."
fi

# Build
echo ""
echo "Building..."
cd "$SCRIPT_DIR"
rm -rf build
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -B build
cmake --build build -j"$(nproc)"

echo ""
echo "Build complete. Output:"
ls -lh build/freertos-stm32.bin build/freertos-stm32.hex

echo ""
echo "To flash (plug in Nucleo-F411RE via USB):"
echo "  st-flash write build/freertos-stm32.bin 0x08000000"

echo ""
echo "=== Wiring ==="
echo "  BME280 sensor (I2C1):"
echo "    SDA   -> PB9"
echo "    SCL   -> PB8"
echo "    VCC   -> 3.3V"
echo "    GND   -> GND"
echo "    ADDR  -> GND (for 0x76) or VCC (for 0x77)"
echo ""
echo "  NOTE: No BME280 yet. The DHT11 from the Elegoo kit can sub in"
echo "  but needs a driver swap (different protocol -- 1-wire vs I2C)."
echo "  For initial bringup, the code will run and report sensor errors"
echo "  until BME280 is wired up."
echo ""
echo "  USART2 (PA2/PA3) -> ST-Link VCP (built into Nucleo, just USB)"
echo ""
echo "Serial monitor: screen /dev/ttyACM0 115200"
echo "CLI commands available once running (type 'help' in serial console)"
