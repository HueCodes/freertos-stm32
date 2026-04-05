cmake_minimum_required(VERSION 3.22)

# Toolchain must be set before project() call
include(cmake/arm-none-eabi.cmake OPTIONAL)

project(freertos-stm32 C ASM)

include(cmake/stm32f411.cmake)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------

set(APP_SOURCES
    src/main.c
    src/tasks/sensor_task.c
    src/tasks/processing_task.c
    src/tasks/uart_task.c
    src/tasks/watchdog_task.c
    src/tasks/cli_task.c
    src/drivers/bme280.c
    src/drivers/flash_config.c
)

# FreeRTOS sources - these live in third_party/FreeRTOS after you clone it.
# See README for setup instructions.
set(FREERTOS_DIR ${CMAKE_SOURCE_DIR}/third_party/FreeRTOS/Source)

set(FREERTOS_SOURCES
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
)

# STM32 HAL and CMSIS sources - generated/fetched by CubeMX or manually.
# Set this path after CubeMX project generation.
# See README for setup instructions.
set(HAL_DIR ${CMAKE_SOURCE_DIR}/third_party/STM32F4xx_HAL_Driver)
set(CMSIS_DIR ${CMAKE_SOURCE_DIR}/third_party/CMSIS)

set(HAL_SOURCES
    ${HAL_DIR}/Src/stm32f4xx_hal.c
    ${HAL_DIR}/Src/stm32f4xx_hal_cortex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_rcc.c
    ${HAL_DIR}/Src/stm32f4xx_hal_gpio.c
    ${HAL_DIR}/Src/stm32f4xx_hal_i2c.c
    ${HAL_DIR}/Src/stm32f4xx_hal_uart.c
    ${HAL_DIR}/Src/stm32f4xx_hal_flash.c
    ${HAL_DIR}/Src/stm32f4xx_hal_flash_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_iwdg.c
    ${HAL_DIR}/Src/stm32f4xx_hal_pwr.c
    ${HAL_DIR}/Src/stm32f4xx_hal_pwr_ex.c
    # Startup file (assembly)
    ${CMSIS_DIR}/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s
    # System init (sets up SystemCoreClock)
    ${CMSIS_DIR}/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
)

# ---------------------------------------------------------------------------
# Executable target
# ---------------------------------------------------------------------------

add_executable(${PROJECT_NAME}.elf
    ${APP_SOURCES}
    ${FREERTOS_SOURCES}
    ${HAL_SOURCES}
)

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    src/
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
    ${HAL_DIR}/Inc
    ${CMSIS_DIR}/Include
    ${CMSIS_DIR}/Device/ST/STM32F4xx/Include
)

# ---------------------------------------------------------------------------
# Compile flags
# ---------------------------------------------------------------------------

target_compile_options(${PROJECT_NAME}.elf PRIVATE ${C_FLAGS})

# ---------------------------------------------------------------------------
# Link flags
# ---------------------------------------------------------------------------

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${LINKER_FLAGS}
    -T${LINKER_SCRIPT}
)

# ---------------------------------------------------------------------------
# Post-build: generate .bin and .hex files for flashing
# ---------------------------------------------------------------------------

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE}              ${PROJECT_NAME}.elf
    COMMENT "Generating ${PROJECT_NAME}.bin and ${PROJECT_NAME}.hex"
)
