#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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
#include "edge-task/edge-task.h"
#include "uart-task/uart-task.h"
#include "block-task/block-task.h"

#define DEFAULT_PI_TERMS 1000000
#define ECHO_MAX 32

#define BLOCK_SIZE 64

#define SCOPE_SAMPLES PLATFORM_BLOCK_MAX
#define SCOPE_BAUD_MIN 9600
#define SCOPE_BAUD_MAX 115200
#define SCOPE_TEXT_MAX 4

static const char text_line[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

static uint8_t scope_rx[UART_TASK_MAX];
static size_t scope_rx_size;
static uint32_t scope_rx_errors;

static bool parse_u32(const char *text, uint32_t *number)
{
    if (text == NULL) return false;
    char *end;
    unsigned long val = strtoul(text, &end, 10);
    if (*end != '\0') return false;
    *number = (uint32_t)val;
    return true;
}

static bool parse_i32(const char *text, int32_t *number)
{
    if (text == NULL) return false;
    char *end;
    long val = strtol(text, &end, 10);
    if (*end != '\0') return false;
    *number = (int32_t)val;
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
    printf("button: %s, presses %u\n",
           button_task_is_pressed(PLATFORM_BUTTON_1) ? "pressed" : "released",
           (unsigned)button_task_get_press_count(PLATFORM_BUTTON_1));
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

static void print_edge(platform_line_mode_t mode, bool ok, uint32_t rise_us, uint32_t fall_us)
{
    const char *name = (mode == PLATFORM_LINE_PUSH_PULL) ? "pp" : "od";

    if (!ok)
    {
        printf("edge %s: timeout\n", name);
        return;
    }

    printf("edge %s: rise %.1f ms, fall %.1f ms\n",
           name, rise_us / 1000.0f, fall_us / 1000.0f);
}

static void cmd_edge(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage edge pp|od\n");
        return;
    }

    platform_line_mode_t mode;
    if (strcmp(command->argv[0], "pp") == 0)
    {
        mode = PLATFORM_LINE_PUSH_PULL;
    }
    else if (strcmp(command->argv[0], "od") == 0)
    {
        mode = PLATFORM_LINE_OPEN_DRAIN;
    }
    else
    {
        printf("error: usage edge pp|od\n");
        return;
    }

    if (!edge_task_start(mode, print_edge))
    {
        printf("error: edge is busy\n");
        return;
    }
}

static void print_bytes(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        putchar(isprint(data[i]) ? data[i] : '.');
    }
}

static void print_errors(uint32_t errors)
{
    printf("err:");
    if (errors == 0)
    {
        printf(" none");
        return;
    }
    if (errors & PLATFORM_UART_FE) printf(" FE");
    if (errors & PLATFORM_UART_PE) printf(" PE");
    if (errors & PLATFORM_UART_BE) printf(" BE");
    if (errors & PLATFORM_UART_OE) printf(" OE");
}

static void print_echo(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("rx: ");
    print_bytes(data, size);
    printf(" ");
    print_errors(errors);
    printf("\n");
}

static void cmd_loop(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage loop internal|pins\n");
        return;
    }

    if (strcmp(command->argv[0], "internal") == 0)
    {
        platform_uart_loop_set(true);
        printf("loop: internal\n");
    }
    else if (strcmp(command->argv[0], "pins") == 0)
    {
        platform_uart_loop_set(false);
        printf("loop: pins\n");
    }
    else
    {
        printf("error: usage loop internal|pins\n");
    }
}

static void cmd_echo(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage echo <text>\n");
        return;
    }

    const char *text = command->argv[0];
    size_t len = strlen(text);

    if (len > ECHO_MAX)
    {
        printf("error: text longer than %d\n", ECHO_MAX);
        return;
    }

    if (!uart_task_start(PLATFORM_UART0, PLATFORM_UART0,
                         (const uint8_t *)text, len, print_echo))
    {
        printf("error: uart is busy\n");
        return;
    }
}

static void print_link_back(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("1->0: ");
    print_bytes(data, size);
    printf(" ");
    print_errors(errors);
    printf("\n");
}

