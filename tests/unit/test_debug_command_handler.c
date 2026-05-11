#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "debug_command_handler.h"
#include "pca9685_fake.h"

#define DEBUG_COMMAND_HANDLER_TEST_OUTPUT_CAPACITY 1024U
#define DEBUG_COMMAND_HANDLER_TEST_PI_F 3.14159265358979323846f
#define DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(angle_deg) ((angle_deg) * (DEBUG_COMMAND_HANDLER_TEST_PI_F / 180.0f))
#define DEBUG_COMMAND_HANDLER_TEST_FLOAT_TOLERANCE 0.0001f
#define DEBUG_COMMAND_HANDLER_TEST_ELBOW_HOME_RAD DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(102.857142857f)
#define DEBUG_COMMAND_HANDLER_TEST_ELBOW_POSE_RAD DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(120.0f)
#define DEBUG_COMMAND_HANDLER_TEST_WRIST_TILT_HOME_RAD DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(90.0f)
#define DEBUG_COMMAND_HANDLER_TEST_WRIST_TILT_POSE_RAD DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(135.0f)
#define DEBUG_COMMAND_HANDLER_TEST_WRIST_ROTATE_HOME_RAD DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(90.0f)
#define DEBUG_COMMAND_HANDLER_TEST_WRIST_ROTATE_POSE_RAD DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(150.0f)

typedef struct
{
    char output[DEBUG_COMMAND_HANDLER_TEST_OUTPUT_CAPACITY];
    uint16_t output_length;
} debug_command_handler_test_output_t;

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
        if (!float_is_close((actual), (expected), DEBUG_COMMAND_HANDLER_TEST_FLOAT_TOLERANCE)) \
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

static void append_text(debug_command_handler_test_output_t *output, const char *text)
{
    uint16_t index = 0U;

    while ((text[index] != '\0') && (output->output_length < (DEBUG_COMMAND_HANDLER_TEST_OUTPUT_CAPACITY - 1U)))
    {
        output->output[output->output_length] = text[index];
        output->output_length++;
        index++;
    }

    output->output[output->output_length] = '\0';
}

static void append_byte(debug_command_handler_test_output_t *output, uint8_t byte)
{
    if (output->output_length >= (DEBUG_COMMAND_HANDLER_TEST_OUTPUT_CAPACITY - 1U))
    {
        return;
    }

    output->output[output->output_length] = (char)byte;
    output->output_length++;
    output->output[output->output_length] = '\0';
}

static void handler_write_string(void *context, const char *text)
{
    append_text((debug_command_handler_test_output_t *)context, text);
}

static void handler_write_byte(void *context, uint8_t byte)
{
    append_byte((debug_command_handler_test_output_t *)context, byte);
}

static void handler_write_prompt(void *context)
{
    append_text((debug_command_handler_test_output_t *)context, "> ");
}

static const debug_command_handler_io_t test_handler_io = {
    .write_string = handler_write_string,
    .write_byte = handler_write_byte,
    .write_prompt = handler_write_prompt,
};

typedef struct
{
    bool was_called;
} debug_command_handler_recovery_test_context_t;

typedef struct
{
    bool was_called;
    uint32_t last_delay_ms;
} debug_command_handler_delay_test_context_t;

static void reset_output(debug_command_handler_test_output_t *output)
{
    output->output[0] = '\0';
    output->output_length = 0U;
}

static void fill_test_pose(robot_arm_pose_t *pose)
{
    pose->base_rad = DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(10.0f);
    pose->shoulder_rad = DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(30.0f);
    pose->elbow_rad = DEBUG_COMMAND_HANDLER_TEST_ELBOW_POSE_RAD;
    pose->wrist_tilt_rad = DEBUG_COMMAND_HANDLER_TEST_WRIST_TILT_POSE_RAD;
    pose->wrist_rotate_rad = DEBUG_COMMAND_HANDLER_TEST_WRIST_ROTATE_POSE_RAD;
    pose->gripper_rad = DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(10.0f);
}

