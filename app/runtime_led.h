/**
 ******************************************************************************
 * @file    runtime_led.h
 * @brief   Runtime LED state patterns for early V1 observability
 ******************************************************************************
 */

#ifndef RUNTIME_LED_H
#define RUNTIME_LED_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    RUNTIME_LED_STATE_STARTUP = 0,
    RUNTIME_LED_STATE_READY_IDLE,
    RUNTIME_LED_STATE_DEGRADED,
    RUNTIME_LED_STATE_FAULT_LATCHED,
} runtime_led_state_t;

typedef struct
{
    runtime_led_state_t state;
    uint32_t tick_counter;
    bool blink_phase_on;
} runtime_led_t;

void runtime_led_init(runtime_led_t *runtime_led);
void runtime_led_set_state(runtime_led_t *runtime_led, runtime_led_state_t state);
void runtime_led_tick(runtime_led_t *runtime_led);

#endif
