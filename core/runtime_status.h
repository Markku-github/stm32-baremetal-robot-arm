/**
 ******************************************************************************
 * @file    runtime_status.h
 * @brief   Early boot status capture helpers for reset and fault visibility
 ******************************************************************************
 */

#ifndef RUNTIME_STATUS_H
#define RUNTIME_STATUS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    RUNTIME_STATUS_RESET_FLAG_NONE = 0,
    RUNTIME_STATUS_RESET_FLAG_LOW_POWER = (1UL << 0),
    RUNTIME_STATUS_RESET_FLAG_WINDOW_WATCHDOG = (1UL << 1),
    RUNTIME_STATUS_RESET_FLAG_INDEPENDENT_WATCHDOG = (1UL << 2),
    RUNTIME_STATUS_RESET_FLAG_SOFTWARE = (1UL << 3),
    RUNTIME_STATUS_RESET_FLAG_POWER_ON = (1UL << 4),
    RUNTIME_STATUS_RESET_FLAG_PIN = (1UL << 5),
    RUNTIME_STATUS_RESET_FLAG_BROWNOUT = (1UL << 6),
} runtime_status_reset_flag_t;

typedef enum
{
    RUNTIME_STATUS_FAULT_NONE = 0,
    RUNTIME_STATUS_FAULT_DEFAULT_HANDLER,
    RUNTIME_STATUS_FAULT_NMI,
    RUNTIME_STATUS_FAULT_HARDFAULT,
    RUNTIME_STATUS_FAULT_MEMMANAGE,
    RUNTIME_STATUS_FAULT_BUSFAULT,
    RUNTIME_STATUS_FAULT_USAGEFAULT,
} runtime_status_fault_kind_t;

void runtime_status_capture_early_boot(void);
uint32_t runtime_status_reset_flags(void);
bool runtime_status_has_reset_flag(runtime_status_reset_flag_t flag);
runtime_status_fault_kind_t runtime_status_fault_kind(void);
bool runtime_status_has_fault_record(void);
void runtime_status_capture_fault_and_reset(runtime_status_fault_kind_t fault_kind);
void runtime_status_log_boot_snapshot(void);

#endif