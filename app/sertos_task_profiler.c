#include "sertos_task_profiler.h"

#include <stdint.h>

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
        uint8_t events = profiler_transport_poll(NULL);

        if ((events & (PROFILER_TRANSPORT_EVENT_ERROR | PROFILER_TRANSPORT_EVENT_OVERRUN)) != 0U) {
            (void)profiler_transport_status();
            Error_Handler();
        }
        if ((events & PROFILER_TRANSPORT_EVENT_FRAME) != 0U) {
            const Stream* stream = profiler_transport_stream();

            if (stream == NULL) {
                Error_Handler();
            }
            (void)profiler_stream_process(stream);
            if (profiler_transport_listen() != PROFILER_TRANSPORT_STATUS_OK) {
                Error_Handler();
            }
        }

        (void)profiler_process();
        if (profiler_transport_status() != PROFILER_TRANSPORT_STATUS_OK) {
            Error_Handler();
        }

        (void)sertos_scheduler_delay(10U);
    }
}
