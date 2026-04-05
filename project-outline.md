# FreeRTOS STM32 - Project Outline

## Goal

Build a multi-task FreeRTOS firmware on STM32 that demonstrates production-relevant
patterns: task communication via queues, ISR-safe data passing, watchdog supervision,
and persistent configuration in flash. Every pattern here appears in real firmware jobs.

---

## Hardware

**Primary board:** STM32 Nucleo-64 with STM32L476RG or STM32F446RE.
- L476RG preferred: low-power peripherals, good for showing sleep modes later
- F446RE alternative: faster, more common in tutorials if you get stuck

**Peripherals needed:**
- Any I2C or SPI sensor (BME280 temp/humidity/pressure is cheap and well-documented,
  or an MPU-6050 IMU if you have one)
- USB-UART adapter or use the Nucleo's onboard ST-Link UART
- Optional: small SSD1306 OLED (I2C) for visual output without a terminal

**Tools:**
- STM32CubeIDE or CLion + CMake + arm-none-eabi-gcc
- OpenOCD or ST-Link for flashing and GDB debugging
- Logic analyzer for validating SPI/I2C timing

---

## Architecture Overview

```
+------------------+     queue      +-------------------+
|  Sensor Task     | ------------> |  Processing Task   |
|  (periodic, I2C) |               |  (formats data)    |
+------------------+               +-------------------+
                                            |
                                            | queue
                                            v
+------------------+               +-------------------+
|  Watchdog Task   |               |  UART Print Task  |
|  (supervises all)|               |  (serial output)  |
+------------------+               +-------------------+
         ^
         | task checkin flags
         |
  [all tasks check in or system resets]
```

One ISR (hardware timer or GPIO EXTI) feeds data into the sensor queue directly
using xQueueSendFromISR to demonstrate ISR-safe queue usage.

---

## Tasks

### 1. Sensor Task
**Priority:** Normal (2)
**Stack:** 512 words
**Period:** 1000ms (vTaskDelayUntil for accurate periodicity)

Responsibilities:
- Reads sensor over I2C (blocking, with timeout)
- Packages reading into a struct with a timestamp (xTaskGetTickCount)
- Sends struct to processing queue (non-blocking, drop if full)
- Sets its watchdog checkin flag

```c
typedef struct {
    uint32_t timestamp_ms;
    int32_t  temperature_cdeg;  // centidegrees
    uint32_t humidity_cpct;     // centi-percent
    uint32_t pressure_pa;
} sensor_reading_t;
```

Failure behavior: if I2C read fails 3 consecutive times, sends an error sentinel
value and increments an error counter in a shared stats struct.

---

### 2. Processing Task
**Priority:** Normal (2)
**Stack:** 512 words

Responsibilities:
- Blocks on xQueueReceive from sensor queue (portMAX_DELAY)
- Applies simple calibration offset from config (read from flash at boot)
- Formats reading into a human-readable string
- Sends string to UART print queue
- Sets its watchdog checkin flag

This task is intentionally simple - the point is demonstrating the pipeline pattern,
not doing complex math.

---

### 3. UART Print Task
**Priority:** Low (1)
**Stack:** 256 words

Responsibilities:
- Blocks on xQueueReceive from print queue
- Writes to UART via HAL_UART_Transmit (or DMA variant)
- Sets its watchdog checkin flag

Why a dedicated print task: UART is slow. Blocking the sensor or processing task
on UART TX would break timing. This pattern appears in almost all real firmware.

---

### 4. Watchdog Supervisor Task
**Priority:** High (4) - highest in the system
**Stack:** 256 words
**Period:** 500ms

Responsibilities:
- Every 500ms, checks that all other tasks have set their checkin flags
- If all flags set: clears flags, feeds IWDG (HAL_IWDG_Refresh)
- If any flag missing after deadline: does NOT feed watchdog, logs which task failed,
  system resets via watchdog timeout

