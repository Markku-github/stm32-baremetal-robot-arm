#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "runtime_tick.h"

uint32_t SystemCoreClock = 16000000U;
void SysTick_Handler(void);

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

#define TEST_ASSERT_UINT32_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %lu, got %lu\n", __FILE__, __LINE__, (unsigned long)(expected), (unsigned long)(actual)); \
            return false; \
        } \
    } while (0)

static bool test_systick_handler_advances_runtime_tick(void)
{
    const uint32_t before = runtime_tick_now_ms();

    SysTick_Handler();

    TEST_ASSERT_UINT32_EQUAL(before + 1U, runtime_tick_now_ms());
    return true;
}

static bool test_periodic_due_reports_ready_after_requested_period(void)
{
    uint32_t last_run_ms = 10U;

    TEST_ASSERT_FALSE(runtime_tick_periodic_due(10U, 5U, &last_run_ms));
    TEST_ASSERT_FALSE(runtime_tick_periodic_due(14U, 5U, &last_run_ms));
    TEST_ASSERT_TRUE(runtime_tick_periodic_due(15U, 5U, &last_run_ms));
    TEST_ASSERT_UINT32_EQUAL(15U, last_run_ms);
    return true;
}

static bool test_deadline_reached_reports_true_at_exact_threshold(void)
{
    TEST_ASSERT_FALSE(runtime_tick_deadline_reached(100U, 5U, 104U));
    TEST_ASSERT_TRUE(runtime_tick_deadline_reached(100U, 5U, 105U));
    return true;
}

static bool test_deadline_reached_handles_wraparound_safely(void)
{
    TEST_ASSERT_FALSE(runtime_tick_deadline_reached(0xFFFFFFFEUL, 4U, 1U));
    TEST_ASSERT_TRUE(runtime_tick_deadline_reached(0xFFFFFFFEUL, 4U, 2U));
    return true;
}

static bool test_deadline_reached_accepts_zero_delay_immediately(void)
{
    TEST_ASSERT_TRUE(runtime_tick_deadline_reached(123U, 0U, 123U));
    TEST_ASSERT_TRUE(runtime_tick_deadline_reached(123U, 0U, 122U));
    return true;
}

static bool test_periodic_due_handles_wraparound_safely(void)
{
    uint32_t last_run_ms = 0xFFFFFFFEUL;

    TEST_ASSERT_TRUE(runtime_tick_periodic_due(1U, 2U, &last_run_ms));
    TEST_ASSERT_UINT32_EQUAL(1U, last_run_ms);
    return true;
}

static bool test_periodic_due_rejects_invalid_arguments(void)
{
    uint32_t last_run_ms = 0U;

    TEST_ASSERT_FALSE(runtime_tick_periodic_due(10U, 0U, &last_run_ms));
    TEST_ASSERT_FALSE(runtime_tick_periodic_due(10U, 1U, 0));
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "systick_handler_advances_runtime_tick", test_systick_handler_advances_runtime_tick },
        { "periodic_due_reports_ready_after_requested_period", test_periodic_due_reports_ready_after_requested_period },
        { "deadline_reached_reports_true_at_exact_threshold", test_deadline_reached_reports_true_at_exact_threshold },
        { "deadline_reached_handles_wraparound_safely", test_deadline_reached_handles_wraparound_safely },
        { "deadline_reached_accepts_zero_delay_immediately", test_deadline_reached_accepts_zero_delay_immediately },
        { "periodic_due_handles_wraparound_safely", test_periodic_due_handles_wraparound_safely },
        { "periodic_due_rejects_invalid_arguments", test_periodic_due_rejects_invalid_arguments },
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

    printf("All %u runtime tick unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}
