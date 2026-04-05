# FreeRTOS STM32F411RE - Setup Guide

## What is done

All application code is written. The following need to happen before it compiles:

1. Install the ARM toolchain
2. Clone FreeRTOS into third_party/
3. Generate HAL + CMSIS sources via STM32CubeMX
4. Build

---

## Step 1 - Install ARM Toolchain

```bash
brew install --cask gcc-arm-embedded
```

Verify:
```bash
arm-none-eabi-gcc --version
```

---

## Step 2 - Clone FreeRTOS

```bash
mkdir -p third_party
git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git third_party/FreeRTOS/Source
```

Verify these files exist after cloning:
- third_party/FreeRTOS/Source/tasks.c
- third_party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c

---

## Step 3 - Generate HAL via STM32CubeMX

STM32CubeMX is a free GUI tool from ST that generates clock config and peripheral
init code. Download from: https://www.st.com/en/development-tools/stm32cubemx.html

Configuration for Nucleo-F411RE:

1. New project > Board Selector > search "NUCLEO-F411RE" > select it
2. Accept default peripheral init (it enables USART2 and USB)
3. Pinout & Configuration:
   - Connectivity > I2C1: set Mode = "I2C" (uses PB8=SCL, PB9=SDA by default)
   - Connectivity > USART2: already enabled via ST-Link (PA2=TX, PA3=RX)
   - System Core > IWDG: set Activated = checked
     - Prescaler: 256
     - Reload counter: 249  (gives ~2s timeout at 32kHz LSI)
4. Clock Configuration:
   - Input: HSE (8MHz from ST-Link)
   - PLL: configured to 100MHz SYSCLK (CubeMX does this automatically for F411RE)
5. Project Manager:
   - Project Name: freertos-stm32-hal (save somewhere temporary)
   - Toolchain: Makefile (we only want the generated .c/.h files, not the Makefile)
   - Do NOT enable FreeRTOS in CubeMX (we manage it ourselves)
6. Generate Code

After generation, copy these into this project's third_party/ folder:
```
third_party/
  STM32F4xx_HAL_Driver/
    Inc/     (copy from <cubemx_output>/Drivers/STM32F4xx_HAL_Driver/Inc/)
    Src/     (copy from <cubemx_output>/Drivers/STM32F4xx_HAL_Driver/Src/)
  CMSIS/
    Include/ (copy from <cubemx_output>/Drivers/CMSIS/Include/)
    Device/  (copy from <cubemx_output>/Drivers/CMSIS/Device/)
```

Also copy the generated init functions into a separate file. In `src/hal_init.c`:
- The contents of `main.c` from CubeMX output (SystemClock_Config, MX_GPIO_Init, etc.)
- Add `#include "hal_init.h"` to main.c and declare externs there

---

## Step 4 - Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake
make -j$(nproc)
```

Successful build output ends with something like:
```
Memory region    Used Size  Region Size  %age Used
           FLASH:   24680 B       384 KB      6.28%
             RAM:   10240 B       128 KB      7.81%
```

---

## Step 5 - Flash

### Via ST-Link (recommended)

Install OpenOCD:
```bash
brew install openocd
```

Flash:
```bash
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "program build/freertos-stm32.bin verify reset exit 0x08000000"
```

### Via STM32CubeProgrammer (GUI)

Download from ST website. Connect Nucleo via USB, open .bin or .hex file, program.

---

## Step 6 - Monitor UART Output

The Nucleo's ST-Link exposes UART2 as a USB virtual COM port.

```bash
# Find the port
ls /dev/tty.usbmodem*

# Connect at 115200 baud
screen /dev/tty.usbmodem* 115200
# or
minicom -D /dev/tty.usbmodem* -b 115200
```

Expected output after boot:
```
[CLI] ready. type 'help' for commands.
[DATA] t=1000ms T=23.50C H=45.00% P=101325Pa
[DATA] t=2000ms T=23.51C H=45.01% P=101326Pa
```

---

## Step 7 - Verify

Work through the verification checklist in project-outline.md once hardware is connected.

Key tests:
1. Type `tasks` in the CLI - should show all 5 tasks with state and stack watermark
2. Type `status` - should show stats
3. Type `config set temp_offset_cdeg 50` then `config get` - verify persistence across reset
4. Suspend a task (use `vTaskSuspend` from CLI) and verify watchdog resets the system
