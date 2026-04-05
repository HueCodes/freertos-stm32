#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define SENSOR_TASK_PRIORITY        2
#define SENSOR_TASK_STACK_WORDS     512
#define SENSOR_TASK_PERIOD_MS       1000    /* sample every 1000ms exactly */
#define SENSOR_MAX_CONSECUTIVE_FAIL 3       /* send error sentinel after this many fails */

void sensor_task(void *pvParameters);

#endif /* SENSOR_TASK_H */
