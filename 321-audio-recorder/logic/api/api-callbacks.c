#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#define ADC_LEVELS 4096
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
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"

#define DEFAULT_PI_TERMS 1000000
#define TELEMETRY_DEFAULT_PERIOD_MS 100
#define STREAM_BLINK_PERIOD_MS 250

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
        if (!parse_u32(command->argv[0], &terms))
        {
            printf("error: usage pi_start [terms]\n");
            return;
        }
        if (terms == 0)
        {
            printf("error: terms must be greater than 0\n");
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
    uint32_t period = 0;
    if (command->argc != 1 || !parse_u32(command->argv[0], &period))
    {
        printf("error: usage led_period [period_ms]\n");
        return;
    }
    if (period == 0)
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
    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }
    printf("voltage: %.3f V\n", platform_voltage_read());
}

static void cmd_get_temp(const command_t *command)
{
    (void)command;
    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }
    printf("temp: %.1f C\n", platform_temp_read());
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

static void cmd_timer_blink_start(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage timer_blink_start <period_ms>\n");
        return;
    }

    uint32_t period = 0;
    if (!parse_u32(command->argv[0], &period))
    {
        printf("error: usage timer_blink_start <period_ms>\n");
        return;
    }

    if (!platform_blink_start(period))
    {
        printf("error: period_ms must be 10..60000\n");
        return;
    }

    led_task_set_state(LED_STATE_OFF);
    printf("timer_blink: started, period %u ms\n", (unsigned)period);
}

static void cmd_timer_blink_stop(const command_t *command)
{
    (void)command;
    uint32_t toggles = platform_blink_stop();
    printf("timer_blink: stopped, %u toggles\n", (unsigned)toggles);
}

static void cmd_cap_start(const command_t *command)
{
    if (command->argc != 2)
    {
        printf("error: usage cap_start <N> <period_us>\n");
        return;
    }

    uint32_t count = 0;
    uint32_t period_us = 0;
    if (!parse_u32(command->argv[0], &count) || !parse_u32(command->argv[1], &period_us))
    {
        printf("error: usage cap_start <N> <period_us>\n");
        return;
    }

    if (count == 0 || count > PLATFORM_CAPTURE_MAX || period_us < 10)
    {
        printf("error: N must be 1..2000, period_us at least 10\n");
        return;
    }

    if (platform_adc_busy() || telemetry_task_get_state() == TELEMETRY_STATE_RUN)
    {
        printf("error: adc busy\n");
        return;
    }

    if (!platform_capture_start(count, period_us))
    {
        printf("error: adc busy\n");
        return;
    }

    printf("capture: started, %u samples, period %u us\n", (unsigned)count, (unsigned)period_us);
}

static void cmd_cap_dump(const command_t *command)
{
    (void)command;

    if (!platform_capture_done())
    {
        printf("error: no capture\n");
        return;
    }

    platform_capture_t capture;
    platform_capture_get(&capture);

    printf("capture: %u samples, period %u us, interval %u..%u us, handler %.1f %%\n",
           (unsigned)capture.count, (unsigned)capture.period_us,
           (unsigned)capture.min_interval_us, (unsigned)capture.max_interval_us,
           (double)capture.handler_percent);

    for (uint32_t i = 0; i < capture.count; i++)
    {
        if (i % 16 == 0)
        {
            printf("cap:");
        }
        printf(" %u", (unsigned)capture.samples[i]);
        if (i % 16 == 15 || i == capture.count - 1)
        {
            printf("\n");
        }
    }
}

static void print_dma_cap(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    uint16_t min_val = 65535;
    uint16_t max_val = 0;
    uint32_t sum = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        if (samples[i] < min_val) min_val = samples[i];
        if (samples[i] > max_val) max_val = samples[i];
        sum += samples[i];
    }

    double mean = (double)sum / (double)count;
    uint64_t expected_us = (uint64_t)count * 1000000ULL / PLATFORM_SAMPLE_RATE_HZ;

    printf("dma_cap: %u samples in %u us, expected %llu us, min %u max %u mean %.1f\n",
           (unsigned)count, (unsigned)time_us, (unsigned long long)expected_us,
           (unsigned)min_val, (unsigned)max_val, mean);
}