static void print_link_forth(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("0->1: ");
    print_bytes(data, size);
    printf(" ");
    print_errors(errors);
    printf("\n");

    uart_task_start(PLATFORM_UART1, PLATFORM_UART0, data, size, print_link_back);
}

static void cmd_link(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage link <text>\n");
        return;
    }

    const char *text = command->argv[0];
    size_t len = strlen(text);

    if (len > ECHO_MAX)
    {
        printf("error: text longer than %d\n", ECHO_MAX);
        return;
    }

    if (!uart_task_start(PLATFORM_UART0, PLATFORM_UART1,
                         (const uint8_t *)text, len, print_link_forth))
    {
        printf("error: uart is busy\n");
        return;
    }
}

static void cmd_baud_shift(const command_t *command)
{
    int32_t percent = 0;

    if (command->argc != 1 || !parse_i32(command->argv[0], &percent))
    {
        printf("error: usage baud_shift <percent>\n");
        return;
    }

    if (percent < -50 || percent > 50)
    {
        printf("error: percent must be -50..50\n");
        return;
    }

    uint32_t baud = 115200 * (100 + percent) / 100;
    uint32_t actual = platform_uart_baud_set(PLATFORM_UART1, baud);
    printf("baud_shift: uart1 %u baud\n", (unsigned)actual);
}

static void print_text(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("text: %u chars ", (unsigned)size);
    print_bytes(data, size);
    printf(" ");
    print_errors(errors);
    printf("\n");
}

static void cmd_text(const command_t *command)
{
    (void)command;
    if (!uart_task_start(PLATFORM_UART1, PLATFORM_UART0,
                         (const uint8_t *)text_line, sizeof(text_line) - 1,
                         print_text))
    {
        printf("error: uart is busy\n");
    }
}

static bool parse_parity(const char *text, platform_parity_t *parity)
{
    if (strcmp(text, "none") == 0) { *parity = PLATFORM_PARITY_NONE; return true; }
    if (strcmp(text, "even") == 0) { *parity = PLATFORM_PARITY_EVEN; return true; }
    if (strcmp(text, "odd") == 0) { *parity = PLATFORM_PARITY_ODD; return true; }
    return false;
}

static void cmd_parity(const command_t *command)
{
    if (command->argc != 2)
    {
        printf("error: usage parity none|even|odd none|even|odd\n");
        return;
    }

    platform_parity_t rx_parity, tx_parity;
    if (!parse_parity(command->argv[0], &rx_parity) ||
        !parse_parity(command->argv[1], &tx_parity))
    {
        printf("error: usage parity none|even|odd none|even|odd\n");
        return;
    }

    platform_uart_parity_set(PLATFORM_UART0, rx_parity);
    platform_uart_parity_set(PLATFORM_UART1, tx_parity);

    printf("parity: uart0 %s, uart1 %s\n", command->argv[0], command->argv[1]);
}

static void print_block(const uint8_t *data, size_t size, uint32_t errors)
{
    (void)data;
    (void)size;

    platform_uart_rx_irq_set(false);

    static uint8_t block[BLOCK_SIZE];
    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        block[i] = (uint8_t)i;
    }

    uint32_t match = 1;
    if (size != BLOCK_SIZE)
    {
        match = 0;
    }
    else if (memcmp(data, block, BLOCK_SIZE) != 0)
    {
        match = 0;
    }

    printf("block irq: %u/%u %s, ", (unsigned)size, BLOCK_SIZE, match ? "match" : "mismatch");
    print_errors(errors);
    printf("\n");
}

static void print_block_poll(const uint8_t *data, size_t size, uint32_t errors)
{
    static uint8_t block[BLOCK_SIZE];
    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        block[i] = (uint8_t)i;
    }

    uint32_t match = 1;
    if (size != BLOCK_SIZE)
    {
        match = 0;
    }
    else if (memcmp(data, block, BLOCK_SIZE) != 0)
    {
        match = 0;
    }

    printf("block poll: %u/%u %s, ", (unsigned)size, BLOCK_SIZE, match ? "match" : "mismatch");
    print_errors(errors);
    printf("\n");
}

