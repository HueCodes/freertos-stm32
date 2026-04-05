#include "cli_task.h"
#include "app_types.h"
#include "drivers/flash_config.h"
#include "watchdog_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
 * cli_uart_rx_isr_callback
 *
 * Called from UART RX ISR. Pushes a single byte into g_rx_char_queue.
 * Uses the FromISR variant of xQueueSend - mandatory from interrupt context.
 * ------------------------------------------------------------------------- */
void cli_uart_rx_isr_callback(uint8_t byte)
{
    BaseType_t higher_prio_woken = pdFALSE;
    xQueueSendFromISR(g_rx_char_queue, &byte, &higher_prio_woken);
    /* Yield to a higher-priority task if sending unblocked one */
    portYIELD_FROM_ISR(higher_prio_woken);
}

/* -------------------------------------------------------------------------
 * Command handlers (forward declarations)
 * ------------------------------------------------------------------------- */
static void cmd_status(void);
static void cmd_config_get(void);
static void cmd_config_set(char *key, char *value);
static void cmd_tasks(void);
static void cmd_reset(void);
static void cmd_help(void);

/* -------------------------------------------------------------------------
 * cli_task
 *
 * Accumulates bytes from g_rx_char_queue into a line buffer.
 * Dispatches to command handlers when '\n' or '\r' is received.
 * ------------------------------------------------------------------------- */
void cli_task(void *pvParameters)
{
    (void)pvParameters;

    char    cmd_buf[CLI_CMD_BUF_LEN];
    uint8_t idx  = 0;
    uint8_t byte = 0;

    QUEUE_PRINT("\r\n[CLI] ready. type 'help' for commands.\r\n");

    for (;;)
    {
        /* Block with bounded timeout so watchdog gets fed even when idle */
        if (xQueueReceive(g_rx_char_queue, &byte, pdMS_TO_TICKS(400)) != pdTRUE)
        {
            watchdog_task_checkin(WDG_BIT_CLI);
            continue;
        }

        if (byte == '\r' || byte == '\n')
        {
            if (idx == 0)
            {
                watchdog_task_checkin(WDG_BIT_CLI);
                continue;   /* ignore empty lines */
            }

            cmd_buf[idx] = '\0';
            idx = 0;

            /* Parse command */
            char *token = strtok(cmd_buf, " ");
            if (!token) { watchdog_task_checkin(WDG_BIT_CLI); continue; }

            if      (strcmp(token, "status") == 0) { cmd_status(); }
            else if (strcmp(token, "config") == 0)
            {
                char *subcmd = strtok(NULL, " ");
                if (!subcmd || strcmp(subcmd, "get") == 0)
                {
                    cmd_config_get();
                }
                else if (strcmp(subcmd, "set") == 0)
                {
                    char *key = strtok(NULL, " ");
                    char *val = strtok(NULL, " ");
                    if (key && val) { cmd_config_set(key, val); }
                    else { QUEUE_PRINT("[CLI] usage: config set <key> <value>\r\n"); }
                }
            }
            else if (strcmp(token, "tasks")  == 0) { cmd_tasks(); }
            else if (strcmp(token, "reset")  == 0) { cmd_reset(); }
            else if (strcmp(token, "help")   == 0) { cmd_help(); }
            else
            {
                QUEUE_PRINT("[CLI] unknown command: %s\r\n", token);
            }
        }
        else if (byte == 0x7F || byte == '\b')  /* backspace */
        {
            if (idx > 0) { idx--; }
        }
        else if (idx < CLI_CMD_BUF_LEN - 1)
        {
            cmd_buf[idx++] = (char)byte;
        }

        watchdog_task_checkin(WDG_BIT_CLI);
    }
}

/* -------------------------------------------------------------------------
 * Command implementations
 * ------------------------------------------------------------------------- */