static bool recover_robot_for_test(void *context, robot_arm_t *robot)
{
    debug_command_handler_recovery_test_context_t *recovery_context = (debug_command_handler_recovery_test_context_t *)context;

    (void)robot;

    if (recovery_context == 0)
    {
        return false;
    }

    recovery_context->was_called = true;
    pca9685_fake_state()->next_status = PCA9685_OK;
    return true;
}

static void delay_ms_for_test(void *context, uint32_t delay_ms)
{
    debug_command_handler_delay_test_context_t *delay_context = (debug_command_handler_delay_test_context_t *)context;

    if (delay_context == 0)
    {
        return;
    }

    delay_context->was_called = true;
    delay_context->last_delay_ms = delay_ms;
}

static bool test_empty_command_writes_prompt_only(void)
{
    debug_command_handler_test_output_t output;

    reset_output(&output);
    debug_command_handler_execute("", 0, &test_handler_io, &output);

    TEST_ASSERT_STRING_EQUAL("> ", output.output);
    return true;
}

static bool test_help_command_writes_help_and_prompt(void)
{
    debug_command_handler_test_output_t output;

    reset_output(&output);
    debug_command_handler_execute("HELP", 0, &test_handler_io, &output);

    TEST_ASSERT_STRING_EQUAL(
        "Commands:\r\n"
        "HELP\r\n"
        "HOME\r\n"
        "POSE <base_deg> <shoulder_deg> <elbow_deg> <wrist_tilt_deg> <wrist_rotate_deg> <gripper_deg>\r\n"
        "POSE_DELAY <delay_s> <base_deg> <shoulder_deg> <elbow_deg> <wrist_tilt_deg> <wrist_rotate_deg> <gripper_deg>\r\n"
        "STATUS\r\n"
        "> ",
        output.output);
    return true;
}

static bool test_status_requires_ready_robot(void)
{
    debug_command_handler_test_output_t output;
    debug_command_handler_context_t context = { 0 };

    reset_output(&output);
    debug_command_handler_execute("STATUS", &context, &test_handler_io, &output);

    TEST_ASSERT_STRING_EQUAL("ERR CONTROLLER_NOT_READY\r\n> ", output.output);
    return true;
}

static bool test_status_reports_current_pose(void)
{
    debug_command_handler_test_output_t output;
    debug_command_handler_context_t context = { 0 };
    robot_arm_t robot;
    robot_arm_pose_t pose;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_test_pose(&pose);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));

    context.robot_ready = true;
    context.robot = &robot;
    reset_output(&output);
    debug_command_handler_execute("STATUS", &context, &test_handler_io, &output);

    TEST_ASSERT_STRING_EQUAL(
        "STATUS\r\n"
        "base=10 deg\r\n"
        "shoulder=30 deg\r\n"
        "elbow=120 deg\r\n"
        "wrist_tilt=135 deg\r\n"
        "wrist_rotate=150 deg\r\n"
        "gripper=10 deg\r\n"
        "> ",
        output.output);
    return true;
}

static bool test_pose_command_updates_robot_and_reports_ok(void)
{
    debug_command_handler_test_output_t output;
    debug_command_handler_context_t context = { 0 };
    robot_arm_t robot;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    context.robot_ready = true;
    context.robot = &robot;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    reset_output(&output);
    debug_command_handler_execute("POSE 10 30 120 135 150 10", &context, &test_handler_io, &output);

    TEST_ASSERT_STRING_EQUAL("OK POSE\r\n> ", output.output);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(10.0f), current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(30.0f), current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_ELBOW_POSE_RAD, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_WRIST_TILT_POSE_RAD, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_WRIST_ROTATE_POSE_RAD, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(10.0f), current_pose.gripper_rad);
    return true;
}

