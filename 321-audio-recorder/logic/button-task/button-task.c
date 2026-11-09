#include "button-task/button-task.h"
#include <stddef.h>
#include "systime/systime.h"

#define DEBOUNCE_TIME_US 20000

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

typedef struct {
    button_state_t state;
    uint64_t state_change_time_us;
    uint32_t press_count;
    button_task_callback_t on_press;
} button_machine_t;

static button_machine_t buttons[PLATFORM_BUTTON_COUNT];

void button_task_init(platform_button_t button, button_task_callback_t on_press)
{
    if (button >= PLATFORM_BUTTON_COUNT) return;
    buttons[button].state = BUTTON_STATE_RELEASED;
    buttons[button].state_change_time_us = systime_us();
    buttons[button].press_count = 0;
    buttons[button].on_press = on_press;
}

void button_task_handle(void)
{
    uint64_t now_us = systime_us();

    for (uint32_t i = 0; i < PLATFORM_BUTTON_COUNT; i++)
    {
        button_machine_t *b = &buttons[i];
        bool pressed = platform_button_read((platform_button_t)i);

        switch (b->state)
        {
            case BUTTON_STATE_RELEASED:
                if (pressed)
                {
                    b->state_change_time_us = now_us;
                    b->state = BUTTON_STATE_PRESSING;
                }
                break;

            case BUTTON_STATE_PRESSING:
                if (!pressed)
                {
                    b->state = BUTTON_STATE_RELEASED;
                }
                else if (now_us - b->state_change_time_us >= DEBOUNCE_TIME_US)
                {
                    b->press_count++;
                    if (b->on_press != NULL)
                    {
                        b->on_press();
                    }
                    b->state = BUTTON_STATE_PRESSED;
                }
                break;

            case BUTTON_STATE_PRESSED:
                if (!pressed)
                {
                    b->state_change_time_us = now_us;
                    b->state = BUTTON_STATE_RELEASING;
                }
                break;

            case BUTTON_STATE_RELEASING:
                if (pressed)
                {
                    b->state = BUTTON_STATE_PRESSED;
                }
                else if (now_us - b->state_change_time_us >= DEBOUNCE_TIME_US)
                {
                    b->state = BUTTON_STATE_RELEASED;
                }
                break;
        }
    }
}

bool button_task_is_pressed(platform_button_t button)
{
    if (button >= PLATFORM_BUTTON_COUNT) return false;
    return (buttons[button].state == BUTTON_STATE_PRESSED || buttons[button].state == BUTTON_STATE_RELEASING);
}

uint32_t button_task_get_press_count(platform_button_t button)
{
    if (button >= PLATFORM_BUTTON_COUNT) return 0;
    return buttons[button].press_count;
}