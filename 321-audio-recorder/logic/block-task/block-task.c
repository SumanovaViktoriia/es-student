#include "block-task/block-task.h"
#include <stddef.h>
#include "platform/platform.h"

static uint32_t pending_count = 0;
static block_task_callback_t pending_callback = NULL;

bool block_task_start(uint32_t count, block_task_callback_t on_block)
{
    if (pending_callback != NULL)
    {
        return false;
    }

    if (!platform_block_start(count))
    {
        return false;
    }

    pending_count = count;
    pending_callback = on_block;
    return true;
}

void block_task_handle(void)
{
    if (pending_callback == NULL)
    {
        return;
    }

    if (!platform_block_done())
    {
        return;
    }

    block_task_callback_t callback = pending_callback;
    pending_callback = NULL;

    callback(platform_block_samples(), pending_count, platform_block_time_us());
}