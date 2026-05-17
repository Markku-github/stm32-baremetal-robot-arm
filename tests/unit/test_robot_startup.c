#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pca9685_fake.h"
#include "robot_startup.h"

#define ROBOT_STARTUP_TEST_PI_F 3.14159265358979323846f
#define ROBOT_STARTUP_TEST_DEG_TO_RAD(angle_deg) ((angle_deg) * (ROBOT_STARTUP_TEST_PI_F / 180.0f))
#define ROBOT_STARTUP_TEST_FLOAT_TOLERANCE 0.0001f
#define ROBOT_STARTUP_TEST_BASE_HOME_RAD ROBOT_STARTUP_TEST_DEG_TO_RAD(90.0f)
#define ROBOT_STARTUP_TEST_SHOULDER_HOME_RAD 0.0f
#define ROBOT_STARTUP_TEST_ELBOW_HOME_RAD ROBOT_STARTUP_TEST_DEG_TO_RAD(180.0f)
#define ROBOT_STARTUP_TEST_WRIST_TILT_HOME_RAD ROBOT_STARTUP_TEST_DEG_TO_RAD(180.0f)
#define ROBOT_STARTUP_TEST_WRIST_ROTATE_HOME_RAD ROBOT_STARTUP_TEST_DEG_TO_RAD(90.0f)
#define ROBOT_STARTUP_TEST_GRIPPER_HOME_RAD 0.0f

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
        if (!float_is_close((actual), (expected), ROBOT_STARTUP_TEST_FLOAT_TOLERANCE)) \
        { \
            printf("Assertion failed at %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, (double)(expected), (double)(actual)); \
            return false; \
        } \
    } while (0)

static bool test_robot_startup_validates_arguments(void)
{
    robot_arm_t robot;

    TEST_ASSERT_INT_EQUAL(ROBOT_STARTUP_ERR_INVALID_ARGUMENT, robot_startup_initialize(0, &test_device));
    TEST_ASSERT_INT_EQUAL(ROBOT_STARTUP_ERR_INVALID_ARGUMENT, robot_startup_initialize(&robot, 0));
    TEST_ASSERT_INT_EQUAL(ROBOT_STARTUP_ERR_INVALID_ARGUMENT, robot_startup_initialize_and_home(0, &test_device));
    TEST_ASSERT_INT_EQUAL(ROBOT_STARTUP_ERR_INVALID_ARGUMENT, robot_startup_initialize_and_home(&robot, 0));
    return true;
}

static bool test_robot_startup_initializes_robot_without_motion(void)
{
    robot_arm_t robot;
    const servo_t *base_servo;
    pca9685_fake_state_t *fake_state;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_INT_EQUAL(ROBOT_STARTUP_OK, robot_startup_initialize(&robot, &test_device));
    base_servo = robot_arm_get_servo_const(&robot, ROBOT_ARM_JOINT_BASE);
    TEST_ASSERT_TRUE(base_servo != 0);
    TEST_ASSERT_TRUE(base_servo->device == &test_device);
    TEST_ASSERT_UINT32_EQUAL(0U, fake_state->call_count);
    return true;
}

static bool test_robot_startup_initialize_and_home_moves_home(void)
{
    robot_arm_t robot;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_INT_EQUAL(ROBOT_STARTUP_OK, robot_startup_initialize_and_home(&robot, &test_device));
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_TRUE(fake_state->last_device == &test_device);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_STARTUP_TEST_BASE_HOME_RAD, current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_STARTUP_TEST_SHOULDER_HOME_RAD, current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_STARTUP_TEST_ELBOW_HOME_RAD, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_STARTUP_TEST_WRIST_TILT_HOME_RAD, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_STARTUP_TEST_WRIST_ROTATE_HOME_RAD, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_STARTUP_TEST_GRIPPER_HOME_RAD, current_pose.gripper_rad);
    return true;
}

static bool test_robot_startup_reports_home_failure(void)
{
    robot_arm_t robot;
    pca9685_fake_state_t *fake_state;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    fake_state->next_status = PCA9685_ERR_I2C;

    TEST_ASSERT_INT_EQUAL(ROBOT_STARTUP_ERR_HOME, robot_startup_initialize_and_home(&robot, &test_device));
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->call_count);
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "robot_startup_validates_arguments", test_robot_startup_validates_arguments },
        { "robot_startup_initializes_robot_without_motion", test_robot_startup_initializes_robot_without_motion },
        { "robot_startup_initialize_and_home_moves_home", test_robot_startup_initialize_and_home_moves_home },
        { "robot_startup_reports_home_failure", test_robot_startup_reports_home_failure },
    };
    uint32_t index;

    for (index = 0U; index < (sizeof(tests) / sizeof(tests[0])); index++)
    {
        if (!tests[index].function())
        {
            printf("FAIL %s\n", tests[index].name);
            return 1;
        }

        printf("PASS %s\n", tests[index].name);
    }

    printf("All robot_startup tests passed.\n");
    return 0;
}