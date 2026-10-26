#pragma once

#include <stdbool.h>
#include <stdint.h>

#define STDIO_TEXT_PROTOCOL_LINE_SIZE 64
#define STDIO_TEXT_PROTOCOL_MAX_ARGS 4

typedef struct
{
    const char *name;
    uint32_t argc;
    const char *argv[STDIO_TEXT_PROTOCOL_MAX_ARGS];
    bool truncated;
} command_t;

void stdio_text_protocol_init(void);
const command_t *stdio_text_protocol_handle(void);