static void cmd_dma_cap(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage dma_cap <N>\n");
        return;
    }

    uint32_t count = 0;
    if (!parse_u32(command->argv[0], &count))
    {
        printf("error: usage dma_cap <N>\n");
        return;
    }

    if (count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        printf("error: N must be 1..8192\n");
        return;
    }

    if (platform_adc_busy() || telemetry_task_get_state() == TELEMETRY_STATE_RUN)
    {
        printf("error: adc busy\n");
        return;
    }

    if (!block_task_start(count, print_dma_cap))
    {
        printf("error: adc busy\n");
        return;
    }
}

static void cmd_tm_start(const command_t *command)
{
    uint32_t period = TELEMETRY_DEFAULT_PERIOD_MS;

    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }

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

static void cmd_stream_start(const command_t *command)
{
    uint8_t bits = 16;

    if (command->argc > 1)
    {
        printf("error: usage stream_start [8|16]\n");
        return;
    }

    if (command->argc == 1)
    {
        uint32_t val = 0;
        if (!parse_u32(command->argv[0], &val) || (val != 8 && val != 16))
        {
            printf("error: usage stream_start [8|16]\n");
            return;
        }
        bits = (uint8_t)val;
    }

    if (platform_adc_busy() || telemetry_task_get_state() == TELEMETRY_STATE_RUN)
    {
        printf("error: adc busy\n");
        return;
    }

    if (!stream_task_start(bits))
    {
        printf("error: adc busy\n");
        return;
    }

    platform_blink_start(STREAM_BLINK_PERIOD_MS);
    led_task_set_state(LED_STATE_OFF);
    printf("stream: started, 44100 Hz, %u bit\n", (unsigned)bits);
}

static void cmd_stream_stop(const command_t *command)
{
    (void)command;

    if (stream_task_get_state() == STREAM_STATE_RUN)
    {
        platform_blink_stop();
        stream_task_stop();
        printf("stream: stopped, %u frames, %u samples, %u overruns\n",
               (unsigned)stream_task_get_frames(),
               (unsigned)stream_task_get_samples(),
               (unsigned)stream_task_get_overruns());
    }
    else
    {
        printf("stream: stopped, 0 frames, 0 samples, 0 overruns\n");
    }
}

void api_print_telemetry(const telemetry_record_t *record)
{
    printf("tm: %llu %.3f %.1f\n", record->time_us, record->voltage_v, record->temp_c);
}

void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes)
{
    platform_write(header, sizeof(*header));
    platform_write(data, bytes);
}

static void cmd_generate_tone(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage generate_tone <Hz>\n");
        return;
    }

    uint32_t freq = 0;
    if (!parse_u32(command->argv[0], &freq))
    {
        printf("error: usage generate_tone <Hz>\n");
        return;
    }

    if (!platform_tone_set(freq))
    {
        printf("error: Hz must be 0 or 20..20000\n");
        return;
    }

    if (freq == 0)
    {
        printf("tone: off\n");
    }
    else
    {
        printf("tone: %u Hz\n", (unsigned)freq);
    }
}

static void cmd_wdt_enable(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage wdt_enable <timeout_ms>\n");
        return;
    }

    uint32_t timeout_ms = 0;
    if (!parse_u32(command->argv[0], &timeout_ms))
    {
        printf("error: usage wdt_enable <timeout_ms>\n");
        return;
    }

    if (!platform_watchdog_enable(timeout_ms))
    {
        printf("error: timeout_ms must be 1..8388\n");
        return;
    }

    printf("wdt: enabled, timeout %u ms\n", (unsigned)timeout_ms);
}

static void cmd_reset_reason(const command_t *command)
{
    (void)command;
    uint32_t restarts = 0;
    bool wdt = platform_watchdog_caused_reset(&restarts);

    if (wdt)
    {
        printf("reset: watchdog, restarts %u\n", (unsigned)restarts);
    }
    else
    {
        printf("reset: other, restarts %u\n", (unsigned)restarts);
    }
}

