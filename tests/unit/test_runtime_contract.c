#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "runtime_contract.h"
#include "runtime_log.h"

static runtime_log_level_t captured_level = RUNTIME_LOG_LEVEL_DEBUG;
static bool stub_begin_line_result = true;
static bool begin_line_called = false;
static bool end_line_called = false;
static char captured_message[256];

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

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %d, got %d\n", __FILE__, __LINE__, (expected), (actual)); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    do \
    { \
        if (strcmp((expected), (actual)) != 0) \
        { \
            printf("Assertion failed at %s:%d:\nexpected: %s\nactual:   %s\n", __FILE__, __LINE__, (expected), (actual)); \
            return false; \
        } \
    } while (0)

bool runtime_log_begin_line(runtime_log_level_t level)
{
    begin_line_called = true;
    captured_level = level;
    return stub_begin_line_result;
}

void runtime_log_write_raw(const char *message)
{
    size_t current_length;
    size_t remaining_space;

    if (message == 0)
    {
        return;
    }

    current_length = strlen(captured_message);
    remaining_space = sizeof(captured_message) - current_length - 1U;
    strncat(captured_message, message, remaining_space);
}

void runtime_log_end_line(void)
{
    end_line_called = true;
}

static void reset_stubs(void)
{
    captured_level = RUNTIME_LOG_LEVEL_DEBUG;
    stub_begin_line_result = true;
    begin_line_called = false;
    end_line_called = false;
    captured_message[0] = '\0';
}

static bool test_logs_current_runtime_contract_when_info_line_is_enabled(void)
{
    reset_stubs();

    runtime_contract_log_current_baseline();

    TEST_ASSERT_TRUE(begin_line_called);
    TEST_ASSERT_EQUAL_INT(RUNTIME_LOG_LEVEL_INFO, captured_level);
    TEST_ASSERT_EQUAL_STRING(
        "Runtime contract: control_tick=1000 Hz, periodic_service=1 ms, motion_update=direct-pose baseline, POSE_DELAY acknowledges before waiting.",
        captured_message);
    TEST_ASSERT_TRUE(end_line_called);
    return true;
}

static bool test_skips_output_when_info_line_is_filtered_out(void)
{
    reset_stubs();
    stub_begin_line_result = false;

    runtime_contract_log_current_baseline();

    TEST_ASSERT_TRUE(begin_line_called);
    TEST_ASSERT_EQUAL_INT(RUNTIME_LOG_LEVEL_INFO, captured_level);
    TEST_ASSERT_EQUAL_STRING("", captured_message);
    TEST_ASSERT_FALSE(end_line_called);
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "logs_current_runtime_contract_when_info_line_is_enabled", test_logs_current_runtime_contract_when_info_line_is_enabled },
        { "skips_output_when_info_line_is_filtered_out", test_skips_output_when_info_line_is_filtered_out },
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

    printf("All %u runtime contract unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}