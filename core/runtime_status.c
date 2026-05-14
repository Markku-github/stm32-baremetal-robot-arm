/**
 ******************************************************************************
 * @file    runtime_status.c
 * @brief   Early boot status capture helpers for reset and fault visibility
 ******************************************************************************
 */

#include "runtime_status.h"

#include "runtime_log.h"
#include "stm32f767_registers.h"

static uint32_t runtime_status_captured_reset_flags = RUNTIME_STATUS_RESET_FLAG_NONE;

static uint32_t runtime_status_translate_reset_flags(uint32_t raw_csr)
{
    uint32_t flags = RUNTIME_STATUS_RESET_FLAG_NONE;

    if ((raw_csr & RCC_CSR_LPWRRSTF) != 0U)
    {
        flags |= RUNTIME_STATUS_RESET_FLAG_LOW_POWER;
    }

    if ((raw_csr & RCC_CSR_WWDGRSTF) != 0U)
    {
        flags |= RUNTIME_STATUS_RESET_FLAG_WINDOW_WATCHDOG;
    }

    if ((raw_csr & RCC_CSR_IWDGRSTF) != 0U)
    {
        flags |= RUNTIME_STATUS_RESET_FLAG_INDEPENDENT_WATCHDOG;
    }

    if ((raw_csr & RCC_CSR_SFTRSTF) != 0U)
    {
        flags |= RUNTIME_STATUS_RESET_FLAG_SOFTWARE;
    }

    if ((raw_csr & RCC_CSR_PORRSTF) != 0U)
    {
        flags |= RUNTIME_STATUS_RESET_FLAG_POWER_ON;
    }

    if ((raw_csr & RCC_CSR_PINRSTF) != 0U)
    {
        flags |= RUNTIME_STATUS_RESET_FLAG_PIN;
    }

    if ((raw_csr & RCC_CSR_BORRSTF) != 0U)
    {
        flags |= RUNTIME_STATUS_RESET_FLAG_BROWNOUT;
    }

    return flags;
}

static void runtime_status_log_reset_flag(uint32_t *remaining_flags, runtime_status_reset_flag_t flag, const char *name)
{
    if (((*remaining_flags) & (uint32_t)flag) == 0U)
    {
        return;
    }

    runtime_log_write_raw(name);
    *remaining_flags &= ~((uint32_t)flag);

    if (*remaining_flags != 0U)
    {
        runtime_log_write_raw(", ");
    }
}

void runtime_status_capture_early_boot(void)
{
    runtime_status_captured_reset_flags = runtime_status_translate_reset_flags(RCC->CSR);
    RCC->CSR |= RCC_CSR_RMVF;
}

uint32_t runtime_status_reset_flags(void)
{
    return runtime_status_captured_reset_flags;
}

bool runtime_status_has_reset_flag(runtime_status_reset_flag_t flag)
{
    return (runtime_status_captured_reset_flags & (uint32_t)flag) != 0U;
}

void runtime_status_log_boot_snapshot(void)
{
    uint32_t remaining_flags = runtime_status_captured_reset_flags;

    if (!runtime_log_begin_line(RUNTIME_LOG_LEVEL_INFO))
    {
        return;
    }

    runtime_log_write_raw("Reset flags: ");

    if (remaining_flags == RUNTIME_STATUS_RESET_FLAG_NONE)
    {
        runtime_log_write_raw("none");
        runtime_log_end_line();
        return;
    }

    runtime_status_log_reset_flag(&remaining_flags, RUNTIME_STATUS_RESET_FLAG_LOW_POWER, "LPWRRSTF");
    runtime_status_log_reset_flag(&remaining_flags, RUNTIME_STATUS_RESET_FLAG_WINDOW_WATCHDOG, "WWDGRSTF");
    runtime_status_log_reset_flag(&remaining_flags, RUNTIME_STATUS_RESET_FLAG_INDEPENDENT_WATCHDOG, "IWDGRSTF");
    runtime_status_log_reset_flag(&remaining_flags, RUNTIME_STATUS_RESET_FLAG_SOFTWARE, "SFTRSTF");
    runtime_status_log_reset_flag(&remaining_flags, RUNTIME_STATUS_RESET_FLAG_POWER_ON, "PORRSTF");
    runtime_status_log_reset_flag(&remaining_flags, RUNTIME_STATUS_RESET_FLAG_PIN, "PINRSTF");
    runtime_status_log_reset_flag(&remaining_flags, RUNTIME_STATUS_RESET_FLAG_BROWNOUT, "BORRSTF");
    runtime_log_end_line();
}