static void cmd_block(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage block irq|poll\n");
        return;
    }

    static uint8_t block[BLOCK_SIZE];
    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        block[i] = (uint8_t)i;
    }

    if (strcmp(command->argv[0], "irq") == 0)
    {
        platform_uart_rx_irq_set(true);
        if (!uart_task_start(PLATFORM_UART1, PLATFORM_UART0, block, BLOCK_SIZE, print_block))
        {
            platform_uart_rx_irq_set(false);
            printf("error: uart is busy\n");
        }
    }
    else if (strcmp(command->argv[0], "poll") == 0)
    {
        platform_uart_rx_irq_set(false);
        if (!uart_task_start(PLATFORM_UART1, PLATFORM_UART0, block, BLOCK_SIZE, print_block_poll))
        {
            printf("error: uart is busy\n");
        }
    }
    else
    {
        printf("error: usage block irq|poll\n");
    }
}

static void print_scope_rx(const uint8_t *data, size_t size, uint32_t errors)
{
    scope_rx_size = size;
    scope_rx_errors = errors;
    if (size > sizeof(scope_rx)) size = sizeof(scope_rx);
    memcpy(scope_rx, data, size);
}

static void print_scope(bool ok)
{
    (void)ok;

    platform_uart_baud_set(PLATFORM_UART0, 115200);
    platform_uart_baud_set(PLATFORM_UART1, 115200);

    uint32_t count = 0;
    const uint16_t *samples = platform_block_samples(&count);

    printf("scope: %u samples, %u Hz, %u baud, uart1 ",
           (unsigned)count, PLATFORM_SAMPLE_RATE_HZ, 115200);
    print_bytes(scope_rx, scope_rx_size);
    printf(" ");
    print_errors(scope_rx_errors);
    printf("\n");

    for (uint32_t i = 0; i < count; i++)
    {
        printf("%u", (unsigned)samples[i]);
        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
        else
        {
            printf(" ");
        }
    }
    printf("scope: end\n");
}

static void cmd_scope(const command_t *command)
{
    if (command->argc != 2)
    {
        printf("error: usage scope <baud> <text>\n");
        return;
    }

    uint32_t baud = 0;
    if (!parse_u32(command->argv[0], &baud) || baud < SCOPE_BAUD_MIN || baud > SCOPE_BAUD_MAX)
    {
        printf("error: usage scope <baud> <text>\n");
        return;
    }

    const char *text = command->argv[1];
    size_t len = strlen(text);
    if (len > SCOPE_TEXT_MAX)
    {
        printf("error: usage scope <baud> <text>\n");
        return;
    }

    scope_rx_size = 0;
    scope_rx_errors = 0;

    platform_uart_baud_set(PLATFORM_UART0, baud);
    platform_uart_baud_set(PLATFORM_UART1, baud);

    if (!block_task_start(SCOPE_SAMPLES, print_scope))
    {
        platform_uart_baud_set(PLATFORM_UART0, 115200);
        platform_uart_baud_set(PLATFORM_UART1, 115200);
        printf("error: adc busy\n");
        return;
    }

    if (!uart_task_start(PLATFORM_UART0, PLATFORM_UART1,
                         (const uint8_t *)text, len, print_scope_rx))
    {
        printf("error: uart is busy\n");
    }
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
    {"edge", "rise and fall of the RC line: edge pp|od", cmd_edge},
    {"loop", "UART0 loopback: loop internal|pins", cmd_loop},
    {"echo", "text through UART0 and back: echo <text>", cmd_echo},
    {"link", "text from UART0 to UART1 and back: link <text>", cmd_link},
    {"baud_shift", "shift UART1 baud rate: baud_shift <percent>", cmd_baud_shift},
    {"text", "32 characters from UART1 to UART0", cmd_text},
    {"parity", "parity of UART0 and UART1: parity <rx> <tx>", cmd_parity},
    {"block", "64 bytes from UART1 to UART0: block irq|poll", cmd_block},
    {"scope", "UART0 line on ADC: scope <baud> <text>", cmd_scope},
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);