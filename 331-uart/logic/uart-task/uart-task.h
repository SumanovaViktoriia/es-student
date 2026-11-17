#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/platform.h"

#define UART_TASK_MAX 64

typedef void (*uart_task_callback_t)(const uint8_t *data, size_t size, uint32_t errors);

bool uart_task_start(platform_uart_t tx, platform_uart_t rx, const uint8_t *data, size_t size,
                     uart_task_callback_t on_done);

void uart_task_handle(void);