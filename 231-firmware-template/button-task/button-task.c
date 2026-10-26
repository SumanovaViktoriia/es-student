#include "button-task.h"
#include "pico/stdlib.h"
#include "led-task.h"
#include "systime/systime.h"

#define BUTTON_PIN 15
#define DEBOUNCE_TIME_US 20000

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint64_t state_change_time_us = 0;
static uint32_t press_count = 0;

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    
    button_state = BUTTON_STATE_RELEASED;
    state_change_time_us = systime_us();
    press_count = 0;
}

void button_task_handle(void)
{
    bool pressed = !gpio_get(BUTTON_PIN);
    uint64_t now_us = systime_us();

    switch (button_state)
    {
        case BUTTON_STATE_RELEASED:
            if (pressed)
            {
                state_change_time_us = now_us;
                button_state = BUTTON_STATE_PRESSING;
            }
            break;

        case BUTTON_STATE_PRESSING:
            if (!pressed)
            {
                button_state = BUTTON_STATE_RELEASED;
            }
            else if (now_us - state_change_time_us >= DEBOUNCE_TIME_US)
            {
                press_count++;
                led_task_next_state();
                button_state = BUTTON_STATE_PRESSED;
            }
            break;

        case BUTTON_STATE_PRESSED:
            if (!pressed)
            {
                state_change_time_us = now_us;
                button_state = BUTTON_STATE_RELEASING;
            }
            break;

        case BUTTON_STATE_RELEASING:
            if (pressed)
            {
                button_state = BUTTON_STATE_PRESSED;
            }
            else if (now_us - state_change_time_us >= DEBOUNCE_TIME_US)
            {
                button_state = BUTTON_STATE_RELEASED;
            }
            break;
    }
}

bool button_task_is_pressed(void)
{
    return (button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_RELEASING);
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}