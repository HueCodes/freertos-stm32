#include "flash_config.h"
#include <string.h>
#include <stddef.h>

/*
 * HAL flash stubs - replace with real calls once CubeMX is set up.
 *
 * Real sequence for STM32F4:
 *   HAL_FLASH_Unlock();
 *   FLASH_Erase_Sector(sector, FLASH_VOLTAGE_RANGE_3);   // 2.7-3.6V
 *   HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, word);
 *   HAL_FLASH_Lock();
 */

/* -------------------------------------------------------------------------
 * CRC32 - software implementation (STM32F4 has hardware CRC peripheral,
 * but it uses a fixed polynomial and requires peripheral init. Software
 * CRC is simpler to stub and portable.)
 * ------------------------------------------------------------------------- */
static uint32_t crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    while (len--)
    {
        crc ^= *data++;
        for (int i = 0; i < 8; i++)
        {
            crc = (crc >> 1) ^ (0xEDB88320UL & -(crc & 1));
        }
    }
    return ~crc;
}

static uint32_t config_crc(const device_config_t *cfg)
{
    /* CRC covers all fields except the crc field itself */
    return crc32((const uint8_t *)cfg, offsetof(device_config_t, crc32));
}

/* -------------------------------------------------------------------------
 * flash_config_read
 * ------------------------------------------------------------------------- */
status_t flash_config_read(device_config_t *cfg)
{
    /* Read directly from flash memory - it is memory-mapped on STM32 */
    const device_config_t *stored = (const device_config_t *)CONFIG_FLASH_ADDR;

    if (stored->magic   != CONFIG_MAGIC   ||
        stored->version != CONFIG_VERSION ||
        stored->crc32   != config_crc(stored))
    {
        /* No valid config - fill defaults */
        cfg->magic            = CONFIG_MAGIC;
        cfg->version          = CONFIG_VERSION;
        cfg->temp_offset_cdeg = CONFIG_DEFAULT_TEMP_OFFSET;
        cfg->sample_period_ms = CONFIG_DEFAULT_PERIOD_MS;
        cfg->uart_baud        = CONFIG_DEFAULT_BAUD;
        cfg->crc32            = config_crc(cfg);
        return STATUS_FLASH_INVALID_CONFIG;
    }

    memcpy(cfg, stored, sizeof(device_config_t));
    return STATUS_OK;
}

/* -------------------------------------------------------------------------
 * flash_config_write
 * ------------------------------------------------------------------------- */
status_t flash_config_write(const device_config_t *cfg)
{
    device_config_t to_write = *cfg;
    to_write.magic   = CONFIG_MAGIC;
    to_write.version = CONFIG_VERSION;
    to_write.crc32   = config_crc(&to_write);

    /*
     * TODO: Replace stubs below with HAL calls once CubeMX is set up.
     *
     * HAL_FLASH_Unlock();
     *
     * FLASH_EraseInitTypeDef erase = {
     *     .TypeErase    = FLASH_TYPEERASE_SECTORS,
     *     .Sector       = CONFIG_FLASH_SECTOR,
     *     .NbSectors    = 1,
     *     .VoltageRange = FLASH_VOLTAGE_RANGE_3,
     * };
     * uint32_t sector_error;
     * if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK) {
     *     HAL_FLASH_Lock();
     *     return STATUS_FLASH_ERR;
     * }
     *
     * // Write word by word (4 bytes at a time)
     * uint32_t *src  = (uint32_t *)&to_write;
     * uint32_t  addr = CONFIG_FLASH_ADDR;
     * for (size_t i = 0; i < sizeof(device_config_t) / 4; i++) {
     *     if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i]) != HAL_OK) {
     *         HAL_FLASH_Lock();
     *         return STATUS_FLASH_ERR;
     *     }
     *     addr += 4;
     * }
     *
     * HAL_FLASH_Lock();
     *
     * // Verify by reading back
     * device_config_t verify;
     * flash_config_read(&verify);
     * if (verify.crc32 != to_write.crc32) { return STATUS_FLASH_ERR; }
     */

    (void)to_write;
    return STATUS_OK;   /* stub: always succeeds */
}
