#include "memory.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/regs/addressmap.h"
#include "led.h"
#include "device.h"
#include "command.h"

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

int main(void);

uint32_t data_variable = 100;
uint32_t bss_variable;

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void mem_info(void)
{
    printf("area       start      end        size\n");

    row("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("sram", SRAM_BASE, SRAM_BASE + 264 * 1024);
    row("rom", ROM_BASE, ROM_BASE + 16 * 1024);

    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);
    row("free", (uintptr_t)&__flash_binary_end, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);

    row("data flash", (uintptr_t)&__etext,
        (uintptr_t)&__etext + ((uintptr_t)&__data_end__ - (uintptr_t)&__data_start__));
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);
    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

    unsigned boot2 = (unsigned)(&__boot2_end__ - &__boot2_start__);
    unsigned text = (unsigned)(&__etext - &__boot2_end__);
    unsigned data = (unsigned)(&__data_end__ - &__data_start__);
    unsigned bss = (unsigned)(&__bss_end__ - &__bss_start__);
    unsigned heap = (unsigned)(&__HeapLimit - &__bss_end__);
    unsigned stack = (unsigned)(&__StackTop - &__StackBottom);

    printf("\ntotal\n");
    printf("  flash image    %u = boot2 %u + text %u + data %u\n",
           boot2 + text + data, boot2, text, data);
    printf("  flash free    %u of %u\n",
           (unsigned)(PICO_FLASH_SIZE_BYTES - (boot2 + text + data)),
           (unsigned)PICO_FLASH_SIZE_BYTES);
    printf("  ram used        %u = data %u + bss %u\n",
           data + bss, data, bss);
    printf("  ram free       %u for heap and %u for stack\n",
           heap, stack);
}

void fw_info(void)
{
    data_variable = data_variable + 1;
    bss_variable = bss_variable + 1;

    uint16_t *main_code = (uint16_t *)((uintptr_t)main & ~1u);
    uint16_t *fw_info_code = (uint16_t *)((uintptr_t)fw_info & ~1u);

    printf("object          address     value\n");
    printf("main            0x%08x  0x%04x\n",
           (unsigned)(uintptr_t)main, *main_code);
    printf("fw_info         0x%08x  0x%04x\n",
           (unsigned)(uintptr_t)fw_info, *fw_info_code);

    printf("commands        0x%08x\n", (unsigned)(uintptr_t)commands);
    for (uint i = 0; i < command_count; i++)
    {
        printf("- %-13s 0x%08x\n",
               commands[i].name,
               (unsigned)(uintptr_t)commands[i].handler);
    }

    printf("DEVICE_PROJECT  0x%08x  %s\n",
           (unsigned)(uintptr_t)DEVICE_PROJECT, DEVICE_PROJECT);
    printf("DEVICE_BOARD    0x%08x  %s\n",
           (unsigned)(uintptr_t)DEVICE_BOARD, DEVICE_BOARD);

    printf("data_variable   0x%08x  %u\n",
           (unsigned)(uintptr_t)&data_variable, data_variable);
    printf("bss_variable    0x%08x  %u\n",
           (unsigned)(uintptr_t)&bss_variable, bss_variable);

    uint32_t stack_variable = 1946;
    uint32_t *heap_variable = malloc(sizeof(uint32_t));

    if (heap_variable != NULL)
    {
        *heap_variable = 1951;
        printf("stack_variable  0x%08x  %u\n",
               (unsigned)(uintptr_t)&stack_variable, stack_variable);
        printf("heap_variable   0x%08x  %u\n",
               (unsigned)(uintptr_t)heap_variable, *heap_variable);
    }

    free(heap_variable);
}

#define VECTOR_TABLE 0x10000100
#define GPIO_IN_REG  0xd0000004

void boot_info(void)
{
    const uint32_t *vectors = (const uint32_t *)VECTOR_TABLE;

    uint32_t stack_top = vectors[0];
    uint32_t reset_handler = vectors[1];

    volatile uint32_t *gpio_in = (uint32_t *)GPIO_IN_REG;
    uint32_t level = (*gpio_in >> led_pin()) & 1u;

    printf("vector table   0x%08x\n", VECTOR_TABLE);
    printf("  stack top    0x%08x\n", (unsigned)stack_top);
    printf("  reset        0x%08x\n", (unsigned)reset_handler);
    printf("  reset (even) 0x%08x\n", (unsigned)(reset_handler & ~1u));
    printf("gpio in        0x%08x\n", GPIO_IN_REG);
    printf("  led bit      %u\n", (unsigned)level);
    printf("  gpio_get     %u\n", (unsigned)gpio_get(led_pin()));
}