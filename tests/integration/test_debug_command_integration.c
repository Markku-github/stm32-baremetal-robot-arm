#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "debug_command_handler.h"
#include "debug_command_shell.h"
#include "pca9685_fake.h"

#define DEBUG_COMMAND_INTEGRATION_OUTPUT_CAPACITY 2048U
#define DEBUG_COMMAND_INTEGRATION_PI_F 3.14159265358979323846f
#define DEBUG_COMMAND_INTEGRATION_DEG_TO_RAD(angle_deg) ((angle_deg) * (DEBUG_COMMAND_INTEGRATION_PI_F / 180.0f))
#define DEBUG_COMMAND_INTEGRATION_FLOAT_TOLERANCE 0.0001f

typedef struct
{
    char output[DEBUG_COMMAND_INTEGRATION_OUTPUT_CAPACITY];
    uint16_t output_length;
} debug_command_integration_output_t;

typedef struct
{
    debug_command_integration_output_t output;
    debug_command_handler_context_t handler_context;
    uint32_t executed_command_count;
} debug_command_integration_context_t;

static const pca9685_device_t test_device = {
    .instance = BSP_I2C_INSTANCE_I2C1,
    .address = PCA9685_I2C_ADDRESS_DEFAULT,
    .oscillator_frequency_hz = PCA9685_OSCILLATOR_FREQUENCY_HZ,
    .pwm_frequency_hz = 50U,
};

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

#define TEST_ASSERT_INT_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %d, got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
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

#define TEST_ASSERT_FLOAT_CLOSE(expected, actual) \
    do \
    { \
        if (!float_is_close((actual), (expected), DEBUG_COMMAND_INTEGRATION_FLOAT_TOLERANCE)) \
        { \
            printf("Assertion failed at %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, (double)(expected), (double)(actual)); \
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

static void append_text(debug_command_integration_output_t *output, const char *text)
{
    uint16_t index = 0U;

    while ((text[index] != '\0') && (output->output_length < (DEBUG_COMMAND_INTEGRATION_OUTPUT_CAPACITY - 1U)))
    {
        output->output[output->output_length] = text[index];
        output->output_length++;
        index++;
    }

    output->output[output->output_length] = '\0';
}

static void append_byte(debug_command_integration_output_t *output, uint8_t byte)
{
    if (output->output_length >= (DEBUG_COMMAND_INTEGRATION_OUTPUT_CAPACITY - 1U))
    {
        return;
    }

    output->output[output->output_length] = (char)byte;
    output->output_length++;
    output->output[output->output_length] = '\0';
}

static void test_write_string(void *context, const char *text)
{
    debug_command_integration_context_t *test_context = (debug_command_integration_context_t *)context;
    append_text(&test_context->output, text);
}

static void test_write_byte(void *context, uint8_t byte)
{
    debug_command_integration_context_t *test_context = (debug_command_integration_context_t *)context;
    append_byte(&test_context->output, byte);
}

static void test_write_prompt(void *context)
{
    debug_command_integration_context_t *test_context = (debug_command_integration_context_t *)context;
    append_text(&test_context->output, "> ");
}

static const debug_command_handler_io_t test_handler_io = {
    .write_string = test_write_string,
    .write_byte = test_write_byte,
    .write_prompt = test_write_prompt,
};

static void shell_execute_command(void *context, const char *command_line)
{
    debug_command_integration_context_t *test_context = (debug_command_integration_context_t *)context;

    test_context->executed_command_count++;
    debug_command_handler_execute(command_line, &test_context->handler_context, &test_handler_io, test_context);
}

static const debug_command_shell_io_t test_shell_io = {
    .write_string = test_write_string,
    .write_byte = test_write_byte,
    .write_prompt = test_write_prompt,
    .execute_command = shell_execute_command,
};

static void reset_test_context(debug_command_integration_context_t *context)
{
    context->output.output[0] = '\0';
    context->output.output_length = 0U;
    context->executed_command_count = 0U;
}

static void feed_text(
    debug_command_shell_t *shell,
    debug_command_integration_context_t *context,
    const char *text)
{
    uint16_t index = 0U;

    while (text[index] != '\0')
    {
        debug_command_shell_process_byte(shell, (uint8_t)text[index], &test_shell_io, context);
        index++;
    }
}

static bool test_help_command_runs_through_shell_and_handler(void)
{
    debug_command_shell_t shell;
    debug_command_integration_context_t context;

    debug_command_shell_init(&shell);
    reset_test_context(&context);
    context.handler_context.robot_ready = false;
    context.handler_context.robot = 0;

    feed_text(&shell, &context, "HELP\r");

    TEST_ASSERT_UINT32_EQUAL(1U, context.executed_command_count);
    TEST_ASSERT_STRING_EQUAL(
        "HELP\r\n"
        "Commands:\r\n"
        "HELP\r\n"
        "HOME\r\n"
        "POSE <base_deg> <shoulder_deg> <elbow_deg> <wrist_tilt_deg> <wrist_rotate_deg> <gripper_deg>\r\n"
        "STATUS\r\n"
        "> ",
        context.output.output);
    return true;
}

static bool test_pose_and_status_run_through_shell_handler_and_robot(void)
{
    debug_command_shell_t shell;
    debug_command_integration_context_t context;
    robot_arm_t robot;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));

    debug_command_shell_init(&shell);
    reset_test_context(&context);
    context.handler_context.robot_ready = true;
    context.handler_context.robot = &robot;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    feed_text(&shell, &context, "POSE 10 -10 20 -15 30 10\rSTATUS\r");

    TEST_ASSERT_UINT32_EQUAL(2U, context.executed_command_count);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_INTEGRATION_DEG_TO_RAD(10.0f), current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_INTEGRATION_DEG_TO_RAD(-10.0f), current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_INTEGRATION_DEG_TO_RAD(20.0f), current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_INTEGRATION_DEG_TO_RAD(-15.0f), current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_INTEGRATION_DEG_TO_RAD(30.0f), current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_INTEGRATION_DEG_TO_RAD(10.0f), current_pose.gripper_rad);
    TEST_ASSERT_STRING_EQUAL(
        "POSE 10 -10 20 -15 30 10\r\n"
        "OK POSE\r\n"
        "> "
        "STATUS\r\n"
        "STATUS\r\n"
        "base=10 deg\r\n"
        "shoulder=-10 deg\r\n"
        "elbow=20 deg\r\n"
        "wrist_tilt=-15 deg\r\n"
        "wrist_rotate=30 deg\r\n"
        "gripper=10 deg\r\n"
        "> ",
        context.output.output);
    return true;
}

int main(void)
{
    uint32_t failed_count = 0U;

    if (!test_help_command_runs_through_shell_and_handler())
    {
        failed_count++;
    }

    if (!test_pose_and_status_run_through_shell_handler_and_robot())
    {
        failed_count++;
    }

    if (failed_count > 0U)
    {
        printf("%u test(s) failed.\n", failed_count);
        return 1;
    }

    printf("All debug command integration tests passed.\n");
    return 0;
}