Implementation:
- Use a volatile uint32_t bitmask, one bit per task
- Each task calls a checkin function that sets its bit atomically
- Supervisor checks all bits set, clears with a single write

This is the pattern used in real safety-conscious firmware. A watchdog fed
unconditionally from one place is nearly useless.

---

### 5. CLI Task (stretch goal)
**Priority:** Normal (2)
**Stack:** 512 words

A simple UART command shell. Commands:
- `status` - print current sensor reading, uptime, error counts
- `config get` - print current config values
- `config set <key> <value>` - update config, write to flash
- `reset` - software reset via NVIC_SystemReset

Receiving input from UART RX interrupt (or DMA circular buffer) feeding a character
queue that the CLI task reads. This demonstrates bidirectional UART with FreeRTOS.

---

## Inter-Task Communication

| Queue | Producer | Consumer | Depth | Item Type |
|-------|----------|----------|-------|-----------|
| sensor_queue | Sensor Task | Processing Task | 4 | sensor_reading_t |
| print_queue | Processing Task, CLI Task | UART Print Task | 8 | char[80] |
| rx_char_queue | UART RX ISR | CLI Task | 32 | char |

Queue depth choices:
- sensor_queue depth 4: if processing falls behind, we buffer 4 readings before dropping.
  Dropping is intentional - stale sensor data is worse than no data.
- print_queue depth 8: allows burst output without blocking producers
- rx_char_queue depth 32: matches a typical command length

---

## Persistent Configuration (Flash Storage)

Store a config struct in the last flash page (STM32L476 has 2KB pages).

```c
typedef struct {
    uint32_t magic;              // 0xDEADBEEF - validates config is written
    uint32_t version;            // increment on struct layout changes
    int32_t  temp_offset_cdeg;   // calibration offset
    uint32_t sample_period_ms;   // default 1000
    uint32_t uart_baud;          // default 115200
    uint32_t crc32;              // CRC of above fields
} device_config_t;
```

Operations:
- Read at boot: validate magic + CRC, fall back to defaults if invalid
- Write: erase page, write struct with updated CRC
- Wear leveling: not needed for this project (config writes are rare),
  but worth noting the concept

This teaches flash erase/program cycles, alignment requirements (STM32 writes
must be aligned), and CRC validation - all real production concerns.

---

## Watchdog Configuration

IWDG (Independent Watchdog) on STM32: runs on its own 32kHz LSI clock,
independent of the main system clock. Cannot be disabled once started.

Configuration:
- Prescaler: /256
- Reload value: calculated for ~2 second timeout
- Feed window: only within the supervisor task's 500ms cycle

Why IWDG over WWDG: IWDG is truly independent (keeps running in stop/standby modes).
WWDG (Window Watchdog) is clocked from APB and is better for catching runaway code
within a window - worth understanding the difference but IWDG is more commonly required.

---

## Error Handling Strategy

Every function that can fail returns a status code. No silent failures.

```c
typedef enum {
    STATUS_OK = 0,
    STATUS_SENSOR_TIMEOUT,
    STATUS_SENSOR_CSUM_ERR,
    STATUS_QUEUE_FULL,
    STATUS_FLASH_ERR,
} status_t;
```

Global stats struct (access protected by a mutex):
```c
typedef struct {
    uint32_t sensor_read_ok;
    uint32_t sensor_read_fail;
    uint32_t queue_drops;
    uint32_t flash_write_ok;
    uint32_t flash_write_fail;
    uint32_t uptime_s;
} system_stats_t;
```

Print stats on `status` CLI command and on every watchdog check cycle to UART.

---

## Memory Budget

STM32L476RG: 128KB RAM, 1MB flash.

| Region | Allocation |
|--------|-----------|
| FreeRTOS heap (heap_4) | 24KB |
| Sensor task stack | 2KB |
| Processing task stack | 2KB |
| UART print task stack | 1KB |
| Watchdog task stack | 1KB |
| CLI task stack | 2KB |
| FreeRTOS kernel overhead | ~1KB |
| Static globals + BSS | ~2KB |
| Total RAM | ~35KB of 128KB |

