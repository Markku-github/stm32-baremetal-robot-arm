/**
 ******************************************************************************
 * @file    runtime_tick.c
 * @brief   Minimal SysTick-backed runtime tick helpers for early V1 control pacing
 ******************************************************************************
 */

#include "runtime_tick.h"

#include "stm32f767_registers.h"
#include "system_stm32f7xx.h"

#define RUNTIME_TICK_FREQUENCY_HZ 1000U

static volatile uint32_t runtime_tick_milliseconds = 0U;

bool runtime_tick_init(void)
{
    uint32_t reload_value;

    if (SystemCoreClock < RUNTIME_TICK_FREQUENCY_HZ)
    {
        return false;
    }

    reload_value = (SystemCoreClock / RUNTIME_TICK_FREQUENCY_HZ) - 1U;
    if (reload_value > SYSTICK_LOAD_RELOAD_MASK)
    {
        return false;
    }

    runtime_tick_milliseconds = 0U;
    SYSTICK_LOAD = reload_value;
    SYSTICK_VAL = 0U;
    SYSTICK_CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
    return true;
}

uint32_t runtime_tick_now_ms(void)
{
    return runtime_tick_milliseconds;
}

bool runtime_tick_deadline_reached(uint32_t start_ms, uint32_t delay_ms, uint32_t now_ms)
{
    return (uint32_t)(now_ms - start_ms) >= delay_ms;
}

bool runtime_tick_periodic_due(uint32_t now_ms, uint32_t period_ms, uint32_t *last_run_ms)
{
    if ((last_run_ms == 0) || (period_ms == 0U))
    {
        return false;
    }

    if ((uint32_t)(now_ms - *last_run_ms) < period_ms)
    {
        return false;
    }

    *last_run_ms = now_ms;
    return true;
}

void SysTick_Handler(void)
{
    runtime_tick_milliseconds++;
}
