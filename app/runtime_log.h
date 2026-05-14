/**
 ******************************************************************************
 * @file    runtime_log.h
 * @brief   Minimal runtime log output helpers for the early V1 observability path
 ******************************************************************************
 */

#ifndef RUNTIME_LOG_H
#define RUNTIME_LOG_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    RUNTIME_LOG_LEVEL_DEBUG = 0,
    RUNTIME_LOG_LEVEL_INFO,
    RUNTIME_LOG_LEVEL_WARNING,
    RUNTIME_LOG_LEVEL_ERROR,
} runtime_log_level_t;

#ifndef RUNTIME_LOG_MIN_LEVEL
#define RUNTIME_LOG_MIN_LEVEL RUNTIME_LOG_LEVEL_DEBUG
#endif

bool runtime_log_init(void);
bool runtime_log_is_ready(void);
void runtime_log_enable_debug_fallback(bool enabled);
void runtime_log_write_raw(const char *message);
void runtime_log_write_hex_byte(uint8_t value);
void runtime_log_write_hex_word(uint16_t value);
void runtime_log_write_line(runtime_log_level_t level, const char *message);

#endif