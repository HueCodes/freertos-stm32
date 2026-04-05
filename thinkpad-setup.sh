#!/usr/bin/env bash
# ThinkPad hardware projects setup -- run once on fresh Linux install
# Tested on Ubuntu 22.04/24.04, Fedora 39+, and OpenSUSE Tumbleweed
set -euo pipefail

echo "=== Hardware Projects ThinkPad Setup ==="
echo ""

# Detect package manager
if command -v apt &>/dev/null; then
    PM="apt"
elif command -v dnf &>/dev/null; then
    PM="dnf"
elif command -v zypper &>/dev/null; then
    PM="zypper"
else
    echo "ERROR: No supported package manager found (apt/dnf/zypper)"
    exit 1
fi

echo "Detected package manager: $PM"
echo ""

# ---- 1. ARM Toolchain (STM32 projects) ----
echo "--- [1/6] ARM GCC Toolchain ---"
if ! command -v arm-none-eabi-gcc &>/dev/null; then
    case $PM in
        apt)    sudo apt update && sudo apt install -y gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi ;;
        dnf)    sudo dnf install -y arm-none-eabi-gcc-cs arm-none-eabi-newlib ;;
        zypper) sudo zypper install -y cross-arm-none-eabi-gcc cross-arm-none-eabi-newlib ;;
    esac
else
    echo "  Already installed: $(arm-none-eabi-gcc --version | head -1)"
fi

# ---- 2. Build tools ----
echo ""
echo "--- [2/6] CMake, OpenOCD, stlink, gdb ---"
case $PM in
    apt)    sudo apt install -y cmake openocd stlink-tools gdb-multiarch libusb-1.0-0-dev ;;
    dnf)    sudo dnf install -y cmake openocd stlink gdb libusb1-devel ;;
    zypper) sudo zypper install -y cmake openocd stlink gdb libusb-1_0-devel ;;
esac

# ---- 3. ST-Link udev rules (Linux needs this for non-root flash) ----
echo ""
echo "--- [3/6] ST-Link udev rules ---"
UDEV_RULES="/etc/udev/rules.d/49-stlink.rules"
if [ ! -f "$UDEV_RULES" ]; then
    echo 'Setting up udev rules for ST-Link...'
    sudo tee "$UDEV_RULES" > /dev/null <<'UDEV'
# ST-Link V2
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="0666", GROUP="plugdev"
# ST-Link V2-1 (Nucleo onboard)
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE="0666", GROUP="plugdev"
# ST-Link V3
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374e", MODE="0666", GROUP="plugdev"
UDEV
    sudo udevadm control --reload-rules
    sudo udevadm trigger
    echo "  Done. You may need to replug the ST-Link."
else
    echo "  Already configured."
fi

# Add user to plugdev and dialout groups
sudo usermod -aG plugdev,dialout "$(whoami)" 2>/dev/null || true

# ---- 4. ESP-IDF (ESP32 projects) ----
echo ""
echo "--- [4/6] ESP-IDF v5.2 ---"
ESP_IDF_DIR="$HOME/esp/esp-idf"
if [ ! -d "$ESP_IDF_DIR" ]; then
    # Prerequisites
    case $PM in
        apt)    sudo apt install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 ;;
        dnf)    sudo dnf install -y git wget flex bison gperf python3 python3-pip cmake ninja-build ccache libffi-devel openssl-devel dfu-util libusb1 ;;
        zypper) sudo zypper install -y git wget flex bison gperf python3 python3-pip cmake ninja ccache libffi-devel libopenssl-devel dfu-util libusb-1_0 ;;
    esac

    mkdir -p "$HOME/esp"
    git clone -b v5.2 --recursive https://github.com/espressif/esp-idf.git "$ESP_IDF_DIR"
    cd "$ESP_IDF_DIR"
    ./install.sh esp32,esp32s3
    cd -
    echo ""
    echo "  ESP-IDF installed. Source it with:"
    echo "    source $ESP_IDF_DIR/export.sh"
else
    echo "  Already installed at $ESP_IDF_DIR"
fi

# ---- 5. PlatformIO (LoRa, spectrum analyzer) ----
echo ""
echo "--- [5/6] PlatformIO ---"
if ! command -v pio &>/dev/null; then
    python3 -m pip install --user platformio
    # Ensure ~/.local/bin is in PATH
    if ! echo "$PATH" | grep -q "$HOME/.local/bin"; then
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
        export PATH="$HOME/.local/bin:$PATH"
    fi
else
    echo "  Already installed: $(pio --version)"
fi

# ---- 6. STM32CubeF4 (HAL + CMSIS for STM32 projects) ----
echo ""
echo "--- [6/6] STM32CubeF4 HAL Package ---"
CUBE_DIR="$HOME/STM32Cube/Repository/STM32Cube_FW_F4"
if [ ! -d "$CUBE_DIR" ]; then
    echo "  Cloning STM32CubeF4 (this takes a few minutes)..."
    mkdir -p "$HOME/STM32Cube/Repository"
    git clone --depth 1 -b v1.28.1 https://github.com/STMicroelectronics/STM32CubeF4.git "$CUBE_DIR"
else
    echo "  Already installed at $CUBE_DIR"
fi

# ---- 7. FreeRTOS kernel (for freertos-stm32 and slam-engine) ----
echo ""
echo "--- [Bonus] FreeRTOS Kernel ---"
FREERTOS_DIR="$HOME/STM32Cube/FreeRTOS-Kernel"
if [ ! -d "$FREERTOS_DIR" ]; then
    git clone --depth 1 -b V11.1.0 https://github.com/FreeRTOS/FreeRTOS-Kernel.git "$FREERTOS_DIR"
else
    echo "  Already at $FREERTOS_DIR"
fi

echo ""
echo "========================================"
echo "Setup complete. Summary:"
echo "  ARM GCC:     $(arm-none-eabi-gcc --version 2>/dev/null | head -1 || echo 'check PATH')"
echo "  CMake:       $(cmake --version | head -1)"
echo "  OpenOCD:     $(openocd --version 2>&1 | head -1)"
echo "  st-flash:    $(st-flash --version 2>&1 | head -1)"
echo "  ESP-IDF:     $ESP_IDF_DIR"
echo "  PlatformIO:  $(pio --version 2>/dev/null || echo 'restart shell')"
echo "  STM32CubeF4: $CUBE_DIR"
echo "  FreeRTOS:    $FREERTOS_DIR"
echo ""
echo "NEXT STEPS:"
echo "  1. Log out and back in (for group changes)"
echo "  2. For ESP32 projects: source ~/esp/esp-idf/export.sh"
echo "  3. Run project-specific build scripts in each project dir"
echo "========================================"
