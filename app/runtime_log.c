/**
 ******************************************************************************
 * @file    runtime_log.c
 * @brief   Minimal runtime log output helpers for the early V1 observability path
 ******************************************************************************
 */

#include "runtime_log.h"

#include <stdint.h>

#include "board_nucleo_f767zi.h"

static bool runtime_log_ready = false;
static bool runtime_log_debug_fallback_enabled = false;

static char runtime_log_hex_character(uint8_t nibble)
{
    return (nibble < 10U) ? (char)('0' + nibble) : (char)('A' + (nibble - 10U));
}

static bool runtime_log_level_enabled(runtime_log_level_t level)
{
    switch (level)
    {
        case RUNTIME_LOG_LEVEL_DEBUG:
            return RUNTIME_LOG_MIN_LEVEL <= RUNTIME_LOG_LEVEL_DEBUG;

        case RUNTIME_LOG_LEVEL_INFO:
            return RUNTIME_LOG_MIN_LEVEL <= RUNTIME_LOG_LEVEL_INFO;

        case RUNTIME_LOG_LEVEL_WARNING:
            return RUNTIME_LOG_MIN_LEVEL <= RUNTIME_LOG_LEVEL_WARNING;

        case RUNTIME_LOG_LEVEL_ERROR:
            return RUNTIME_LOG_MIN_LEVEL <= RUNTIME_LOG_LEVEL_ERROR;

        default:
            return false;
    }
}

static const char *runtime_log_level_prefix(runtime_log_level_t level)
{
    switch (level)
    {
        case RUNTIME_LOG_LEVEL_DEBUG:
            return "[DEBUG] ";

        case RUNTIME_LOG_LEVEL_INFO:
            return "[INFO] ";

        case RUNTIME_LOG_LEVEL_WARNING:
            return "[WARNING] ";

        case RUNTIME_LOG_LEVEL_ERROR:
            return "[ERROR] ";

        default:
            return "[INFO] ";
    }
}

static void runtime_log_write_sink_string(const char *message)
{
    if (message == 0)
    {
        return;
    }

    if (runtime_log_ready)
    {
        board_nucleo_f767zi_write_log_string(message);
        return;
    }

    if (runtime_log_debug_fallback_enabled)
    {
        board_nucleo_f767zi_write_debug_string(message);
    }
}

bool runtime_log_init(void)
{
    runtime_log_ready = board_nucleo_f767zi_init_log_uart() == BSP_UART_OK;
    return runtime_log_ready;
}

bool runtime_log_is_ready(void)
{
    return runtime_log_ready;
}

void runtime_log_enable_debug_fallback(bool enabled)
{
    runtime_log_debug_fallback_enabled = enabled;
}

bool runtime_log_begin_line(runtime_log_level_t level)
{
    if (!runtime_log_level_enabled(level))
    {
        return false;
    }

    runtime_log_write_sink_string(runtime_log_level_prefix(level));
    return true;
}

void runtime_log_end_line(void)
{
    runtime_log_write_sink_string("\r\n");
}

void runtime_log_write_raw(const char *message)
{
    runtime_log_write_sink_string(message);
}

void runtime_log_write_hex_byte(uint8_t value)
{
    char text[3];

    text[0] = runtime_log_hex_character((uint8_t)(value >> 4U));
    text[1] = runtime_log_hex_character((uint8_t)(value & 0x0FU));
    text[2] = '\0';

    runtime_log_write_raw(text);
}

void runtime_log_write_hex_word(uint16_t value)
{
    char text[5];

    text[0] = runtime_log_hex_character((uint8_t)(value >> 12U));
    text[1] = runtime_log_hex_character((uint8_t)((value >> 8U) & 0x0FU));
    text[2] = runtime_log_hex_character((uint8_t)((value >> 4U) & 0x0FU));
    text[3] = runtime_log_hex_character((uint8_t)(value & 0x0FU));
    text[4] = '\0';

    runtime_log_write_raw(text);
}

void runtime_log_write_line(runtime_log_level_t level, const char *message)
{
    if (message == 0)
    {
        return;
    }

    if (!runtime_log_begin_line(level))
    {
        return;
    }

    runtime_log_write_sink_string(message);
    runtime_log_end_line();
}