#include "uart_task.h"
#include "app_types.h"
#include "watchdog_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*
 * NOTE: HAL_UART_Transmit uses huart2 (the Nucleo's ST-Link UART).
 * When CubeMX generates the project, declare it extern here:
 *   extern UART_HandleTypeDef huart2;
 */

/* -------------------------------------------------------------------------
 * uart_task
 *
 * Single consumer of g_print_queue. Blocks waiting for print_msg_t items
 * and transmits them over UART2.
 *
 * Why a dedicated UART task:
 *   HAL_UART_Transmit is blocking. At 115200 baud, sending 96 bytes takes
 *   ~8ms. Blocking the sensor or processing task for 8ms every print would
 *   break their timing. Dedicated low-priority task absorbs this delay.
 * ------------------------------------------------------------------------- */
void uart_task(void *pvParameters)
{
    (void)pvParameters;

    print_msg_t msg;

    for (;;)
    {
        /* Block with bounded timeout so watchdog gets fed even when idle */
        if (xQueueReceive(g_print_queue, &msg, pdMS_TO_TICKS(400)) != pdTRUE)
        {
            watchdog_task_checkin(WDG_BIT_UART);
            continue;
        }

        /* Transmit over UART2 */
        /* HAL_UART_Transmit(&huart2,               */
        /*     (uint8_t *)msg.buf,                   */
        /*     msg.len,                              */
        /*     UART_TX_TIMEOUT_MS);                  */
        /* TODO: uncomment when HAL is present       */

        watchdog_task_checkin(WDG_BIT_UART);
    }
}
