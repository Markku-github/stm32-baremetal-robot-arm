#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "debug_command_shell.h"

#define TEST_OUTPUT_CAPACITY 512U

typedef struct
{
    char output[TEST_OUTPUT_CAPACITY];
    uint16_t output_length;
    char last_command[DEBUG_COMMAND_SHELL_COMMAND_CAPACITY];
    uint32_t command_count;
} debug_command_shell_test_context_t;

#define TEST_ASSERT_TRUE(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            printf("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
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

static bool strings_are_equal(const char *left, const char *right)
{
    uint16_t index = 0U;

    while ((left[index] != '\0') && (right[index] != '\0'))
    {
        if (left[index] != right[index])
        {
            return false;
        }

        index++;
    }

    return left[index] == right[index];
}

#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    do \
    { \
        if (!strings_are_equal((expected), (actual))) \
        { \
            printf("Assertion failed at %s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, (expected), (actual)); \
            return false; \
        } \
    } while (0)

static void append_text(debug_command_shell_test_context_t *context, const char *text)
{
    uint16_t index = 0U;

    while ((text[index] != '\0') && (context->output_length < (TEST_OUTPUT_CAPACITY - 1U)))
    {
        context->output[context->output_length] = text[index];
        context->output_length++;
        index++;
    }

    context->output[context->output_length] = '\0';
}

static void append_byte(debug_command_shell_test_context_t *context, uint8_t byte)
{
    if (context->output_length >= (TEST_OUTPUT_CAPACITY - 1U))
    {
        return;
    }

    context->output[context->output_length] = (char)byte;
    context->output_length++;
    context->output[context->output_length] = '\0';
}

static void shell_write_string(void *context, const char *text)
{
    append_text((debug_command_shell_test_context_t *)context, text);
}

static void shell_write_byte(void *context, uint8_t byte)
{
    append_byte((debug_command_shell_test_context_t *)context, byte);
}

static void shell_write_prompt(void *context)
{
    append_text((debug_command_shell_test_context_t *)context, "> ");
}

static void shell_execute_command(void *context, const char *command_line)
{
    debug_command_shell_test_context_t *test_context = (debug_command_shell_test_context_t *)context;
    uint16_t index = 0U;

    while ((command_line[index] != '\0') && (index < (DEBUG_COMMAND_SHELL_COMMAND_CAPACITY - 1U)))
    {
        test_context->last_command[index] = command_line[index];
        index++;
    }

    test_context->last_command[index] = '\0';
    test_context->command_count++;
}

static const debug_command_shell_io_t test_shell_io = {
    .write_string = shell_write_string,
    .write_byte = shell_write_byte,
    .write_prompt = shell_write_prompt,
    .execute_command = shell_execute_command,
};

static void reset_test_context(debug_command_shell_test_context_t *context)
{
    context->output[0] = '\0';
    context->output_length = 0U;
    context->last_command[0] = '\0';
    context->command_count = 0U;
}

static void feed_text(
    debug_command_shell_t *shell,
    debug_command_shell_test_context_t *context,
    const char *text)
{
    uint16_t index = 0U;

    while (text[index] != '\0')
    {
        debug_command_shell_process_byte(shell, (uint8_t)text[index], &test_shell_io, context);
        index++;
    }
}

static bool test_carriage_return_executes_trimmed_command(void)
{
    debug_command_shell_t shell;
    debug_command_shell_test_context_t context;

    debug_command_shell_init(&shell);
    reset_test_context(&context);
    feed_text(&shell, &context, "  HELP  \r");

    TEST_ASSERT_STRING_EQUAL("  HELP  \r\n", context.output);
    TEST_ASSERT_INT_EQUAL(1, context.command_count);
    TEST_ASSERT_STRING_EQUAL("HELP", context.last_command);
    return true;
}

static bool test_crlf_executes_command_only_once(void)
{
    debug_command_shell_t shell;
    debug_command_shell_test_context_t context;

    debug_command_shell_init(&shell);
    reset_test_context(&context);
    feed_text(&shell, &context, "STATUS\r\n");

    TEST_ASSERT_STRING_EQUAL("STATUS\r\n", context.output);
    TEST_ASSERT_INT_EQUAL(1, context.command_count);
    TEST_ASSERT_STRING_EQUAL("STATUS", context.last_command);
    return true;
}

static bool test_backspace_updates_buffer_before_submission(void)
{
    debug_command_shell_t shell;
    debug_command_shell_test_context_t context;

    debug_command_shell_init(&shell);
    reset_test_context(&context);
    feed_text(&shell, &context, "HELPX\b\r");

    TEST_ASSERT_STRING_EQUAL("HELPX\b \b\r\n", context.output);
    TEST_ASSERT_INT_EQUAL(1, context.command_count);
    TEST_ASSERT_STRING_EQUAL("HELP", context.last_command);
    return true;
}

static bool test_overflow_reports_error_and_discards_line(void)
{
    debug_command_shell_t shell;
    debug_command_shell_test_context_t context;
    uint16_t index;

    debug_command_shell_init(&shell);
    reset_test_context(&context);

    for (index = 0U; index < DEBUG_COMMAND_SHELL_COMMAND_CAPACITY; index++)
    {
        debug_command_shell_process_byte(&shell, (uint8_t)'A', &test_shell_io, &context);
    }

    debug_command_shell_process_byte(&shell, (uint8_t)'\r', &test_shell_io, &context);

    TEST_ASSERT_INT_EQUAL(0, context.command_count);
    TEST_ASSERT_STRING_EQUAL("", context.last_command);
    TEST_ASSERT_STRING_EQUAL(
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\nERR COMMAND_TOO_LONG\r\n> ",
        context.output);
    return true;
}

static bool test_transport_overflow_resets_partial_command(void)
{
    debug_command_shell_t shell;
    debug_command_shell_test_context_t context;

    debug_command_shell_init(&shell);
    reset_test_context(&context);

    feed_text(&shell, &context, "HEL");
    debug_command_shell_handle_transport_overflow(&shell, &test_shell_io, &context);
    feed_text(&shell, &context, "OK\r");

    TEST_ASSERT_STRING_EQUAL("HEL\r\n[RX overflow]\r\n> OK\r\n", context.output);
    TEST_ASSERT_INT_EQUAL(1, context.command_count);
    TEST_ASSERT_STRING_EQUAL("OK", context.last_command);
    return true;
}

static bool test_transport_read_error_resets_partial_command(void)
{
    debug_command_shell_t shell;
    debug_command_shell_test_context_t context;

    debug_command_shell_init(&shell);
    reset_test_context(&context);

    feed_text(&shell, &context, "PO");
    debug_command_shell_handle_transport_read_error(&shell, &test_shell_io, &context);
    feed_text(&shell, &context, "SE\r");

    TEST_ASSERT_STRING_EQUAL("PO\r\n[RX read error]\r\n> SE\r\n", context.output);
    TEST_ASSERT_INT_EQUAL(1, context.command_count);
    TEST_ASSERT_STRING_EQUAL("SE", context.last_command);
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "carriage_return_executes_trimmed_command", test_carriage_return_executes_trimmed_command },
        { "crlf_executes_command_only_once", test_crlf_executes_command_only_once },
        { "backspace_updates_buffer_before_submission", test_backspace_updates_buffer_before_submission },
        { "overflow_reports_error_and_discards_line", test_overflow_reports_error_and_discards_line },
        { "transport_overflow_resets_partial_command", test_transport_overflow_resets_partial_command },
        { "transport_read_error_resets_partial_command", test_transport_read_error_resets_partial_command },
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

    printf("All %u debug command shell unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}