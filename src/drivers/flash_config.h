#ifndef FLASH_CONFIG_H
#define FLASH_CONFIG_H

#include <stdint.h>
#include "app_types.h"

/*
 * Persistent configuration stored in STM32F411RE flash.
 *
 * Flash layout for F411RE (512KB total):
 *   Sectors 0-5: application code  (384KB, 0x08000000 - 0x0805FFFF)
 *   Sector 6:    reserved config   (128KB, 0x08060000 - 0x0807FFFF)
 *
 * Sector 6 is erased and re-written on every config update.
 * Only the first sizeof(device_config_t) bytes are used.
 *
 * NOTE: On STM32F4, flash erase is by sector (not page). Erasing sector 6
 * takes ~1-2 seconds. Do not call flash_config_write from a time-critical task.
 * The CLI task is appropriate.
 */

#define CONFIG_FLASH_SECTOR     6
#define CONFIG_FLASH_ADDR       0x08060000UL

#define CONFIG_MAGIC            0xDEADBEEFUL
#define CONFIG_VERSION          1UL

typedef struct {
    uint32_t magic;              /* CONFIG_MAGIC - validates config is present  */
    uint32_t version;            /* CONFIG_VERSION - handle layout changes      */
    int32_t  temp_offset_cdeg;   /* calibration offset added to raw temp        */
    uint32_t sample_period_ms;   /* sensor sample period (default 1000)         */
    uint32_t uart_baud;          /* UART baud rate (default 115200)             */
    uint32_t crc32;              /* CRC32 of all fields above (except this one) */
} device_config_t;

/* Default values used when flash contains no valid config */
#define CONFIG_DEFAULT_TEMP_OFFSET  0
#define CONFIG_DEFAULT_PERIOD_MS    1000
#define CONFIG_DEFAULT_BAUD         115200

/*
 * flash_config_read()
 *
 * Reads config from flash. Validates magic and CRC32.
 * If invalid, fills cfg with defaults and returns STATUS_FLASH_INVALID_CONFIG.
 * If valid, fills cfg with stored values and returns STATUS_OK.
 *
 * Safe to call from any task context.
 */
status_t flash_config_read(device_config_t *cfg);

/*
 * flash_config_write()
 *
 * Erases CONFIG_FLASH_SECTOR, writes cfg with updated CRC32.
 * BLOCKING: takes ~1-2 seconds for the sector erase.
 * Returns STATUS_OK or STATUS_FLASH_ERR.
 *
 * Call only from CLI task. Do NOT call from ISR or time-critical task.
 */
status_t flash_config_write(const device_config_t *cfg);

#endif /* FLASH_CONFIG_H */
