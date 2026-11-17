#include "platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/dma.h"

#define LED_PIN 25
#define BUTTON_1_PIN 15

#define ADC_PIN 26
#define ADC_CHANNEL 0
#define ADC_TEMP_CHANNEL 4

#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

#define LINE_PIN 22

#define UART0_TX_PIN 0
#define UART0_RX_PIN 1
#define UART1_TX_PIN 8
#define UART1_RX_PIN 9

#define UART_BAUD 115200

#define RX_BUFFER_SIZE 128

static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint32_t rx_count;

static uint16_t block_buffer[PLATFORM_BLOCK_MAX];
static volatile uint32_t block_count;
static volatile bool block_running;
static int block_dma_chan = -1;

static uart_inst_t *uart_of(platform_uart_t uart)
{
    return uart == PLATFORM_UART0 ? uart0 : uart1;
}

static void on_uart0_rx(void)
{
    while (uart_is_readable(uart0))
    {
        uint8_t byte = uart_get_hw(uart0)->dr & 0xff;
        if (rx_count < RX_BUFFER_SIZE)
        {
            rx_buffer[rx_count++] = byte;
        }
    }
}

static void on_block_done(void)
{
    dma_hw->ints0 = 1u << block_dma_chan;
    block_running = false;
}

static void adc_free_running(void)
{
    adc_set_clkdiv(0);
    adc_fifo_setup(true, true, 1, false, false);
    adc_run(true);
}

static void adc_one_shot(void)
{
    adc_run(false);
    adc_fifo_drain();
    adc_set_clkdiv(0);
    adc_fifo_setup(true, true, 1, false, false);
}

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BUTTON_1_PIN);
    gpio_set_dir(BUTTON_1_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_1_PIN);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_temp_sensor_enabled(true);

    gpio_init(LINE_PIN);
    platform_line_set(PLATFORM_LINE_PUSH_PULL, false);

    uart_init(uart0, UART_BAUD);
    platform_uart_loop_set(false);

    uart_init(uart1, UART_BAUD);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
    irq_set_enabled(UART0_IRQ, true);

    block_dma_chan = dma_claim_unused_channel(true);
    dma_channel_set_irq0_enabled(block_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, on_block_done);
    irq_set_enabled(DMA_IRQ_0, true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_button_read(platform_button_t button)
{
    switch (button)
    {
        case PLATFORM_BUTTON_1: return !gpio_get(BUTTON_1_PIN);
        default: return false;
    }
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    float voltage = adc_read() * ADC_VREF_V / ADC_LEVELS;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

void platform_line_set(platform_line_mode_t mode, bool high)
{
    if (mode == PLATFORM_LINE_PUSH_PULL)
    {
        gpio_disable_pulls(LINE_PIN);
        gpio_put(LINE_PIN, high);
        gpio_set_dir(LINE_PIN, GPIO_OUT);
        return;
    }

    gpio_pull_up(LINE_PIN);
    gpio_put(LINE_PIN, 0);
    gpio_set_dir(LINE_PIN, high ? GPIO_IN : GPIO_OUT);
}

void platform_uart_loop_set(bool internal)
{
    uint function = internal ? GPIO_FUNC_NULL : GPIO_FUNC_UART;
    gpio_set_function(UART0_TX_PIN, function);
    gpio_set_function(UART0_RX_PIN, function);

    if (internal)
    {
        hw_set_bits(&uart_get_hw(uart0)->cr, UART_UARTCR_LBE_BITS);
    }
    else
    {
        hw_clear_bits(&uart_get_hw(uart0)->cr, UART_UARTCR_LBE_BITS);
    }
}

void platform_uart_clear(platform_uart_t uart)
{
    uart_inst_t *u = uart_of(uart);

    while (uart_is_readable(u))
    {
        (void)uart_get_hw(u)->dr;
    }

    uart_get_hw(u)->rsr = 0;

    if (uart == PLATFORM_UART0)
    {
        rx_count = 0;
    }
}

bool platform_uart_put(platform_uart_t uart, uint8_t byte)
{
    uart_inst_t *u = uart_of(uart);

    if (!uart_is_writable(u))
    {
        return false;
    }

    uart_putc_raw(u, byte);
    return true;
}

bool platform_uart_busy(platform_uart_t uart)
{
    return uart_get_hw(uart_of(uart))->fr & UART_UARTFR_BUSY_BITS;
}

size_t platform_uart_read(platform_uart_t uart, uint8_t *data, size_t size)
{
    uart_inst_t *u = uart_of(uart);
    size_t n = 0;

    if (uart == PLATFORM_UART0)
    {
        while (n < rx_count && n < size)
        {
            data[n] = rx_buffer[n];
            n++;
        }
    }

    while (uart_is_readable(u) && n < size)
    {
        data[n++] = uart_get_hw(u)->dr & 0xff;
    }

    return n;
}

uint32_t platform_uart_errors(platform_uart_t uart)
{
    return uart_get_hw(uart_of(uart))->rsr & 0xf;
}

uint32_t platform_uart_baud_set(platform_uart_t uart, uint32_t baud)
{
    return uart_set_baudrate(uart_of(uart), baud);
}

void platform_uart_parity_set(platform_uart_t uart, platform_parity_t parity)
{
    static const uart_parity_t parities[] = {
        UART_PARITY_NONE,
        UART_PARITY_EVEN,
        UART_PARITY_ODD,
    };

    uart_set_format(uart_of(uart), 8, 1, parities[parity]);
}

void platform_uart_rx_irq_set(bool on)
{
    uart_set_irqs_enabled(uart0, on, false);
}

bool platform_block_start(uint32_t count)
{
    if (block_running || count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        return false;
    }

    block_count = count;
    block_running = true;

    adc_select_input(ADC_CHANNEL);
    adc_free_running();

    dma_channel_config cfg = dma_channel_get_default_config(block_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(block_dma_chan, &cfg,
                          block_buffer,
                          &adc_hw->fifo,
                          count,
                          true);

    return true;
}

bool platform_block_done(void)
{
    return !block_running && block_count > 0;
}

const uint16_t *platform_block_samples(uint32_t *count)
{
    adc_run(false);
    if (count != NULL)
    {
        *count = block_count;
    }
    return block_buffer;
}