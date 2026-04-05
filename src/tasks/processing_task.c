#include "processing_task.h"
#include "app_types.h"
#include "drivers/flash_config.h"
#include "watchdog_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

/* -------------------------------------------------------------------------
 * processing_task
 *
 * Blocks on g_sensor_queue waiting for a reading from sensor_task.
 * Applies a calibration offset from persistent config.
 * Formats the reading as a string and sends to g_print_queue.
 * ------------------------------------------------------------------------- */
void processing_task(void *pvParameters)
{
    (void)pvParameters;

    sensor_reading_t reading;

    for (;;)
    {
        /* Block with bounded timeout so watchdog gets fed even when idle */
        if (xQueueReceive(g_sensor_queue, &reading, pdMS_TO_TICKS(400)) != pdTRUE)
        {
            watchdog_task_checkin(WDG_BIT_PROCESSING);
            continue;
        }

        if (!reading.valid)
        {
            /* Sensor error sentinel - report it */
            QUEUE_PRINT("[SENSOR] ERROR: persistent read failure\r\n");
            watchdog_task_checkin(WDG_BIT_PROCESSING);
            continue;
        }

        /* Apply calibration offset from config */
        device_config_t cfg;
        flash_config_read(&cfg);
        int32_t calibrated_temp = reading.temperature_cdeg + cfg.temp_offset_cdeg;

        /* Format the output string
           Temperature: divide by 100 for integer part, modulo for decimal
           Humidity: divide by 100 for integer part */
        int32_t temp_int  = calibrated_temp / 100;
        int32_t temp_frac = (calibrated_temp < 0)
                          ? -(calibrated_temp % 100)
                          :  (calibrated_temp % 100);

        uint32_t hum_int  = reading.humidity_cpct / 100;
        uint32_t hum_frac = reading.humidity_cpct % 100;

        QUEUE_PRINT("[DATA] t=%lums T=%ld.%02ldC H=%lu.%02lu%% P=%luPa\r\n",
                    (unsigned long)reading.timestamp_ms,
                    (long)temp_int, (long)temp_frac,
                    (unsigned long)hum_int, (unsigned long)hum_frac,
                    (unsigned long)reading.pressure_pa);

        /* Update uptime counter (approximation: one reading per second) */
        xSemaphoreTake(g_stats_mutex, portMAX_DELAY);
        g_stats.uptime_s++;
        xSemaphoreGive(g_stats_mutex);

        watchdog_task_checkin(WDG_BIT_PROCESSING);
    }
}
