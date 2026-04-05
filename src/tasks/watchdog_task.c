#include "watchdog_task.h"
#include "app_types.h"
#include "FreeRTOS.h"
#include "task.h"

/*
 * NOTE: HAL_IWDG_Refresh(&hiwdg) is stubbed below.
 * When CubeMX generates the project, it creates hiwdg in main.c.
 * Declare it extern here once that file exists:
 *   extern IWDG_HandleTypeDef hiwdg;
 */

/* -------------------------------------------------------------------------
 * watchdog_task_checkin
 *
 * Called by each task to signal it is alive. Uses a critical section
 * (disables interrupts briefly) to ensure the bit-set is atomic.
 * Critical sections are short - this is the right tool here.
 * ------------------------------------------------------------------------- */
void watchdog_task_checkin(uint32_t task_bit)
{
    taskENTER_CRITICAL();
    g_watchdog_checkin_flags |= task_bit;
    taskEXIT_CRITICAL();
}

/* -------------------------------------------------------------------------
 * watchdog_task
 * ------------------------------------------------------------------------- */
void watchdog_task(void *pvParameters)
{
    (void)pvParameters;

    TickType_t last_wake = xTaskGetTickCount();

    for (;;)
    {
        /* Block until our period expires. vTaskDelayUntil keeps the period
           accurate regardless of how long the check takes. */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(WATCHDOG_CHECK_PERIOD_MS));

        /* Read and clear the checkin flags atomically */
        uint32_t flags;
        taskENTER_CRITICAL();
        flags = g_watchdog_checkin_flags;
        g_watchdog_checkin_flags = 0;
        taskEXIT_CRITICAL();

        if ((flags & WDG_ALL_BITS) == WDG_ALL_BITS)
        {
            /* All tasks checked in. Feed the watchdog. */
            /* HAL_IWDG_Refresh(&hiwdg); */   /* TODO: uncomment when HAL is present */
        }
        else
        {
            /* One or more tasks missed their checkin. Log which ones failed,
               then do NOT feed the watchdog. System resets at timeout (~2s). */
            if (!(flags & WDG_BIT_SENSOR))     { QUEUE_PRINT("[WDG] sensor task missed checkin\r\n"); }
            if (!(flags & WDG_BIT_PROCESSING)) { QUEUE_PRINT("[WDG] processing task missed checkin\r\n"); }
            if (!(flags & WDG_BIT_UART))       { QUEUE_PRINT("[WDG] uart task missed checkin\r\n"); }
            if (!(flags & WDG_BIT_CLI))        { QUEUE_PRINT("[WDG] cli task missed checkin\r\n"); }

            /* Give the UART task a moment to flush the message before reset */
            vTaskDelay(pdMS_TO_TICKS(200));

            /* Do not feed the watchdog. The IWDG will reset the system. */
        }
    }
}
