#ifndef CLI_TASK_H
#define CLI_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define CLI_TASK_PRIORITY       2
#define CLI_TASK_STACK_WORDS    512
#define CLI_CMD_BUF_LEN         48

void cli_task(void *pvParameters);

/*
 * cli_uart_rx_isr_callback()
 *
 * Call this from the UART RX complete interrupt handler.
 * Feeds received character into g_rx_char_queue using ISR-safe API.
 *
 * In stm32f4xx_it.c, inside the relevant IRQHandler:
 *   cli_uart_rx_isr_callback(received_byte);
 *   HAL_UART_Receive_IT(&huart2, &rx_byte, 1);  // re-arm for next byte
 */
void cli_uart_rx_isr_callback(uint8_t byte);

#endif /* CLI_TASK_H */
