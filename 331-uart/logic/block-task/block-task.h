#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef void (*block_task_callback_t)(bool ok);

bool block_task_start(uint32_t count, block_task_callback_t on_done);
void block_task_handle(void);