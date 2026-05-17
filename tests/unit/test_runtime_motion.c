#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pca9685_fake.h"
#include "runtime_log.h"
#include "runtime_motion.h"

#define RUNTIME_MOTION_TEST_PI_F 3.14159265358979323846f
#define RUNTIME_MOTION_TEST_DEG_TO_RAD(angle_deg) ((angle_deg) * (RUNTIME_MOTION_TEST_PI_F / 180.0f))
#define RUNTIME_MOTION_TEST_FLOAT_TOLERANCE 0.0001f

static const pca9685_device_t test_device = {
    .instance = BSP_I2C_INSTANCE_I2C1,
    .address = PCA9685_I2C_ADDRESS_DEFAULT,
    .oscillator_frequency_hz = PCA9685_OSCILLATOR_FREQUENCY_HZ,
    .pwm_frequency_hz = 50U,
};

static runtime_log_level_t last_log_level = RUNTIME_LOG_LEVEL_INFO;
static const char *last_log_message = 0;

typedef struct
{
    bool was_called;
} runtime_motion_recovery_test_context_t;

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
        if (!float_is_close((actual), (expected), RUNTIME_MOTION_TEST_FLOAT_TOLERANCE)) \
        { \
            printf("Assertion failed at %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, (double)(expected), (double)(actual)); \
            return false; \
        } \
    } while (0)

void runtime_log_write_line(runtime_log_level_t level, const char *message)
{
    last_log_level = level;
    last_log_message = message;
}

static void fill_test_pose(robot_arm_pose_t *pose)
{
    pose->base_rad = RUNTIME_MOTION_TEST_DEG_TO_RAD(10.0f);
    pose->shoulder_rad = RUNTIME_MOTION_TEST_DEG_TO_RAD(30.0f);
    pose->elbow_rad = RUNTIME_MOTION_TEST_DEG_TO_RAD(120.0f);
    pose->wrist_tilt_rad = RUNTIME_MOTION_TEST_DEG_TO_RAD(135.0f);
    pose->wrist_rotate_rad = RUNTIME_MOTION_TEST_DEG_TO_RAD(150.0f);
    pose->gripper_rad = RUNTIME_MOTION_TEST_DEG_TO_RAD(10.0f);
}

static void reset_log_capture(void)
{
    last_log_level = RUNTIME_LOG_LEVEL_INFO;
    last_log_message = 0;
}

static bool recover_robot_for_test(void *context, robot_arm_t *robot)
{
    runtime_motion_recovery_test_context_t *recovery_context = (runtime_motion_recovery_test_context_t *)context;

    (void)robot;

    if (recovery_context == 0)
    {
        return false;
    }

    recovery_context->was_called = true;
    pca9685_fake_state()->next_status = PCA9685_OK;
    return true;
}

static bool test_schedule_pose_defers_application_until_service(void)
{
    runtime_motion_t motion;
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    runtime_motion_init(&motion);
    runtime_motion_configure(&motion, &robot, 0, 0);
    fill_test_pose(&pose);

    TEST_ASSERT_TRUE(runtime_motion_schedule_pose(&motion, &pose));
    TEST_ASSERT_TRUE(runtime_motion_has_pending_request(&motion));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.base_rad);

    TEST_ASSERT_TRUE(runtime_motion_service(&motion));
    TEST_ASSERT_FALSE(runtime_motion_has_pending_request(&motion));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(pose.base_rad, current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.shoulder_rad, current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.elbow_rad, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.wrist_tilt_rad, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.wrist_rotate_rad, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.gripper_rad, current_pose.gripper_rad);
    return true;
}

static bool test_schedule_home_defers_application_until_service(void)
{
    runtime_motion_t motion;
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;
    robot_arm_pose_t home_pose;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_test_pose(&pose);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));

    runtime_motion_init(&motion);
    runtime_motion_configure(&motion, &robot, 0, 0);
    TEST_ASSERT_TRUE(runtime_motion_schedule_home(&motion));
    TEST_ASSERT_TRUE(runtime_motion_has_pending_request(&motion));

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(pose.base_rad, current_pose.base_rad);

    TEST_ASSERT_TRUE(runtime_motion_service(&motion));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_home_pose(&robot, &home_pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(home_pose.base_rad, current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.shoulder_rad, current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.elbow_rad, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.wrist_tilt_rad, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.wrist_rotate_rad, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.gripper_rad, current_pose.gripper_rad);
    return true;
}

static bool test_schedule_rejects_second_pending_request(void)
{
    runtime_motion_t motion;
    robot_arm_t robot;
    robot_arm_pose_t pose;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    runtime_motion_init(&motion);
    runtime_motion_configure(&motion, &robot, 0, 0);
    fill_test_pose(&pose);

    TEST_ASSERT_TRUE(runtime_motion_schedule_pose(&motion, &pose));
    TEST_ASSERT_FALSE(runtime_motion_schedule_home(&motion));
    return true;
}

static bool test_service_recovers_once_from_single_servo_failure(void)
{
    runtime_motion_t motion;
    runtime_motion_recovery_test_context_t recovery_context = { false };
    robot_arm_t robot;
    robot_arm_pose_t pose;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    runtime_motion_init(&motion);
    runtime_motion_configure(&motion, &robot, recover_robot_for_test, &recovery_context);
    fill_test_pose(&pose);
    pca9685_fake_reset();
    pca9685_fake_state()->next_status = PCA9685_ERR_I2C;
    TEST_ASSERT_TRUE(runtime_motion_schedule_pose(&motion, &pose));

    TEST_ASSERT_TRUE(runtime_motion_service(&motion));
    TEST_ASSERT_TRUE(recovery_context.was_called);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)(ROBOT_ARM_JOINT_COUNT + 1U), pca9685_fake_state()->call_count);
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "schedule_pose_defers_application_until_service", test_schedule_pose_defers_application_until_service },
        { "schedule_home_defers_application_until_service", test_schedule_home_defers_application_until_service },
        { "schedule_rejects_second_pending_request", test_schedule_rejects_second_pending_request },
        { "service_recovers_once_from_single_servo_failure", test_service_recovers_once_from_single_servo_failure },
    };
    unsigned int index;
    unsigned int failed_count = 0U;

    reset_log_capture();

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

    printf("All %u runtime motion unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}