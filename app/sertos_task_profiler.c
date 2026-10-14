#include "sertos_task_profiler.h"

#include "main.h"
#include "profiler.h"
#include "profiler_transport.h"
#include "sertos_scheduler.h"
#include "usart.h"

void sertos_task_profiler_init(void)
{
    if (!profiler_init(HAL_RCC_GetPCLK1Freq(), &huart2)) {
        Error_Handler();
    }

    profiler_start();
}

void sertos_task_profiler(void* param)
{
    (void)param;

    while (1) {
        (void)profiler_process();
        if (profiler_transport_status() != PROFILER_TRANSPORT_STATUS_OK) {
            Error_Handler();
        }

        (void)sertos_scheduler_delay(10U);
    }
}