static void cmd_hang(const command_t *command)
{
    (void)command;
    printf("hang: superloop stopped\n");
    while (1)
    {
    }
}

#define MIC_DEFAULT_WINDOW_MS 100

static void block_stats(const uint16_t *samples, uint32_t count, float *mean, float *rms, float *peak)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        sum += samples[i];
    }
    int32_t center = (int32_t)(sum / count);

    uint64_t squares = 0;
    uint32_t largest = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        int32_t deviation = (int32_t)samples[i] - center;
        uint32_t size = deviation < 0 ? -deviation : deviation;
        squares += (uint64_t)(deviation * deviation);
        if (size > largest)
        {
            largest = size;
        }
    }

    *mean = (float)sum / count;
    *rms = sqrtf((float)squares / count);
    *peak = (float)largest;
}

static void print_mic_bias(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    (void)time_us;
    float mean, rms, peak;
    block_stats(samples, count, &mean, &rms, &peak);
    printf("mic: bias %.3f V\n", (double)platform_adc_volts(mean));
}

static void print_mic_level(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    (void)time_us;
    float mean, rms, peak;
    block_stats(samples, count, &mean, &rms, &peak);

    uint32_t window_ms = (uint32_t)((uint64_t)count * 1000ULL / PLATFORM_SAMPLE_RATE_HZ);
    float mv_per_code = 3300.0f / ADC_LEVELS;

    printf("mic: level %.1f mV rms, peak %.1f mV, %u ms\n",
           (double)(rms * mv_per_code),
           (double)(peak * mv_per_code),
           (unsigned)window_ms);
}

static bool start_block(uint32_t count, block_task_callback_t callback)
{
    if (platform_adc_busy() || telemetry_task_get_state() == TELEMETRY_STATE_RUN)
    {
        printf("error: adc busy\n");
        return false;
    }

    if (!block_task_start(count, callback))
    {
        printf("error: adc busy\n");
        return false;
    }

    return true;
}

static void cmd_mic_bias(const command_t *command)
{
    (void)command;
    start_block(4410, print_mic_bias);
}

static void cmd_mic_level(const command_t *command)
{
    uint32_t window_ms = MIC_DEFAULT_WINDOW_MS;

    if (command->argc > 1)
    {
        printf("error: usage mic_level [<ms>]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &window_ms))
        {
            printf("error: usage mic_level [<ms>]\n");
            return;
        }
    }

    uint64_t count64 = (uint64_t)window_ms * PLATFORM_SAMPLE_RATE_HZ / 1000ULL;
    if (count64 == 0 || count64 > PLATFORM_BLOCK_MAX)
    {
        printf("error: ms must be 1..185\n");
        return;
    }

    start_block((uint32_t)count64, print_mic_level);
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
    {"charge_on", "start charging RC circuit", cmd_charge_on},
    {"charge_off", "stop charging RC circuit", cmd_charge_off},
    {"timer_blink_start", "blink led by timer: timer_blink_start <period_ms>", cmd_timer_blink_start},
    {"timer_blink_stop", "stop timer blink, number of toggles", cmd_timer_blink_stop},
    {"cap_start", "start ADC capture: cap_start <N> <period_us>", cmd_cap_start},
    {"cap_dump", "dump capture results", cmd_cap_dump},
    {"dma_cap", "capture block via DMA: dma_cap <N>", cmd_dma_cap},
    {"tm_start", "start telemetry: tm_start [period_ms]", cmd_tm_start},
    {"tm_stop", "stop telemetry", cmd_tm_stop},
    {"stream_start", "start stream: stream_start [8|16]", cmd_stream_start},
    {"stream_stop", "stop stream", cmd_stream_stop},
    {"generate_tone", "generate tone: generate_tone <Hz>", cmd_generate_tone},
    {"wdt_enable", "enable watchdog: wdt_enable <timeout_ms>", cmd_wdt_enable},
    {"reset_reason", "last reset reason", cmd_reset_reason},
    {"hang", "hang the superloop", cmd_hang},
    {"mic_bias", "microphone bias", cmd_mic_bias},
    {"mic_level", "microphone level: mic_level [ms]", cmd_mic_level}
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);