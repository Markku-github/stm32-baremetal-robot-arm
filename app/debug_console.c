/**
 ******************************************************************************
 * @file    debug_console.c
 * @brief   Board-backed debug UART output helpers for app-level modules
 ******************************************************************************
 */

#include "debug_console.h"

#include "board_nucleo_f767zi.h"

static void debug_console_write_hex_nibble(uint8_t nibble)
{
    const uint8_t character = (nibble < 10U) ? (uint8_t)('0' + nibble) : (uint8_t)('A' + (nibble - 10U));

    (void)board_nucleo_f767zi_write_debug_byte(character);
}

void debug_console_write_prompt(void)
{
    board_nucleo_f767zi_write_debug_string("> ");
}

void debug_console_write_hex_byte(uint8_t value)
{
    debug_console_write_hex_nibble((uint8_t)(value >> 4U));
    debug_console_write_hex_nibble((uint8_t)(value & 0x0FU));
}

void debug_console_write_hex_word(uint16_t value)
{
    debug_console_write_hex_byte((uint8_t)(value >> 8U));
    debug_console_write_hex_byte((uint8_t)(value & 0x00FFU));
}

void debug_console_write_string_adapter(void *context, const char *text)
{
    (void)context;
    board_nucleo_f767zi_write_debug_string(text);
}

void debug_console_write_byte_adapter(void *context, uint8_t byte)
{
    (void)context;
    (void)board_nucleo_f767zi_write_debug_byte(byte);
}

void debug_console_write_prompt_adapter(void *context)
{
    (void)context;
    debug_console_write_prompt();
}