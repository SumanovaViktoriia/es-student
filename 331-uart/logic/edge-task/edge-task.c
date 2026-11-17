#include <stddef.h>
#include "edge-task/edge-task.h"
#include "systime/systime.h"

#define EMPTY_V     0.05f
#define RISE_V      2.0f
#define FULL_V      3.2f
#define FALL_V      1.3f
#define TIMEOUT_US  2000000u

typedef enum {
    EDGE_IDLE,
    EDGE_DISCHARGE,
    EDGE_RISE,
    EDGE_CHARGE,
    EDGE_FALL,
} edge_state_t;

static edge_state_t state = EDGE_IDLE;
static platform_line_mode_t line_mode;
static edge_task_callback_t callback;
static uint64_t phase_start_us;
static uint32_t rise_us;
static uint32_t fall_us;

static void enter(edge_state_t new_state)
{
    state = new_state;
    phase_start_us = systime_us();

    switch (new_state)
    {
        case EDGE_DISCHARGE:
            platform_line_set(line_mode, false);
            break;
        case EDGE_RISE:
            platform_line_set(line_mode, true);
            break;
        case EDGE_CHARGE:
            platform_line_set(line_mode, true);
            break;
        case EDGE_FALL:
            platform_line_set(line_mode, false);
            break;
        default:
            break;
    }
}

static void finish(bool ok, uint32_t r_us, uint32_t f_us)
{
    state = EDGE_IDLE;
    if (callback != NULL)
    {
        callback(line_mode, ok, r_us, f_us);
    }
}

bool edge_task_start(platform_line_mode_t mode, edge_task_callback_t on_done)
{
    if (state != EDGE_IDLE)
    {
        return false;
    }

    line_mode = mode;
    callback = on_done;
    rise_us = 0;
    fall_us = 0;

    enter(EDGE_DISCHARGE);
    return true;
}

void edge_task_handle(void)
{
    if (state == EDGE_IDLE)
    {
        return;
    }

    uint64_t now_us = systime_us();
    uint32_t elapsed_us = (uint32_t)(now_us - phase_start_us);
    float voltage = platform_voltage_read();

    if (elapsed_us > TIMEOUT_US)
    {
        finish(false, 0, 0);
        return;
    }

    switch (state)
    {
        case EDGE_DISCHARGE:
            if (voltage < EMPTY_V)
            {
                rise_us = (uint32_t)(now_us - phase_start_us);
                phase_start_us = now_us;
                enter(EDGE_RISE);
            }
            break;

        case EDGE_RISE:
            if (voltage >= RISE_V)
            {
                rise_us = (uint32_t)(now_us - phase_start_us);
                enter(EDGE_CHARGE);
            }
            break;

        case EDGE_CHARGE:
            if (voltage >= FULL_V)
            {
                enter(EDGE_FALL);
            }
            break;

        case EDGE_FALL:
            if (voltage <= FALL_V)
            {
                fall_us = (uint32_t)(now_us - phase_start_us);
                finish(true, rise_us, fall_us);
            }
            break;

        default:
            break;
    }
}