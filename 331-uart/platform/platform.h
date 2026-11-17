#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PLATFORM_BUTTON_1,
    PLATFORM_BUTTON_COUNT
} platform_button_t;

typedef enum {
    PLATFORM_LINE_PUSH_PULL,
    PLATFORM_LINE_OPEN_DRAIN,
} platform_line_mode_t;

void platform_init(void);
void platform_led_set(bool on);
bool platform_button_read(platform_button_t button);

float platform_voltage_read(void);
float platform_temp_read(void);

void platform_line_set(platform_line_mode_t mode, bool high);