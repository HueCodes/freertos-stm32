#ifndef STM32F4xx_HAL_CONF_H
#define STM32F4xx_HAL_CONF_H

/* Oscillator values */
#define HSE_VALUE            8000000U   /* Nucleo external clock from ST-Link */
#define HSE_STARTUP_TIMEOUT  100U
#define HSI_VALUE            16000000U
#define LSI_VALUE            32000U
#define LSE_VALUE            32768U
#define LSE_STARTUP_TIMEOUT  5000U
#define EXTERNAL_CLOCK_VALUE 12288000U

/* SysTick -- FreeRTOS takes over SysTick, but HAL needs a tick source.
   We route HAL_GetTick() through FreeRTOS xTaskGetTickCount() in main.c. */
#define TICK_INT_PRIORITY    15U  /* Lowest priority -- FreeRTOS manages it */

/* Prefetch, instruction cache, data cache */
#define PREFETCH_ENABLE      1
#define INSTRUCTION_CACHE_ENABLE 1
#define DATA_CACHE_ENABLE    1

/* HAL module selection -- enable only what CMakeLists.txt compiles */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED

/* Includes for enabled modules */
#ifdef HAL_MODULE_ENABLED
  #include "stm32f4xx_hal_def.h"
#endif
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f4xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f4xx_hal_gpio.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f4xx_hal_cortex.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
  #include "stm32f4xx_hal_i2c.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32f4xx_hal_uart.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f4xx_hal_flash.h"
  #include "stm32f4xx_hal_flash_ex.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f4xx_hal_pwr.h"
  #include "stm32f4xx_hal_pwr_ex.h"
#endif
#ifdef HAL_IWDG_MODULE_ENABLED
  #include "stm32f4xx_hal_iwdg.h"
#endif

#endif /* STM32F4xx_HAL_CONF_H */
