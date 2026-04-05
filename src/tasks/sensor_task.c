#include "sensor_task.h"
#include "app_types.h"
#include "drivers/bme280.h"
#include "watchdog_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* -------------------------------------------------------------------------
 * sensor_task
 *
 * Reads the BME280 sensor every SENSOR_TASK_PERIOD_MS using vTaskDelayUntil
 * so the period is accurate regardless of how long the read takes.
 *
 * On success: sends a valid sensor_reading_t to g_sensor_queue.
 * On failure: increments fail counter. After SENSOR_MAX_CONSECUTIVE_FAIL
 *             consecutive failures, sends an error sentinel (valid=0) so
 *             downstream tasks know the sensor is unhealthy.
 * ------------------------------------------------------------------------- */
void sensor_task(void *pvParameters)
{
    (void)pvParameters;

    uint8_t consecutive_fails = 0;
    TickType_t last_wake = xTaskGetTickCount();

    /* Wait for the sensor to finish power-on reset before first read */
    vTaskDelay(pdMS_TO_TICKS(100));

    for (;;)
    {
        /* Block until our next scheduled wake time.
           last_wake is updated by vTaskDelayUntil to the actual wake tick. */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));

        sensor_reading_t reading = {0};
        reading.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        status_t result = bme280_read(&reading.temperature_cdeg,
                                      &reading.humidity_cpct,
                                      &reading.pressure_pa);

        if (result == STATUS_OK)
        {
            reading.valid      = 1;
            consecutive_fails  = 0;

            /* Update stats */
            xSemaphoreTake(g_stats_mutex, portMAX_DELAY);
            g_stats.sensor_read_ok++;
            xSemaphoreGive(g_stats_mutex);
        }
        else
        {
            consecutive_fails++;

            xSemaphoreTake(g_stats_mutex, portMAX_DELAY);
            g_stats.sensor_read_fail++;
            xSemaphoreGive(g_stats_mutex);

            if (consecutive_fails < SENSOR_MAX_CONSECUTIVE_FAIL)
            {
                /* Transient error - skip this sample, try again next period */
                watchdog_task_checkin(WDG_BIT_SENSOR);
                continue;
            }

            /* Persistent error - send sentinel so processing task can react */
            reading.valid = 0;
        }

        /* Send to processing task. Non-blocking (0 timeout): if the queue
           is full, drop this reading and count it. Stale data is worse than
           a gap. */
        if (xQueueSend(g_sensor_queue, &reading, 0) != pdPASS)
        {
            xSemaphoreTake(g_stats_mutex, portMAX_DELAY);
            g_stats.queue_drops++;
            xSemaphoreGive(g_stats_mutex);
        }

        /* Signal watchdog that this task is alive */
        watchdog_task_checkin(WDG_BIT_SENSOR);
    }
}
