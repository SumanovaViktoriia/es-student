#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PROFILING_MAX_STOPWATCHES 8

typedef struct {
    const char *name;
    uint32_t count;
    uint32_t total_us;
    uint32_t max_us;
} profiling_result_t;

void profiling_stopwatch_init(uint32_t id, const char *name);
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);

// Суммы хранятся в 32 битах: переполнение возможно через ~71 минуту
// непрерывных замеров одного секундомера.