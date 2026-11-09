#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "platform/platform.h"

typedef void (*button_task_callback_t)(void);

void button_task_init(platform_button_t button, button_task_callback_t on_press);
void button_task_handle(void);
bool button_task_is_pressed(platform_button_t button);
uint32_t button_task_get_press_count(platform_button_t button);