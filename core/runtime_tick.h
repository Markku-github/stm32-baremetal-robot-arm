/**
 ******************************************************************************
 * @file    runtime_tick.h
 * @brief   Minimal SysTick-backed runtime tick helpers for early V1 control pacing
 ******************************************************************************
 */

#ifndef RUNTIME_TICK_H
#define RUNTIME_TICK_H

#include <stdbool.h>
#include <stdint.h>

#define RUNTIME_TICK_FREQUENCY_HZ 1000U
#define RUNTIME_TICK_FREQUENCY_HZ_TEXT "1000"

bool runtime_tick_init(void);
uint32_t runtime_tick_now_ms(void);
bool runtime_tick_deadline_reached(uint32_t start_ms, uint32_t delay_ms, uint32_t now_ms);
bool runtime_tick_periodic_due(uint32_t now_ms, uint32_t period_ms, uint32_t *last_run_ms);

#endif