heap_4 is the right allocator for most embedded use: it coalesces free blocks
(unlike heap_2) and is deterministic enough for non-safety-critical firmware.
For this project, after boot all allocations are static - pvPortMalloc is only
called during task creation.

---

## Build System

CMake + arm-none-eabi-gcc. Not STM32CubeIDE's generated Makefile.

Reasons:
- CMake is what embedded jobs use in practice
- Easier to add FreeRTOS as a subdirectory (CMakeLists.txt) rather than patching
  a generated Makefile every time you regenerate from CubeMX
- Integrates with CLion, VS Code, or command line

Structure:
```
freertos-stm32/
  CMakeLists.txt
  cmake/
    stm32l476.cmake       # MCU flags, linker script
    freertos.cmake        # FreeRTOS sources and include paths
  src/
    main.c
    tasks/
      sensor_task.c/h
      processing_task.c/h
      uart_task.c/h
      watchdog_task.c/h
      cli_task.c/h
    drivers/
      bme280.c/h          # sensor driver
      flash_config.c/h    # config storage
    freertos/
      FreeRTOSConfig.h    # project-specific config
  third_party/
    FreeRTOS/             # git submodule
    STM32L4xx_HAL/        # CMSIS + HAL sources
  linker/
    STM32L476RGTx_FLASH.ld
```

---

## FreeRTOSConfig.h Key Settings

Settings worth understanding and setting deliberately:

```c
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     1   // for low-power sleep
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      80000000UL
#define configTICK_RATE_HZ                      1000    // 1ms tick
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                128
#define configTOTAL_HEAP_SIZE                   (24 * 1024)
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TRACE_FACILITY                1   // enables vTaskList
#define configGENERATE_RUN_TIME_STATS           1   // CPU usage per task
#define configCHECK_FOR_STACK_OVERFLOW          2   // pattern check, not just watermark
#define configUSE_MALLOC_FAILED_HOOK            1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
```

configCHECK_FOR_STACK_OVERFLOW 2 fills the stack with a known pattern at creation
and checks the pattern on context switch. Slower than mode 1 but catches overflows
that mode 1 misses. Always use in development.

---

## Testing and Verification

Without a test framework, verify via observable outputs:

1. **Task timing:** print timestamp with every sensor reading - verify 1000ms periodicity
   does not drift (vTaskDelayUntil vs vTaskDelay: understand the difference)

2. **Queue backpressure:** artificially slow the processing task (vTaskDelay inside
   processing loop), verify sensor_queue fills and readings are dropped rather than
   blocking the sensor task indefinitely

3. **Watchdog:** suspend a task from the CLI (`suspend <taskname>`), verify system
   resets within watchdog timeout window, verify UART logs which task failed to check in

4. **Stack watermark:** after running 5+ minutes, call uxTaskGetStackHighWaterMark
   for each task and print. Verify no task is within 10% of its stack limit.

5. **Flash config:** write a config value, power cycle, verify value persists.
   Corrupt the flash page manually (write bad CRC), verify firmware falls back to
   defaults cleanly.

---

## Write-up Angle for the Site

The interesting narrative: FreeRTOS primitives are simple. What is hard is
designing around failure - what happens when a sensor hangs, when a queue fills,
when a task starves. The watchdog supervisor pattern and the queue depth rationale
are the things worth writing about. Most tutorials stop at "tasks blink LEDs at
different rates." This one should show a system that can fail gracefully.

---

## Estimated Scope

- CMake build system setup: half a day
- FreeRTOS integration and first task running: half a day
- Sensor driver (BME280 I2C): half a day (driver already exists, integrate it)
- All tasks + queues + watchdog: 1-2 days
- Flash config: half a day
- CLI: 1 day
- Testing + write-up: 1 day

Total: roughly a week of part-time work for a solid, write-up-worthy result.
