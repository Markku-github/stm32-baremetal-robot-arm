#include <stdbool.h>
#include <stdio.h>

#include "debug_command_parser.h"

#define COMMAND_PARSER_PI_F 3.14159265358979323846f
#define COMMAND_PARSER_DEG_TO_RAD(angle_deg) ((angle_deg) * (COMMAND_PARSER_PI_F / 180.0f))
#define COMMAND_PARSER_FLOAT_TOLERANCE 0.0001f

static bool float_is_close(float actual, float expected, float tolerance)
{
    float delta = actual - expected;

    if (delta < 0.0f)
    {
        delta = -delta;
    }

    return delta <= tolerance;
}

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

#define TEST_ASSERT_INT_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %d, got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    do \
    { \
        const char *expected_string = (expected); \
        const char *actual_string = (actual); \
        unsigned int string_index = 0U; \
        while ((expected_string[string_index] != '\0') && (actual_string[string_index] != '\0') && (expected_string[string_index] == actual_string[string_index])) \
        { \
            string_index++; \
        } \
        if (expected_string[string_index] != actual_string[string_index]) \
        { \
            printf("Assertion failed at %s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, expected_string, actual_string); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_CLOSE(expected, actual) \
    do \
    { \
        if (!float_is_close((actual), (expected), COMMAND_PARSER_FLOAT_TOLERANCE)) \
        { \
            printf("Assertion failed at %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, (double)(expected), (double)(actual)); \
            return false; \
        } \
    } while (0)

static bool test_trim_line_removes_leading_and_trailing_whitespace(void)
{
    char line[] = "  \t pose 1 2 3 4 5 6 \t ";
    const uint8_t trimmed_length = debug_command_parser_trim_line(line, (uint8_t)(sizeof(line) - 1U));

    TEST_ASSERT_INT_EQUAL(16, trimmed_length);
    TEST_ASSERT_STRING_EQUAL("pose 1 2 3 4 5 6", line);
    return true;
}

static bool test_command_match_is_case_insensitive_and_returns_arguments(void)
{
    const char *arguments = 0;

    TEST_ASSERT_TRUE(debug_command_parser_matches_name_with_arguments("pose 10 20 30 40 50 60", "POSE", &arguments));
    TEST_ASSERT_STRING_EQUAL("10 20 30 40 50 60", arguments);
    TEST_ASSERT_TRUE(debug_command_parser_matches_name_with_arguments("HeLp", "HELP", &arguments));
    TEST_ASSERT_STRING_EQUAL("", arguments);
    TEST_ASSERT_FALSE(debug_command_parser_matches_name_with_arguments("POSED 1 2 3", "POSE", &arguments));
    return true;
}

static bool test_parse_signed_int32_token_handles_signs_and_whitespace(void)
{
    const char *cursor = "   -123  +45  0";
    int32_t value = 0;

    TEST_ASSERT_TRUE(debug_command_parser_parse_signed_int32_token(&cursor, &value));
    TEST_ASSERT_INT_EQUAL(-123, value);
    TEST_ASSERT_TRUE(debug_command_parser_parse_signed_int32_token(&cursor, &value));
    TEST_ASSERT_INT_EQUAL(45, value);
    TEST_ASSERT_TRUE(debug_command_parser_parse_signed_int32_token(&cursor, &value));
    TEST_ASSERT_INT_EQUAL(0, value);
    TEST_ASSERT_STRING_EQUAL("", cursor);
    return true;
}

static bool test_parse_signed_int32_token_rejects_invalid_tokens(void)
{
    const char *cursor_alpha = "abc";
    const char *cursor_suffix = "12x";
    const char *cursor_overflow = "2147483648";
    int32_t value = 0;

    TEST_ASSERT_FALSE(debug_command_parser_parse_signed_int32_token(&cursor_alpha, &value));
    TEST_ASSERT_FALSE(debug_command_parser_parse_signed_int32_token(&cursor_suffix, &value));
    TEST_ASSERT_FALSE(debug_command_parser_parse_signed_int32_token(&cursor_overflow, &value));
    TEST_ASSERT_FALSE(debug_command_parser_parse_signed_int32_token(0, &value));
    TEST_ASSERT_FALSE(debug_command_parser_parse_signed_int32_token(&cursor_alpha, 0));
    return true;
}

static bool test_parse_pose_arguments_accepts_complete_pose(void)
{
    robot_arm_pose_t pose;

    TEST_ASSERT_TRUE(debug_command_parser_parse_pose_arguments(" 10 20 -10 5 -15 10 ", &pose));
    TEST_ASSERT_FLOAT_CLOSE(COMMAND_PARSER_DEG_TO_RAD(10.0f), pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(COMMAND_PARSER_DEG_TO_RAD(20.0f), pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(COMMAND_PARSER_DEG_TO_RAD(-10.0f), pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(COMMAND_PARSER_DEG_TO_RAD(5.0f), pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(COMMAND_PARSER_DEG_TO_RAD(-15.0f), pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(COMMAND_PARSER_DEG_TO_RAD(10.0f), pose.gripper_rad);
    return true;
}

static bool test_parse_pose_arguments_rejects_incomplete_or_extra_input(void)
{
    robot_arm_pose_t pose;

    TEST_ASSERT_FALSE(debug_command_parser_parse_pose_arguments("10 20 30", &pose));
    TEST_ASSERT_FALSE(debug_command_parser_parse_pose_arguments("10 20 30 40 50 60 70", &pose));
    TEST_ASSERT_FALSE(debug_command_parser_parse_pose_arguments("10 20 nope 40 50 60", &pose));
    TEST_ASSERT_FALSE(debug_command_parser_parse_pose_arguments(0, &pose));
    TEST_ASSERT_FALSE(debug_command_parser_parse_pose_arguments("10 20 30 40 50 60", 0));
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "trim_line_removes_leading_and_trailing_whitespace", test_trim_line_removes_leading_and_trailing_whitespace },
        { "command_match_is_case_insensitive_and_returns_arguments", test_command_match_is_case_insensitive_and_returns_arguments },
        { "parse_signed_int32_token_handles_signs_and_whitespace", test_parse_signed_int32_token_handles_signs_and_whitespace },
        { "parse_signed_int32_token_rejects_invalid_tokens", test_parse_signed_int32_token_rejects_invalid_tokens },
        { "parse_pose_arguments_accepts_complete_pose", test_parse_pose_arguments_accepts_complete_pose },
        { "parse_pose_arguments_rejects_incomplete_or_extra_input", test_parse_pose_arguments_rejects_incomplete_or_extra_input },
    };
    unsigned int test_index;
    unsigned int failed_count = 0U;

    for (test_index = 0U; test_index < (unsigned int)(sizeof(tests) / sizeof(tests[0])); test_index++)
    {
        if (!tests[test_index].function())
        {
            printf("FAIL %s\n", tests[test_index].name);
            failed_count++;
        }
        else
        {
            printf("PASS %s\n", tests[test_index].name);
        }
    }

    if (failed_count > 0U)
    {
        printf("%u test(s) failed.\n", failed_count);
        return 1;
    }

    printf("All %u debug command parser unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}