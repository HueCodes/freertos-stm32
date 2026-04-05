#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*
 * FreeRTOSConfig.h - Project-specific FreeRTOS configuration
 *
 * Every #define here overrides a default in FreeRTOS/Source/include/FreeRTOS.h.
 * Read the comments - understanding these settings is part of the project.
 */

/* -------------------------------------------------------------------------
 * Scheduler behaviour
 * ------------------------------------------------------------------------- */

/* 1 = preemptive scheduler. The scheduler can switch tasks at any tick.
   0 = cooperative: tasks only switch when they yield or block. Always use 1. */
#define configUSE_PREEMPTION                    1

/* 1 = tasks at equal priority time-slice (round-robin).
   0 = only yield when blocked. Always use 1. */
#define configUSE_TIME_SLICING                  1

/* Idle task runs when no other task is ready. We hook it for low-power sleep. */
#define configUSE_IDLE_HOOK                     1

/* Tick hook runs every tick interrupt. Not needed here. */
#define configUSE_TICK_HOOK                     0

/* -------------------------------------------------------------------------
 * Clock and tick
 * ------------------------------------------------------------------------- */

/* Must match your actual CPU clock. CubeMX configures 100MHz for F411RE. */
#define configCPU_CLOCK_HZ                      100000000UL

/* Tick rate: 1000 = 1ms tick resolution. pdMS_TO_TICKS(x) = x ticks.
   Higher tick rate = finer timing + more overhead. 1000Hz is standard. */
#define configTICK_RATE_HZ                      1000

/* -------------------------------------------------------------------------
 * Task priorities and stack
 * ------------------------------------------------------------------------- */

/* Highest valid priority value. Our project uses up to 4. */
#define configMAX_PRIORITIES                    8

/* Stack size for the idle task. Do not reduce below 128. */
#define configMINIMAL_STACK_SIZE                128

/* -------------------------------------------------------------------------
 * Heap
 * ------------------------------------------------------------------------- */

/* Total FreeRTOS heap size in bytes. All task stacks, queues, and semaphores
   come from here. 24KB of our 128KB RAM. */
#define configTOTAL_HEAP_SIZE                   (24 * 1024)

/* -------------------------------------------------------------------------
 * Synchronisation primitives
 * ------------------------------------------------------------------------- */

#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0   /* not needed here */
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0

/* -------------------------------------------------------------------------
 * Software timers
 * ------------------------------------------------------------------------- */

#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            256

/* -------------------------------------------------------------------------
 * Debug and trace
 * ------------------------------------------------------------------------- */

/* Required for vTaskList() - prints task state table over UART. Very useful. */
#define configUSE_TRACE_FACILITY                1

/* Required for vTaskGetRunTimeStats() - CPU usage per task. */
#define configGENERATE_RUN_TIME_STATS           0   /* enable when needed */

/* Required for vTaskList() and vTaskGetRunTimeStats() output formatting. */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* Stack overflow detection:
   1 = check stack pointer on context switch (fast, misses some overflows)
   2 = fill stack with 0xA5 pattern, check last 20 bytes (slower, catches more)
   Always use 2 during development. */
#define configCHECK_FOR_STACK_OVERFLOW          2

/* Called when pvPortMalloc fails. Never silently ignore heap exhaustion. */
#define configUSE_MALLOC_FAILED_HOOK            1

/* -------------------------------------------------------------------------
 * API inclusion - set to 1 to include, 0 to exclude (saves flash)
 * ------------------------------------------------------------------------- */

#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1   /* needed for periodic tasks */
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1   /* needed for watermark checks */
#define INCLUDE_xQueueGetMutexHolder            1
#define INCLUDE_eTaskGetState                   1

/* -------------------------------------------------------------------------
 * Cortex-M4 specific: interrupt priority configuration
 *
 * The Cortex-M4 has configurable interrupt priority bits. STM32F4 uses 4 bits
 * (16 priority levels). FreeRTOS uses the lowest N bits as "sub-priority".
 *
 * configMAX_SYSCALL_INTERRUPT_PRIORITY: interrupts at this priority or HIGHER
 * (lower numerical value) cannot use FreeRTOS ISR-safe API. Set this high
 * enough that your critical hardware ISRs (e.g. motor PWM) are above it.
 *
 * In CMSIS notation, priority 5 means the value written to the NVIC register
 * is (5 << (8 - configPRIO_BITS)) = (5 << 4) = 0x50.
 * ------------------------------------------------------------------------- */

#define configPRIO_BITS                         4   /* STM32F4: 4 priority bits */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY     15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY     ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* FreeRTOS uses PendSV and SysTick. Map them to the CMSIS handler names. */
#define xPortPendSVHandler      PendSV_Handler
#define vPortSVCHandler         SVC_Handler
#define xPortSysTickHandler     SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
