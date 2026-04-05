# stm32f411.cmake - MCU-specific compile and link flags for STM32F411RE

# -mcpu=cortex-m4          : target CPU
# -mthumb                  : use Thumb-2 instruction set (smaller code)
# -mfpu=fpv4-sp-d16        : hardware FPU (single precision, 16 d-registers)
# -mfloat-abi=hard         : pass float args in FPU registers (faster)
set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# Compiler flags for C
set(C_FLAGS
    ${MCU_FLAGS}
    -ffunction-sections     # each function in its own ELF section
    -fdata-sections         # each variable in its own ELF section
    -fno-common             # do not merge common symbols
    -Wall                   # enable common warnings
    -Wextra                 # enable extra warnings
    -Wno-unused-parameter   # FreeRTOS uses (void)pvParameters a lot
    -std=c11
    -DSTM32F411xE           # selects correct stm32f4xx.h variant
    -DUSE_HAL_DRIVER        # enables HAL includes
)

# Linker flags
set(LINKER_FLAGS
    ${MCU_FLAGS}
    -Wl,--gc-sections           # remove unused code/data sections
    -Wl,-Map=${PROJECT_NAME}.map # generate memory map file
    -specs=nosys.specs          # no OS syscalls (no semihosting)
    -specs=nano.specs           # newlib-nano (smaller printf/malloc)
    -Wl,--print-memory-usage    # print RAM/flash usage after link
)

# Linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/STM32F411RETx_FLASH.ld)
