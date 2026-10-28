#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "api-commands.h"
#include "api.h"
#include "firmware.h"
#include "device/device.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "profiling/profiling.h"
#include "systime/systime.h"
#include "platform/platform.h"
#include "telemetry-task/telemetry-task.h"

#define DEFAULT_PI_TERMS 1000000
#define TELEMETRY_DEFAULT_PERIOD_MS 100

static bool parse_u32(const char *text, uint32_t *number)
{
    if (text == NULL) return false;
    char *end;
    unsigned long val = strtoul(text, &end, 10);
    if (*end != '\0') return false;
    *number = (uint32_t)val;
    return true;
}

static void print_led_state(void)
{
    const char *state_str = "";
    switch (led_task_get_state())
    {
        case LED_STATE_OFF: state_str = "off"; break;
        case LED_STATE_ON: state_str = "on"; break;
        case LED_STATE_BLINK: state_str = "blink"; break;
    }
    printf("led: %s, period %u ms\n", state_str, (unsigned)led_task_get_period_ms());
}

static void cmd_info(const command_t *command)
{
    (void)command;
    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", FIRMWARE_REPO);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.chip_manufacturer, (unsigned)info.chip_part, (unsigned)info.chip_revision);
    printf("pico-sdk: %s\n", info.sdk_version);
}

static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n", systime_ms());
}

static void cmd_pi(const command_t *command)
{
    (void)command;
    switch (pi_task_get_state())
    {
        case PI_STATE_NOT_STARTED: printf("pi: not started\n"); break;
        case PI_STATE_RUNNING: printf("pi: running\n"); break;
        case PI_STATE_DONE:
            printf("pi: %.9f in %llu ms\n", pi_task_get_value(), pi_task_get_duration_ms());
            break;
    }
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t terms = DEFAULT_PI_TERMS;

    if (command->argc > 1)
    {
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &terms) || terms == 0)
        {
            printf("error: usage pi_start [terms]\n");
            return;
        }
    }

    if (pi_task_start(terms)) printf("pi: started, %u terms\n", (unsigned)terms);
    else printf("error: pi is busy\n");
}

static void cmd_led_enable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_ON);
    print_led_state();
}

static void cmd_led_disable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_OFF);
    print_led_state();
}

static void cmd_led_blink(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_BLINK);
    print_led_state();
}

static void cmd_led_period(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage led_period [period_ms]\n");
        return;
    }
    uint32_t period = 0;
    if (!parse_u32(command->argv[0], &period) || period == 0)
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }
    led_task_set_period_ms(period);
    print_led_state();
}

static void cmd_button(const command_t *command)
{
    (void)command;
    printf("button 1: %s, presses %u\n",
           button_task_is_pressed(PLATFORM_BUTTON_1) ? "pressed" : "released",
           (unsigned)button_task_get_press_count(PLATFORM_BUTTON_1));
    printf("button 2: %s, presses %u\n",
           button_task_is_pressed(PLATFORM_BUTTON_2) ? "pressed" : "released",
           (unsigned)button_task_get_press_count(PLATFORM_BUTTON_2));
}

static void cmd_profiling(const command_t *command)
{
    (void)command;
#if PROFILING_ENABLED
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        profiling_result_t result;
        if (profiling_get(i, &result))
        {
            printf("%-10s count %-10u mean %u us  max %u us\n",
                   result.name, (unsigned)result.count,
                   (unsigned)result.mean_us, (unsigned)result.max_us);
        }
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_profiling_reset(const command_t *command)
{
    (void)command;
    profiling_reset();
    printf("profiling reset\n");
}

static void cmd_get_voltage(const command_t *command)
{
    (void)command;
    printf("voltage: %.3f V\n", platform_voltage_read());
}

static void cmd_get_temp(const command_t *command)
{
    (void)command;
    printf("temp: %.1f C\n", platform_temp_read());
}

static void cmd_tm_start(const command_t *command)
{
    uint32_t period = TELEMETRY_DEFAULT_PERIOD_MS;

    if (command->argc > 1)
    {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &period))
        {
            printf("error: usage tm_start [<period_ms>]\n");
            return;
        }
    }

    if (telemetry_task_start(period))
    {
        printf("telemetry: started, period %u ms\n", (unsigned)period);
    }
    else
    {
        printf("error: period_ms must be at least 10\n");
    }
}

static void cmd_tm_stop(const command_t *command)
{
    (void)command;
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}

static void cmd_charge_on(const command_t *command)
{
    (void)command;
    platform_charge_set(true);
    printf("charge: on\n");
}

static void cmd_charge_off(const command_t *command)
{
    (void)command;
    platform_charge_set(false);
    printf("charge: off\n");
}

void api_print_telemetry(const telemetry_record_t *record)
{
    printf("tm: %llu %.3f %.1f\n", record->time_us, record->voltage_v, record->temp_c);
}

const api_command_t api_commands[] = {
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"pi", "show pi status or result", cmd_pi},
    {"pi_start", "start pi calculation: pi_start [terms]", cmd_pi_start},
    {"led_enable", "turn LED on", cmd_led_enable},
    {"led_disable", "turn LED off", cmd_led_disable},
    {"led_blink", "blink LED", cmd_led_blink},
    {"led_period", "set LED period: led_period [ms]", cmd_led_period},
    {"button", "button state and press count", cmd_button},
    {"profiling", "show profiling results", cmd_profiling},
    {"profiling_reset", "reset profiling results", cmd_profiling_reset},
    {"get_voltage", "voltage on GP26, ADC input 0", cmd_get_voltage},
    {"get_temp", "chip temperature, degrees Celsius", cmd_get_temp},
    {"tm_start", "start telemetry: tm_start [period_ms]", cmd_tm_start},
    {"tm_stop", "stop telemetry", cmd_tm_stop},
    {"charge_on", "start charging RC circuit", cmd_charge_on},
    {"charge_off", "stop charging RC circuit", cmd_charge_off},
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);