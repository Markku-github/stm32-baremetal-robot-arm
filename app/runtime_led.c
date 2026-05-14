/**
 ******************************************************************************
 * @file    runtime_led.c
 * @brief   Runtime LED state patterns for early V1 observability
 ******************************************************************************
 */

#include "runtime_led.h"

#include "board_nucleo_f767zi.h"

#define RUNTIME_LED_FAST_BLINK_TICKS 250U
#define RUNTIME_LED_SLOW_BLINK_TICKS 1000U

static bool runtime_led_is_valid_state(runtime_led_state_t state)
{
    return (state == RUNTIME_LED_STATE_STARTUP)
        || (state == RUNTIME_LED_STATE_READY_IDLE)
        || (state == RUNTIME_LED_STATE_DEGRADED)
        || (state == RUNTIME_LED_STATE_FAULT_LATCHED);
}

static uint32_t runtime_led_blink_ticks(runtime_led_state_t state)
{
    switch (state)
    {
        case RUNTIME_LED_STATE_STARTUP:
            return RUNTIME_LED_FAST_BLINK_TICKS;

        case RUNTIME_LED_STATE_READY_IDLE:
        case RUNTIME_LED_STATE_DEGRADED:
            return RUNTIME_LED_SLOW_BLINK_TICKS;

        default:
            return 0U;
    }
}

static void runtime_led_apply_outputs(const runtime_led_t *runtime_led)
{
    bool ld1_enabled = false;
    bool ld2_enabled = false;
    bool ld3_enabled = false;

    if (runtime_led == 0)
    {
        return;
    }

    switch (runtime_led->state)
    {
        case RUNTIME_LED_STATE_STARTUP:
            ld2_enabled = runtime_led->blink_phase_on;
            break;

        case RUNTIME_LED_STATE_READY_IDLE:
            ld1_enabled = runtime_led->blink_phase_on;
            break;

        case RUNTIME_LED_STATE_DEGRADED:
            ld1_enabled = true;
            ld3_enabled = runtime_led->blink_phase_on;
            break;

        case RUNTIME_LED_STATE_FAULT_LATCHED:
            ld3_enabled = true;
            break;

        default:
            break;
    }

    board_nucleo_f767zi_set_led(BOARD_NUCLEO_F767ZI_LED_LD1, ld1_enabled);
    board_nucleo_f767zi_set_led(BOARD_NUCLEO_F767ZI_LED_LD2, ld2_enabled);
    board_nucleo_f767zi_set_led(BOARD_NUCLEO_F767ZI_LED_LD3, ld3_enabled);
}

void runtime_led_init(runtime_led_t *runtime_led)
{
    if (runtime_led == 0)
    {
        return;
    }

    runtime_led->state = RUNTIME_LED_STATE_STARTUP;
    runtime_led->tick_counter = 0U;
    runtime_led->blink_phase_on = false;
    runtime_led_apply_outputs(runtime_led);
}

void runtime_led_set_state(runtime_led_t *runtime_led, runtime_led_state_t state)
{
    if ((runtime_led == 0) || !runtime_led_is_valid_state(state))
    {
        return;
    }

    runtime_led->state = state;
    runtime_led->tick_counter = 0U;
    runtime_led->blink_phase_on = true;
    runtime_led_apply_outputs(runtime_led);
}

void runtime_led_tick(runtime_led_t *runtime_led)
{
    const uint32_t blink_ticks = (runtime_led != 0) ? runtime_led_blink_ticks(runtime_led->state) : 0U;

    if ((runtime_led == 0) || !runtime_led_is_valid_state(runtime_led->state))
    {
        return;
    }

    if (blink_ticks == 0U)
    {
        return;
    }

    runtime_led->tick_counter++;
    if (runtime_led->tick_counter < blink_ticks)
    {
        return;
    }

    runtime_led->tick_counter = 0U;
    runtime_led->blink_phase_on = !runtime_led->blink_phase_on;
    runtime_led_apply_outputs(runtime_led);
}
