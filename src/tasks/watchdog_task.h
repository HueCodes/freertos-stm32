#ifndef WATCHDOG_TASK_H
#define WATCHDOG_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "app_types.h"

/* Task configuration */
#define WATCHDOG_TASK_PRIORITY      4       /* highest in the system */
#define WATCHDOG_TASK_STACK_WORDS   256
#define WATCHDOG_CHECK_PERIOD_MS    500     /* check every 500ms */

/*
 * watchdog_task_checkin()
 *
 * Called by each task after completing its work cycle to signal it is alive.
 * Thread-safe: uses taskENTER/EXIT_CRITICAL.
 *
 * Each task has a dedicated bit in WDG_BIT_* (defined in app_types.h).
 */
void watchdog_task_checkin(uint32_t task_bit);

/*
 * watchdog_task()
 *
 * The task function. Pass to xTaskCreate.
 * Checks that all tasks have checked in every WATCHDOG_CHECK_PERIOD_MS.
 * Feeds IWDG only when all bits are set. If any task misses its checkin,
 * the IWDG times out and the system resets.
 */
void watchdog_task(void *pvParameters);

#endif /* WATCHDOG_TASK_H */
