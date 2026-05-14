/**
 ******************************************************************************
 * @file    runtime_status.c
 * @brief   Early boot status capture helpers for reset and fault visibility
 ******************************************************************************
 */

#include "runtime_status.h"

#include "runtime_log.h"
#include "stm32f767_registers.h"

#define RUNTIME_STATUS_RETAINED_FAULT_MAGIC 0x46544C54UL

typedef struct
{
    uint32_t magic;
    uint32_t fault_kind;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
} runtime_status_retained_fault_record_t;

static uint32_t runtime_status_captured_reset_flags = RUNTIME_STATUS_RESET_FLAG_NONE;
static runtime_status_fault_kind_t runtime_status_captured_fault_kind = RUNTIME_STATUS_FAULT_NONE;
static uint32_t runtime_status_captured_cfsr = 0U;
static uint32_t runtime_status_captured_hfsr = 0U;
static uint32_t runtime_status_captured_mmfar = 0U;
static uint32_t runtime_status_captured_bfar = 0U;
static volatile runtime_status_retained_fault_record_t runtime_status_retained_fault __attribute__((section(".noinit")));

static void runtime_status_write_hex_u32(uint32_t value)
{
    runtime_log_write_hex_word((uint16_t)(value >> 16U));
    runtime_log_write_hex_word((uint16_t)(value & 0xFFFFU));
}

static const char *runtime_status_fault_kind_name(runtime_status_fault_kind_t fault_kind)
{
    switch (fault_kind)
    {
        case RUNTIME_STATUS_FAULT_DEFAULT_HANDLER:
            return "Default_Handler";

        case RUNTIME_STATUS_FAULT_NMI:
            return "NMI";

        case RUNTIME_STATUS_FAULT_HARDFAULT:
            return "HardFault";

        case RUNTIME_STATUS_FAULT_MEMMANAGE:
            return "MemManage";

        case RUNTIME_STATUS_FAULT_BUSFAULT:
            return "BusFault";

        case RUNTIME_STATUS_FAULT_USAGEFAULT:
            return "UsageFault";

        default:
            return "None";
    }
}

static void runtime_status_clear_retained_fault(void)
{
    runtime_status_retained_fault.magic = 0U;
    runtime_status_retained_fault.fault_kind = (uint32_t)RUNTIME_STATUS_FAULT_NONE;
    runtime_status_retained_fault.cfsr = 0U;
    runtime_status_retained_fault.hfsr = 0U;
    runtime_status_retained_fault.mmfar = 0U;
    runtime_status_retained_fault.bfar = 0U;
}

static void runtime_status_capture_retained_fault(void)
{
    if (runtime_status_retained_fault.magic != RUNTIME_STATUS_RETAINED_FAULT_MAGIC)
    {
        runtime_status_captured_fault_kind = RUNTIME_STATUS_FAULT_NONE;
        runtime_status_captured_cfsr = 0U;
        runtime_status_captured_hfsr = 0U;
        runtime_status_captured_mmfar = 0U;
        runtime_status_captured_bfar = 0U;
        return;
    }

    runtime_status_captured_fault_kind = (runtime_status_fault_kind_t)runtime_status_retained_fault.fault_kind;
    runtime_status_captured_cfsr = runtime_status_retained_fault.cfsr;
    runtime_status_captured_hfsr = runtime_status_retained_fault.hfsr;
    runtime_status_captured_mmfar = runtime_status_retained_fault.mmfar;
    runtime_status_captured_bfar = runtime_status_retained_fault.bfar;
    runtime_status_clear_retained_fault();
}

static void runtime_status_request_system_reset(void)
{
    SCB_AIRCR = SCB_AIRCR_VECTKEY_WRITE | (SCB_AIRCR & SCB_AIRCR_PRIGROUP_MASK) | SCB_AIRCR_SYSRESETREQ;

    for (;;)
    {
    }
}

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
    runtime_status_capture_retained_fault();
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

runtime_status_fault_kind_t runtime_status_fault_kind(void)
{
    return runtime_status_captured_fault_kind;
}

bool runtime_status_has_fault_record(void)
{
    return runtime_status_captured_fault_kind != RUNTIME_STATUS_FAULT_NONE;
}

void runtime_status_capture_fault_and_reset(runtime_status_fault_kind_t fault_kind)
{
    runtime_status_retained_fault.magic = RUNTIME_STATUS_RETAINED_FAULT_MAGIC;
    runtime_status_retained_fault.fault_kind = (uint32_t)fault_kind;
    runtime_status_retained_fault.cfsr = SCB_CFSR;
    runtime_status_retained_fault.hfsr = SCB_HFSR;
    runtime_status_retained_fault.mmfar = SCB_MMFAR;
    runtime_status_retained_fault.bfar = SCB_BFAR;

    runtime_status_request_system_reset();
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

    if (runtime_status_has_fault_record())
    {
        if (runtime_log_begin_line(RUNTIME_LOG_LEVEL_ERROR))
        {
            runtime_log_write_raw("Latched fault: ");
            runtime_log_write_raw(runtime_status_fault_kind_name(runtime_status_captured_fault_kind));
            runtime_log_end_line();
        }

        if (runtime_log_begin_line(RUNTIME_LOG_LEVEL_DEBUG))
        {
            runtime_log_write_raw("Fault detail: CFSR=0x");
            runtime_status_write_hex_u32(runtime_status_captured_cfsr);
            runtime_log_write_raw(", HFSR=0x");
            runtime_status_write_hex_u32(runtime_status_captured_hfsr);
            runtime_log_write_raw(", MMFAR=0x");
            runtime_status_write_hex_u32(runtime_status_captured_mmfar);
            runtime_log_write_raw(", BFAR=0x");
            runtime_status_write_hex_u32(runtime_status_captured_bfar);
            runtime_log_end_line();
        }
    }
}