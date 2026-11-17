#include "block-task/block-task.h"
#include "platform/platform.h"

typedef enum {
    BLOCK_IDLE,
    BLOCK_RUN,
} block_state_t;

static block_state_t state = BLOCK_IDLE;
static block_task_callback_t callback;

bool block_task_start(uint32_t count, block_task_callback_t on_done)
{
    if (state != BLOCK_IDLE)
    {
        return false;
    }

    if (!platform_block_start(count))
    {
        return false;
    }

    callback = on_done;
    state = BLOCK_RUN;
    return true;
}

void block_task_handle(void)
{
    if (state != BLOCK_RUN)
    {
        return;
    }

    if (platform_block_done())
    {
        state = BLOCK_IDLE;
        if (callback != NULL)
        {
            callback(true);
        }
    }
}