static void cmd_status(void)
{
    xSemaphoreTake(g_stats_mutex, portMAX_DELAY);
    system_stats_t s = g_stats;     /* copy under mutex, then release quickly */
    xSemaphoreGive(g_stats_mutex);

    QUEUE_PRINT("[STATUS] uptime=%lus ok=%lu fail=%lu drops=%lu flash_ok=%lu flash_fail=%lu\r\n",
                (unsigned long)s.uptime_s,
                (unsigned long)s.sensor_read_ok,
                (unsigned long)s.sensor_read_fail,
                (unsigned long)s.queue_drops,
                (unsigned long)s.flash_write_ok,
                (unsigned long)s.flash_write_fail);
}

static void cmd_config_get(void)
{
    device_config_t cfg;
    if (flash_config_read(&cfg) == STATUS_OK)
    {
        QUEUE_PRINT("[CONFIG] temp_offset_cdeg=%ld sample_period_ms=%lu baud=%lu\r\n",
                    (long)cfg.temp_offset_cdeg,
                    (unsigned long)cfg.sample_period_ms,
                    (unsigned long)cfg.uart_baud);
    }
    else
    {
        QUEUE_PRINT("[CONFIG] using defaults (no valid config in flash)\r\n");
    }
}

static void cmd_config_set(char *key, char *value)
{
    device_config_t cfg;
    flash_config_read(&cfg);   /* read current (may be defaults) */

    if (strcmp(key, "temp_offset_cdeg") == 0)
    {
        cfg.temp_offset_cdeg = (int32_t)strtol(value, NULL, 10);
    }
    else if (strcmp(key, "sample_period_ms") == 0)
    {
        cfg.sample_period_ms = (uint32_t)strtoul(value, NULL, 10);
    }
    else
    {
        QUEUE_PRINT("[CONFIG] unknown key: %s\r\n", key);
        return;
    }

    status_t result = flash_config_write(&cfg);
    if (result == STATUS_OK)
    {
        QUEUE_PRINT("[CONFIG] written ok\r\n");
        xSemaphoreTake(g_stats_mutex, portMAX_DELAY);
        g_stats.flash_write_ok++;
        xSemaphoreGive(g_stats_mutex);
    }
    else
    {
        QUEUE_PRINT("[CONFIG] write failed\r\n");
        xSemaphoreTake(g_stats_mutex, portMAX_DELAY);
        g_stats.flash_write_fail++;
        xSemaphoreGive(g_stats_mutex);
    }
}

static void cmd_tasks(void)
{
    /* vTaskList prints a table: name, state, priority, stack watermark, task number.
       Requires configUSE_TRACE_FACILITY=1 and configUSE_STATS_FORMATTING_FUNCTIONS=1 */
    static char task_buf[512];  /* static so it does not go on the task stack */
    vTaskList(task_buf);
    /* Send in chunks that fit in PRINT_BUF_LEN */
    size_t total = strlen(task_buf);
    size_t offset = 0;
    while (offset < total) {
        size_t chunk = total - offset;
        if (chunk > PRINT_BUF_LEN - 1) { chunk = PRINT_BUF_LEN - 1; }
        print_msg_t _msg;
        memcpy(_msg.buf, task_buf + offset, chunk);
        _msg.buf[chunk] = '\0';
        _msg.len = (uint16_t)chunk;
        xQueueSend(g_print_queue, &_msg, pdMS_TO_TICKS(100));
        offset += chunk;
    }
}

static void cmd_reset(void)
{
    QUEUE_PRINT("[CLI] resetting...\r\n");
    vTaskDelay(pdMS_TO_TICKS(50));          /* flush UART */
    /* NVIC_SystemReset(); */               /* TODO: uncomment when CMSIS is present */
}

static void cmd_help(void)
{
    QUEUE_PRINT(
        "commands:\r\n"
        "  status                 - print stats\r\n"
        "  config get             - print config\r\n"
        "  config set <key> <val> - set and write config\r\n"
        "  tasks                  - print FreeRTOS task list\r\n"
        "  reset                  - software reset\r\n"
    );
}
