#pragma once

#include <stdbool.h>

typedef enum {
    PLATFORM_BUTTON_1,
    PLATFORM_BUTTON_2,
    PLATFORM_BUTTON_COUNT
} platform_button_t;

void platform_init(void);
void platform_led_set(bool on);
bool platform_button_read(platform_button_t button);

float platform_voltage_read(void);
float platform_temp_read(void);

void platform_charge_set(bool on);