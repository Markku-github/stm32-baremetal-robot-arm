#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "board_nucleo_f767zi.h"
#include "runtime_led.h"

static bool led_states[3] = { false, false, false };

#define TEST_ASSERT_TRUE(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            printf("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FALSE(condition) \
    do \
    { \
        if (condition) \
        { \
            printf("Assertion failed at %s:%d: expected false for %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

static void reset_led_states(void)
{
    uint32_t index;

    for (index = 0U; index < (sizeof(led_states) / sizeof(led_states[0])); index++)
    {
        led_states[index] = false;
    }
}

void board_nucleo_f767zi_set_led(board_nucleo_f767zi_led_t led, bool enabled)
{
    if ((uint32_t)led >= (sizeof(led_states) / sizeof(led_states[0])))
    {
        return;
    }

    led_states[(uint32_t)led] = enabled;
}

static bool test_runtime_led_init_turns_all_leds_off(void)
{
    runtime_led_t runtime_led;

    reset_led_states();
    runtime_led_init(&runtime_led);

    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD1]);
    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD2]);
    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD3]);
    return true;
}

static bool test_startup_state_blinks_blue_led(void)
{
    runtime_led_t runtime_led;
    uint32_t tick_count;

    reset_led_states();
    runtime_led_init(&runtime_led);
    runtime_led_set_state(&runtime_led, RUNTIME_LED_STATE_STARTUP);

    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD1]);
    TEST_ASSERT_TRUE(led_states[BOARD_NUCLEO_F767ZI_LED_LD2]);
    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD3]);

    for (tick_count = 0U; tick_count < 100U; tick_count++)
    {
        runtime_led_tick(&runtime_led);
    }

    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD2]);
    return true;
}

static bool test_ready_state_blinks_green_led(void)
{
    runtime_led_t runtime_led;

    reset_led_states();
    runtime_led_init(&runtime_led);
    runtime_led_set_state(&runtime_led, RUNTIME_LED_STATE_READY_IDLE);

    TEST_ASSERT_TRUE(led_states[BOARD_NUCLEO_F767ZI_LED_LD1]);
    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD2]);
    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD3]);
    return true;
}

static bool test_degraded_state_keeps_green_on_and_blinks_red(void)
{
    runtime_led_t runtime_led;
    uint32_t tick_count;

    reset_led_states();
    runtime_led_init(&runtime_led);
    runtime_led_set_state(&runtime_led, RUNTIME_LED_STATE_DEGRADED);

    TEST_ASSERT_TRUE(led_states[BOARD_NUCLEO_F767ZI_LED_LD1]);
    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD2]);
    TEST_ASSERT_TRUE(led_states[BOARD_NUCLEO_F767ZI_LED_LD3]);

    for (tick_count = 0U; tick_count < 100U; tick_count++)
    {
        runtime_led_tick(&runtime_led);
    }

    TEST_ASSERT_TRUE(led_states[BOARD_NUCLEO_F767ZI_LED_LD1]);
    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD3]);
    return true;
}

static bool test_fault_latched_state_keeps_red_on(void)
{
    runtime_led_t runtime_led;
    uint32_t tick_count;

    reset_led_states();
    runtime_led_init(&runtime_led);
    runtime_led_set_state(&runtime_led, RUNTIME_LED_STATE_FAULT_LATCHED);

    for (tick_count = 0U; tick_count < 200U; tick_count++)
    {
        runtime_led_tick(&runtime_led);
    }

    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD1]);
    TEST_ASSERT_FALSE(led_states[BOARD_NUCLEO_F767ZI_LED_LD2]);
    TEST_ASSERT_TRUE(led_states[BOARD_NUCLEO_F767ZI_LED_LD3]);
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "runtime_led_init_turns_all_leds_off", test_runtime_led_init_turns_all_leds_off },
        { "startup_state_blinks_blue_led", test_startup_state_blinks_blue_led },
        { "ready_state_blinks_green_led", test_ready_state_blinks_green_led },
        { "degraded_state_keeps_green_on_and_blinks_red", test_degraded_state_keeps_green_on_and_blinks_red },
        { "fault_latched_state_keeps_red_on", test_fault_latched_state_keeps_red_on },
    };
    unsigned int index;
    unsigned int failed_count = 0U;

    for (index = 0U; index < (unsigned int)(sizeof(tests) / sizeof(tests[0])); index++)
    {
        if (!tests[index].function())
        {
            printf("FAIL %s\n", tests[index].name);
            failed_count++;
        }
        else
        {
            printf("PASS %s\n", tests[index].name);
        }
    }

    if (failed_count > 0U)
    {
        printf("%u test(s) failed.\n", failed_count);
        return 1;
    }

    printf("All %u runtime LED unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}