static bool test_pose_command_recovers_from_single_servo_failure(void)
{
    debug_command_handler_test_output_t output;
    debug_command_handler_context_t context = { 0 };
    debug_command_handler_recovery_test_context_t recovery_context = { false };
    robot_arm_t robot;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    context.robot_ready = true;
    context.robot = &robot;
    context.recover_robot = recover_robot_for_test;
    context.recover_context = &recovery_context;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    fake_state->next_status = PCA9685_ERR_I2C;
    reset_output(&output);
    debug_command_handler_execute("POSE 10 30 120 135 150 10", &context, &test_handler_io, &output);

    TEST_ASSERT_TRUE(recovery_context.was_called);
    TEST_ASSERT_STRING_EQUAL("OK POSE\r\n> ", output.output);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)(ROBOT_ARM_JOINT_COUNT + 1U), fake_state->call_count);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(10.0f), current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(30.0f), current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_ELBOW_POSE_RAD, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_WRIST_TILT_POSE_RAD, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_WRIST_ROTATE_POSE_RAD, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(10.0f), current_pose.gripper_rad);
    return true;
}

static bool test_pose_delay_command_waits_then_updates_robot_and_reports_ok(void)
{
    debug_command_handler_test_output_t output;
    debug_command_handler_context_t context = { 0 };
    debug_command_handler_delay_test_context_t delay_context = { false, 0U };
    robot_arm_t robot;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    context.robot_ready = true;
    context.robot = &robot;
    context.delay_ms = delay_ms_for_test;
    context.delay_context = &delay_context;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    reset_output(&output);
    debug_command_handler_execute("POSE_DELAY 3 10 30 120 135 150 10", &context, &test_handler_io, &output);

    TEST_ASSERT_TRUE(delay_context.was_called);
    TEST_ASSERT_UINT32_EQUAL(3000U, delay_context.last_delay_ms);
    TEST_ASSERT_STRING_EQUAL("WAIT POSE 3 s\r\nOK POSE_DELAY\r\n> ", output.output);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(10.0f), current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(30.0f), current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_ELBOW_POSE_RAD, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_WRIST_TILT_POSE_RAD, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_WRIST_ROTATE_POSE_RAD, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(10.0f), current_pose.gripper_rad);
    return true;
}

static bool test_home_command_resets_pose_and_reports_ok(void)
{
    debug_command_handler_test_output_t output;
    debug_command_handler_context_t context = { 0 };
    robot_arm_t robot;
    robot_arm_pose_t current_pose;
    robot_arm_pose_t pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_test_pose(&pose);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));

    context.robot_ready = true;
    context.robot = &robot;
    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    reset_output(&output);
    debug_command_handler_execute("HOME", &context, &test_handler_io, &output);

    TEST_ASSERT_STRING_EQUAL("OK HOME\r\n> ", output.output);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_ELBOW_HOME_RAD, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_WRIST_TILT_HOME_RAD, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_WRIST_ROTATE_HOME_RAD, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(DEBUG_COMMAND_HANDLER_TEST_DEG_TO_RAD(20.0f), current_pose.gripper_rad);
    return true;
}

static bool test_unknown_command_reports_error(void)
{
    debug_command_handler_test_output_t output;

    reset_output(&output);
    debug_command_handler_execute("WAVE", 0, &test_handler_io, &output);

    TEST_ASSERT_STRING_EQUAL("ERR UNKNOWN_COMMAND\r\n> ", output.output);
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "empty_command_writes_prompt_only", test_empty_command_writes_prompt_only },
        { "help_command_writes_help_and_prompt", test_help_command_writes_help_and_prompt },
        { "status_requires_ready_robot", test_status_requires_ready_robot },
        { "status_reports_current_pose", test_status_reports_current_pose },
        { "pose_command_updates_robot_and_reports_ok", test_pose_command_updates_robot_and_reports_ok },
        { "pose_command_recovers_from_single_servo_failure", test_pose_command_recovers_from_single_servo_failure },
        { "pose_delay_command_waits_then_updates_robot_and_reports_ok", test_pose_delay_command_waits_then_updates_robot_and_reports_ok },
        { "home_command_resets_pose_and_reports_ok", test_home_command_resets_pose_and_reports_ok },
        { "unknown_command_reports_error", test_unknown_command_reports_error },
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

    printf("All %u debug command handler unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}