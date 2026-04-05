#include "bme280.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/*
 * BME280 register addresses
 */
#define BME280_REG_ID           0xD0    /* should read 0x60 */
#define BME280_REG_RESET        0xE0    /* write 0xB6 to reset */
#define BME280_REG_CTRL_HUM     0xF2
#define BME280_REG_STATUS       0xF3
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_CONFIG       0xF5
#define BME280_REG_PRESS_MSB    0xF7    /* pressure + temperature + humidity (6 bytes) */
#define BME280_CALIB_00         0x88    /* calibration data start */
#define BME280_CALIB_26         0xE1    /* humidity calibration */

#define BME280_ID               0x60

/*
 * Compensation coefficients - read from OTP at init.
 * See BME280 datasheet section 4.2.2.
 */
static struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4, dig_H5;
    int8_t   dig_H6;
} calib;

/* Fine temperature value shared between temperature and pressure compensation */
static int32_t t_fine;

/* -------------------------------------------------------------------------
 * HAL stubs
 *
 * Replace these with real HAL calls once CubeMX project is set up.
 * The extern huart2 / hi2c1 declarations will come from CubeMX's main.c.
 *
 * Real calls:
 *   HAL_I2C_Master_Transmit(&hi2c1, addr<<1, &reg, 1, timeout)
 *   HAL_I2C_Master_Receive (&hi2c1, addr<<1, buf, len, timeout)
 * ------------------------------------------------------------------------- */
static status_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    (void)addr; (void)reg; (void)value;
    /* TODO: HAL_I2C_Mem_Write(&hi2c1, addr<<1, reg, 1, &value, 1, 10); */
    return STATUS_OK;
}

static status_t i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)addr; (void)reg; (void)len;
    /* TODO: HAL_I2C_Mem_Read(&hi2c1, addr<<1, reg, 1, buf, len, 50); */
    /* Stub: return fake plausible values for testing logic without hardware */
    for (uint16_t i = 0; i < len; i++) { buf[i] = 0; }
    return STATUS_OK;
}

/* -------------------------------------------------------------------------
 * BME280 compensation formulas (integer-only, from datasheet section 8.2)
 * ------------------------------------------------------------------------- */
static int32_t compensate_temperature(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1)))
            * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1))
            * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12)
            * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;   /* T in 0.01 degrees C */
    return T;   /* returns centi-degrees C * 10... */
    /* Note: datasheet returns T in 0.01C units.
       We want centi-degrees (0.01C = same). So T here = centidegrees already. */
}

static uint32_t compensate_pressure(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8)
         + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    if (var1 == 0) { return 0; }
    p    = 1048576 - adc_P;
    p    = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p    = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (uint32_t)(p >> 8);  /* Pa */
}

static uint32_t compensate_humidity(int32_t adc_H)
{
    int32_t x;
    x = (t_fine - ((int32_t)76800));
    x = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20)
        - (((int32_t)calib.dig_H5) * x)) + ((int32_t)16384)) >> 15)
        * (((((((x * ((int32_t)calib.dig_H6)) >> 10)
        * (((x * ((int32_t)calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10)
        + ((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 8192) >> 14));
    x = (x - (((((x >> 15) * (x >> 15)) >> 7) * ((int32_t)calib.dig_H1)) >> 4));
    x = (x < 0) ? 0 : x;
    x = (x > 419430400) ? 419430400 : x;
    /* Returns humidity in %RH * 1024. Convert to centi-percent: */
    return (uint32_t)((x >> 12) * 100 / 1024);  /* centi-percent */
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
status_t bme280_init(void)
{
    uint8_t buf[26];

    /* Check chip ID */
    if (i2c_read_regs(BME280_I2C_ADDR, BME280_REG_ID, buf, 1) != STATUS_OK) {
        return STATUS_SENSOR_TIMEOUT;
    }
    /* buf[0] should be 0x60 when hardware is connected */

    /* Soft reset */
    i2c_write_reg(BME280_I2C_ADDR, BME280_REG_RESET, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(10));   /* wait for reset to complete */

    /* Read calibration data (registers 0x88..0xA1 = 26 bytes) */
    i2c_read_regs(BME280_I2C_ADDR, BME280_CALIB_00, buf, 26);
    calib.dig_T1 = (uint16_t)(buf[1]  << 8 | buf[0]);
    calib.dig_T2 = (int16_t) (buf[3]  << 8 | buf[2]);
    calib.dig_T3 = (int16_t) (buf[5]  << 8 | buf[4]);
    calib.dig_P1 = (uint16_t)(buf[7]  << 8 | buf[6]);
    calib.dig_P2 = (int16_t) (buf[9]  << 8 | buf[8]);
    calib.dig_P3 = (int16_t) (buf[11] << 8 | buf[10]);
    calib.dig_P4 = (int16_t) (buf[13] << 8 | buf[12]);
    calib.dig_P5 = (int16_t) (buf[15] << 8 | buf[14]);
    calib.dig_P6 = (int16_t) (buf[17] << 8 | buf[16]);
    calib.dig_P7 = (int16_t) (buf[19] << 8 | buf[18]);
    calib.dig_P8 = (int16_t) (buf[21] << 8 | buf[20]);
    calib.dig_P9 = (int16_t) (buf[23] << 8 | buf[22]);
    calib.dig_H1 = buf[25];

    /* Humidity calibration from 0xE1 */
    i2c_read_regs(BME280_I2C_ADDR, BME280_CALIB_26, buf, 7);
    calib.dig_H2 = (int16_t)(buf[1] << 8 | buf[0]);
    calib.dig_H3 = buf[2];
    calib.dig_H4 = (int16_t)((int16_t)buf[3] << 4 | (buf[4] & 0x0F));
    calib.dig_H5 = (int16_t)((int16_t)buf[5] << 4 | (buf[4] >> 4));
    calib.dig_H6 = (int8_t)buf[6];

    /* Configure sensor:
       ctrl_hum: osrs_h = 001 (x1 oversampling)
       Must be written before ctrl_meas to take effect */
    i2c_write_reg(BME280_I2C_ADDR, BME280_REG_CTRL_HUM, 0x01);

    /* ctrl_meas: osrs_t=001 (x1), osrs_p=001 (x1), mode=11 (normal) */
    i2c_write_reg(BME280_I2C_ADDR, BME280_REG_CTRL_MEAS, 0x27);

    /* config: t_sb=101 (1000ms standby), filter=000 (off), spi3w_en=0 */
    i2c_write_reg(BME280_I2C_ADDR, BME280_REG_CONFIG, 0xA0);

    return STATUS_OK;
}

status_t bme280_read(int32_t  *temperature_cdeg,
                     uint32_t *humidity_cpct,
                     uint32_t *pressure_pa)
{
    uint8_t buf[8];

    /* Read pressure (3), temperature (3), humidity (2) in one burst from 0xF7 */
    status_t ret = i2c_read_regs(BME280_I2C_ADDR, BME280_REG_PRESS_MSB, buf, 8);
    if (ret != STATUS_OK) { return ret; }

    int32_t adc_P = (int32_t)(((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | (buf[2] >> 4));
    int32_t adc_T = (int32_t)(((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | (buf[5] >> 4));
    int32_t adc_H = (int32_t)(((uint32_t)buf[6] << 8)  |  (uint32_t)buf[7]);

    /* Temperature must be compensated first - it sets t_fine used by P and H */
    *temperature_cdeg = compensate_temperature(adc_T);
    *pressure_pa      = compensate_pressure(adc_P);
    *humidity_cpct    = compensate_humidity(adc_H);

    return STATUS_OK;
}
