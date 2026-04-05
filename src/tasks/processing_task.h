#ifndef PROCESSING_TASK_H
#define PROCESSING_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#define PROCESSING_TASK_PRIORITY    2
#define PROCESSING_TASK_STACK_WORDS 512

void processing_task(void *pvParameters);

#endif /* PROCESSING_TASK_H */
