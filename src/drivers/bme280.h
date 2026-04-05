#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "app_types.h"

/*
 * BME280 I2C driver interface
 *
 * I2C address: 0x76 (SDO pin low) or 0x77 (SDO pin high)
 * Most breakout boards pull SDO low -> 0x76
 *
 * Datasheet: https://www.bosch-sensortec.com/products/environmental-sensors/humidity-sensors-bme280/
 */

#define BME280_I2C_ADDR     0x76

/*
 * bme280_init()
 *
 * Resets the sensor, reads and stores compensation coefficients from OTP,
 * and sets operating mode:
 *   - Oversampling: x1 temp, x1 humidity, x1 pressure
 *   - Mode: normal (continuous measurement)
 *   - Standby time: 1000ms (matches our 1s sample rate)
 *   - Filter: off
 *
 * Returns STATUS_OK or STATUS_SENSOR_TIMEOUT if sensor not found on I2C bus.
 *
 * Call once at startup before bme280_read().
 */
status_t bme280_init(void);

/*
 * bme280_read()
 *
 * Reads raw ADC values from the sensor and applies the compensation formulas
 * from the BME280 datasheet (integer-only version, no floating point).
 *
 * Outputs:
 *   temperature_cdeg  - temperature in centi-degrees C (2350 = 23.50 C)
 *   humidity_cpct     - relative humidity in centi-percent (4500 = 45.00 %)
 *   pressure_pa       - pressure in Pascals
 *
 * Returns STATUS_OK, STATUS_SENSOR_TIMEOUT, or STATUS_SENSOR_CHECKSUM_ERR.
 *
 * NOTE: The actual I2C read uses HAL_I2C_Master_Transmit / Receive.
 *       These are stubbed in bme280.c until CubeMX HAL is available.
 */
status_t bme280_read(int32_t  *temperature_cdeg,
                     uint32_t *humidity_cpct,
                     uint32_t *pressure_pa);

#endif /* BME280_H */
