#include "profiling/profiling.h"
#include "pico/stdlib.h"

typedef struct {
    const char *name;
    uint32_t count;
    uint32_t total_us;
    uint32_t max_us;
    uint32_t start_us;
    bool is_running;
    bool is_used;
} stopwatch_t;

static stopwatch_t stopwatches[PROFILING_MAX_STOPWATCHES];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= PROFILING_MAX_STOPWATCHES) return;
    stopwatches[id].name = name;
    stopwatches[id].count = 0;
    stopwatches[id].total_us = 0;
    stopwatches[id].max_us = 0;
    stopwatches[id].start_us = 0;
    stopwatches[id].is_running = false;
    stopwatches[id].is_used = true;
}

#if PROFILING_ENABLED
void profiling_start(uint32_t id)
{
    if (id >= PROFILING_MAX_STOPWATCHES) return;
    if (!stopwatches[id].is_used) return;
    stopwatches[id].start_us = (uint32_t)time_us_32();
    stopwatches[id].is_running = true;
}

void profiling_stop(uint32_t id)
{
    if (id >= PROFILING_MAX_STOPWATCHES) return;
    if (!stopwatches[id].is_used) return;
    if (!stopwatches[id].is_running) return;

    uint32_t now = (uint32_t)time_us_32();
    uint32_t diff = now - stopwatches[id].start_us;

    stopwatches[id].is_running = false;
    stopwatches[id].count++;
    stopwatches[id].total_us += diff;
    if (diff > stopwatches[id].max_us)
    {
        stopwatches[id].max_us = diff;
    }
}
#endif

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_MAX_STOPWATCHES) return false;
    if (!stopwatches[id].is_used) return false;

    result->name = stopwatches[id].name;
    result->count = stopwatches[id].count;
    if (stopwatches[id].count > 0)
    {
        result->mean_us = stopwatches[id].total_us / stopwatches[id].count;
    }
    else
    {
        result->mean_us = 0;
    }
    result->max_us = stopwatches[id].max_us;
    return true;
}

void profiling_reset(void)
{
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        if (stopwatches[i].is_used)
        {
            stopwatches[i].count = 0;
            stopwatches[i].total_us = 0;
            stopwatches[i].max_us = 0;
        }
    }
}