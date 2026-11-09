#include "telemetry-task/telemetry-task.h"
#include <stddef.h>
#include "platform/platform.h"
#include "systime/systime.h"

static telemetry_state_t state = TELEMETRY_STATE_IDLE;
static uint32_t period_ms = 100;
static uint64_t last_time_us = 0;
static telemetry_task_callback_t callback = NULL;

void telemetry_task_init(telemetry_task_callback_t on_record)
{
    state = TELEMETRY_STATE_IDLE;
    period_ms = 100;
    last_time_us = 0;
    callback = on_record;
}

void telemetry_task_handle(void)
{
    if (state != TELEMETRY_STATE_RUN)
    {
        return;
    }

    if (systime_period_elapsed(&last_time_us, (uint64_t)period_ms * 1000))
    {
        telemetry_record_t record;
        record.time_us = systime_us();
        record.voltage_v = platform_voltage_read();
        record.temp_c = platform_temp_read();

        if (callback != NULL)
        {
            callback(&record);
        }
    }
}

bool telemetry_task_start(uint32_t period)
{
    if (period < 10)
    {
        return false;
    }
    period_ms = period;
    last_time_us = systime_us();
    state = TELEMETRY_STATE_RUN;
    return true;
}

void telemetry_task_stop(void)
{
    state = TELEMETRY_STATE_IDLE;
}

telemetry_state_t telemetry_task_get_state(void)
{
    return state;
}

uint32_t telemetry_task_get_period_ms(void)
{
    return period_ms;
}