#include "led-task.h"
#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 25
#define DEFAULT_PERIOD_MS 1000

static led_state_t led_state = LED_STATE_BLINK;
static uint32_t led_period_ms = DEFAULT_PERIOD_MS;
static uint64_t last_toggle_us = 0;

static bool period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();
    if (now_us - *last_us >= period_us)
    {
        *last_us += period_us;
        if (now_us - *last_us >= period_us)
        {
            *last_us = now_us;
        }
        return true;
    }
    return false;
}

void led_task_init(void)
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    led_period_ms = DEFAULT_PERIOD_MS;
    led_state = LED_STATE_BLINK;
    last_toggle_us = time_us_64();
    gpio_put(LED_PIN, 1);
}

void led_task_set_state(led_state_t state)
{
    led_state = state;
    switch (led_state)
    {
        case LED_STATE_OFF:
            gpio_put(LED_PIN, 0);
            break;
        case LED_STATE_ON:
            gpio_put(LED_PIN, 1);
            break;
        case LED_STATE_BLINK:
            last_toggle_us = time_us_64() - (led_period_ms * 1000 / 2);
            break;
    }
}

void led_task_set_period_ms(uint32_t period_ms)
{
    led_period_ms = period_ms;
}

uint32_t led_task_get_period_ms(void)
{
    return led_period_ms;
}

led_state_t led_task_get_state(void)
{
    return led_state;
}

void led_task_handle(void)
{
    switch (led_state)
    {
        case LED_STATE_OFF:
        case LED_STATE_ON:
            break;
        case LED_STATE_BLINK:
            if (period_elapsed(&last_toggle_us, (led_period_ms * 1000) / 2))
            {
                gpio_xor_mask(1u << LED_PIN);
            }
            break;
    }
}

void led_task_next_state(void)
{
    switch (led_state)
    {
        case LED_STATE_OFF:   led_task_set_state(LED_STATE_ON);    break;
        case LED_STATE_ON:    led_task_set_state(LED_STATE_BLINK); break;
        case LED_STATE_BLINK: led_task_set_state(LED_STATE_OFF);   break;
    }
}