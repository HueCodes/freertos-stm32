#ifndef UART_TASK_H
#define UART_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define UART_TASK_PRIORITY      1           /* lowest - UART is slow, never block others */
#define UART_TASK_STACK_WORDS   256
#define UART_TX_TIMEOUT_MS      100         /* HAL_UART_Transmit timeout */

void uart_task(void *pvParameters);

#endif /* UART_TASK_H */
