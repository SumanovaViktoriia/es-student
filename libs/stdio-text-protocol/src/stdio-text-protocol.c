#include <stdio.h>
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "pico/stdlib.h"

static char line_buffer[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint32_t line_length = 0;
static bool is_truncated = false;
static command_t current_command;

void stdio_text_protocol_init(void)
{
    line_length = 0;
    is_truncated = false;
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT) return NULL;

    if (symbol == '\r' || symbol == '\n')
    {
        putchar('\n');
        if (line_length == 0) return NULL;

        line_buffer[line_length] = '\0';
        current_command.name = NULL;
        current_command.argc = 0;
        current_command.truncated = is_truncated;

        char *p = line_buffer;
        bool in_word = false;

        while (*p != '\0')
        {
            if (*p == ' ')
            {
                *p = '\0';
                in_word = false;
            }
            else if (!in_word)
            {
                in_word = true;
                if (current_command.name == NULL)
                {
                    current_command.name = p;
                }
                else if (current_command.argc < STDIO_TEXT_PROTOCOL_MAX_ARGS)
                {
                    current_command.argv[current_command.argc++] = p;
                }
                else
                {
                    current_command.truncated = true;
                }
            }
            p++;
        }

        line_length = 0;
        is_truncated = false;

        if (current_command.name == NULL) return NULL;
        return &current_command;
    }

    if (symbol == 0x08 || symbol == 0x7f)
    {
        if (line_length > 0)
        {
            line_length--;
            printf("\b \b");
        }
        return NULL;
    }

    if (line_length < STDIO_TEXT_PROTOCOL_LINE_SIZE - 1)
    {
        line_buffer[line_length++] = (char)symbol;
        putchar(symbol);
    }
    else
    {
        is_truncated = true;
    }

    return NULL;
}