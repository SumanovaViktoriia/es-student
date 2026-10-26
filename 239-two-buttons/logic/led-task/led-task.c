#include "led-task/led-task.h"
#include "platform/platform.h"
#include "systime/systime.h"

#define DEFAULT_PERIOD_MS 1000

static led_state_t led_state = LED_STATE_BLINK;
static uint32_t led_period_ms = DEFAULT_PERIOD_MS;
static uint64_t last_toggle_us = 0;

void led_task_init(void)
{
    led_period_ms = DEFAULT_PERIOD_MS;
    led_state = LED_STATE_BLINK;
    last_toggle_us = systime_us();
    platform_led_set(true);
}

void led_task_set_state(led_state_t state)
{
    led_state = state;
    switch (led_state)
    {
        case LED_STATE_OFF: platform_led_set(false); break;
        case LED_STATE_ON: platform_led_set(true); break;
        case LED_STATE_BLINK:
            last_toggle_us = systime_us() - (led_period_ms * 1000 / 2);
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
    static bool led_on = true;
    switch (led_state)
    {
        case LED_STATE_OFF:
        case LED_STATE_ON:
            break;
        case LED_STATE_BLINK:
            if (systime_period_elapsed(&last_toggle_us, (led_period_ms * 1000) / 2))
            {
                led_on = !led_on;
                platform_led_set(led_on);
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

void led_task_next_period(void)
{
    switch (led_period_ms)
    {
        case 250:  led_period_ms = 500;  break;
        case 500:  led_period_ms = 1000; break;
        case 1000: led_period_ms = 250;  break;
        default:   led_period_ms = 500;  break;
    }
}