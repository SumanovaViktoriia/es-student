#include "stream-task/stream-task.h"
#include <stddef.h>
#include "platform/platform.h"

static stream_task_callback_t callback = NULL;
static stream_state_t state = STREAM_STATE_IDLE;
static uint8_t bits = 16;
static uint16_t frame_number = 0;
static uint32_t frames = 0;
static uint32_t samples = 0;

void stream_task_init(stream_task_callback_t on_frame)
{
    callback = on_frame;
    state = STREAM_STATE_IDLE;
}

bool stream_task_start(uint8_t new_bits)
{
    if (state == STREAM_STATE_RUN)
    {
        return false;
    }

    if (new_bits != 8 && new_bits != 16)
    {
        return false;
    }

    if (!platform_stream_start(new_bits))
    {
        return false;
    }

    bits = new_bits;
    frame_number = 0;
    frames = 0;
    samples = 0;
    state = STREAM_STATE_RUN;
    return true;
}

void stream_task_stop(void)
{
    if (state != STREAM_STATE_RUN)
    {
        return;
    }

    platform_stream_stop();
    state = STREAM_STATE_IDLE;
}

void stream_task_handle(void)
{
    if (state != STREAM_STATE_RUN || callback == NULL)
    {
        return;
    }

    uint32_t count = 0;
    const uint8_t *data = platform_stream_next(&count);
    if (data == NULL)
    {
        return;
    }

    stream_header_t header;
    header.sync_word = STREAM_SYNC_WORD;
    header.number = frame_number;
    header.count = (uint16_t)count;

    uint32_t bytes = (bits == 16) ? count * 2 : count;

    callback(&header, data, bytes);

    platform_stream_release();

    frame_number++;
    frames++;
    samples += count;
}

stream_state_t stream_task_get_state(void) { return state; }
uint8_t stream_task_get_bits(void) { return bits; }
uint32_t stream_task_get_frames(void) { return frames; }
uint32_t stream_task_get_samples(void) { return samples; }
uint32_t stream_task_get_overruns(void) { return platform_stream_overruns(); }