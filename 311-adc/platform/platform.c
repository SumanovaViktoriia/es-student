#include "platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LED_PIN 25
#define BUTTON_1_PIN 15
#define BUTTON_2_PIN 14

#define ADC_PIN 26
#define ADC_CHANNEL 0
#define ADC_TEMP_CHANNEL 4

#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

#define CHARGE_PIN 22

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BUTTON_1_PIN);
    gpio_set_dir(BUTTON_1_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_1_PIN);

    gpio_init(BUTTON_2_PIN);
    gpio_set_dir(BUTTON_2_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_2_PIN);

    gpio_init(CHARGE_PIN);
    gpio_set_dir(CHARGE_PIN, GPIO_OUT);
    gpio_put(CHARGE_PIN, 0);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_temp_sensor_enabled(true);
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
        case PLATFORM_BUTTON_2: return !gpio_get(BUTTON_2_PIN);
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

void platform_charge_set(bool on)
{
    gpio_put(CHARGE_PIN, on);
}