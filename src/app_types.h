#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* -------------------------------------------------------------------------
 * Sensor reading - produced by sensor_task, consumed by processing_task
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t timestamp_ms;      /* xTaskGetTickCount() at time of read       */
    int32_t  temperature_cdeg;  /* temperature in centi-degrees C (e.g. 2350 = 23.50C) */
    uint32_t humidity_cpct;     /* relative humidity in centi-percent (e.g. 4500 = 45.00%) */
    uint32_t pressure_pa;       /* pressure in Pascals                        */
    uint8_t  valid;             /* 1 = good read, 0 = sensor error sentinel   */
} sensor_reading_t;

/* -------------------------------------------------------------------------
 * Print buffer - produced by processing_task and cli_task, consumed by uart_task
 * ------------------------------------------------------------------------- */
#define PRINT_BUF_LEN 96

typedef struct {
    char     buf[PRINT_BUF_LEN];
    uint16_t len;
} print_msg_t;

/* -------------------------------------------------------------------------
 * Watchdog checkin bitmask - each task owns one bit
 * ------------------------------------------------------------------------- */
#define WDG_BIT_SENSOR      (1u << 0)
#define WDG_BIT_PROCESSING  (1u << 1)
#define WDG_BIT_UART        (1u << 2)
#define WDG_BIT_CLI         (1u << 3)
#define WDG_ALL_BITS        (WDG_BIT_SENSOR | WDG_BIT_PROCESSING | \
                             WDG_BIT_UART   | WDG_BIT_CLI)

/* -------------------------------------------------------------------------
 * System-wide statistics (protected by stats_mutex)
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t sensor_read_ok;
    uint32_t sensor_read_fail;
    uint32_t queue_drops;
    uint32_t flash_write_ok;
    uint32_t flash_write_fail;
    uint32_t uptime_s;
    uint32_t watchdog_resets;   /* incremented in config before reset, read on boot */
} system_stats_t;

/* -------------------------------------------------------------------------
 * Status codes used throughout the project
 * ------------------------------------------------------------------------- */
typedef enum {
    STATUS_OK = 0,
    STATUS_SENSOR_TIMEOUT,
    STATUS_SENSOR_CHECKSUM_ERR,
    STATUS_QUEUE_FULL,
    STATUS_FLASH_ERR,
    STATUS_FLASH_INVALID_CONFIG,
    STATUS_INVALID_ARG,
} status_t;

/* -------------------------------------------------------------------------
 * Global queue handles - defined in main.c, used everywhere
 * ------------------------------------------------------------------------- */
extern QueueHandle_t    g_sensor_queue;
extern QueueHandle_t    g_print_queue;
extern QueueHandle_t    g_rx_char_queue;

/* -------------------------------------------------------------------------
 * Global stats and mutex - defined in main.c
 * ------------------------------------------------------------------------- */
extern system_stats_t   g_stats;
extern SemaphoreHandle_t g_stats_mutex;

/* -------------------------------------------------------------------------
 * Watchdog checkin flags - written by tasks, read by watchdog_task
 * Access must be inside taskENTER_CRITICAL / taskEXIT_CRITICAL
 * ------------------------------------------------------------------------- */
extern volatile uint32_t g_watchdog_checkin_flags;

/* -------------------------------------------------------------------------
 * Convenience macro for printing to the uart queue
 * Returns STATUS_OK or STATUS_QUEUE_FULL
 * ------------------------------------------------------------------------- */
#define QUEUE_PRINT(fmt, ...)                                           \
    do {                                                                \
        print_msg_t _msg;                                               \
        _msg.len = (uint16_t)snprintf(_msg.buf, PRINT_BUF_LEN,         \
                                      fmt, ##__VA_ARGS__);              \
        if (_msg.len >= PRINT_BUF_LEN) { _msg.len = PRINT_BUF_LEN-1; } \
        xQueueSend(g_print_queue, &_msg, 0);                           \
    } while(0)

#endif /* APP_TYPES_H */
