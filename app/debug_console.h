/**
 ******************************************************************************
 * @file    debug_console.h
 * @brief   Board-backed debug UART output helpers for app-level modules
 ******************************************************************************
 */

#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#include <stdint.h>

void debug_console_write_prompt(void);
void debug_console_write_hex_byte(uint8_t value);
void debug_console_write_hex_word(uint16_t value);

void debug_console_write_string_adapter(void *context, const char *text);
void debug_console_write_byte_adapter(void *context, uint8_t byte);
void debug_console_write_prompt_adapter(void *context);

#endif