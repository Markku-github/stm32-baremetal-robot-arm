#include <stdbool.h>
#include <stdio.h>

#include "pca9685_fake.h"
#include "robot_arm.h"

#define ROBOT_ARM_TEST_PI_F 3.14159265358979323846f
#define ROBOT_ARM_TEST_DEG_TO_RAD(angle_deg) ((angle_deg) * (ROBOT_ARM_TEST_PI_F / 180.0f))
#define ROBOT_ARM_TEST_FLOAT_TOLERANCE 0.0001f
#define ROBOT_ARM_TEST_BASE_MIN_RAD 0.0f
#define ROBOT_ARM_TEST_BASE_MAX_RAD ROBOT_ARM_TEST_DEG_TO_RAD(90.0f)
#define ROBOT_ARM_TEST_SHOULDER_MIN_RAD 0.0f
#define ROBOT_ARM_TEST_SHOULDER_MID_RAD ROBOT_ARM_TEST_DEG_TO_RAD(90.0f)
#define ROBOT_ARM_TEST_SHOULDER_MAX_RAD ROBOT_ARM_TEST_DEG_TO_RAD(180.0f)
#define ROBOT_ARM_TEST_SHOULDER_SAFE_MIN_PULSE_US 1200U
#define ROBOT_ARM_TEST_SHOULDER_SAFE_MID_PULSE_US 2300U
#define ROBOT_ARM_TEST_SHOULDER_SAFE_MAX_PULSE_US 3200U
#define ROBOT_ARM_TEST_ELBOW_MIN_RAD (-ROBOT_ARM_TEST_DEG_TO_RAD(60.0f))
#define ROBOT_ARM_TEST_ELBOW_MAX_RAD ROBOT_ARM_TEST_DEG_TO_RAD(45.0f)
#define ROBOT_ARM_TEST_ELBOW_SAFE_MIN_PULSE_US 900U
#define ROBOT_ARM_TEST_ELBOW_SAFE_MAX_PULSE_US 1800U
#define ROBOT_ARM_TEST_GRIPPER_HOME_RAD ROBOT_ARM_TEST_DEG_TO_RAD(20.0f)
#define ROBOT_ARM_TEST_GRIPPER_MIN_RAD 0.0f
#define ROBOT_ARM_TEST_GRIPPER_MAX_RAD ROBOT_ARM_TEST_DEG_TO_RAD(20.0f)
#define ROBOT_ARM_TEST_GRIPPER_SAFE_CLOSE_PULSE_US 2450U
#define ROBOT_ARM_TEST_GRIPPER_SAFE_OPEN_PULSE_US 1700U

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

#define TEST_ASSERT_UINT8_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %u, got %u\n", __FILE__, __LINE__, (unsigned int)(expected), (unsigned int)(actual)); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_UINT16_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %u, got %u\n", __FILE__, __LINE__, (unsigned int)(expected), (unsigned int)(actual)); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_CLOSE(expected, actual) \
    do \
    { \
        if (!float_is_close((actual), (expected), ROBOT_ARM_TEST_FLOAT_TOLERANCE)) \
        { \
            printf("Assertion failed at %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, (double)(expected), (double)(actual)); \
            return false; \
        } \
    } while (0)

static const pca9685_device_t test_device = {
    .instance = BSP_I2C_INSTANCE_I2C1,
    .address = PCA9685_I2C_ADDRESS_DEFAULT,
    .oscillator_frequency_hz = PCA9685_OSCILLATOR_FREQUENCY_HZ,
    .pwm_frequency_hz = 50U,
};

static void fill_non_home_pose(robot_arm_pose_t *pose)
{
    pose->base_rad = ROBOT_ARM_TEST_DEG_TO_RAD(10.0f);
    pose->shoulder_rad = ROBOT_ARM_TEST_DEG_TO_RAD(30.0f);
    pose->elbow_rad = ROBOT_ARM_TEST_DEG_TO_RAD(20.0f);
    pose->wrist_tilt_rad = ROBOT_ARM_TEST_DEG_TO_RAD(-15.0f);
    pose->wrist_rotate_rad = ROBOT_ARM_TEST_DEG_TO_RAD(30.0f);
    pose->gripper_rad = ROBOT_ARM_TEST_DEG_TO_RAD(10.0f);
}

static bool test_robot_arm_init_configures_joints_and_home_pose(void)
{
    static const uint8_t expected_channels[ROBOT_ARM_JOINT_COUNT] = { 0U, 1U, 2U, 3U, 4U, 5U };
    static const float expected_minimum_angles[ROBOT_ARM_JOINT_COUNT] = {
        ROBOT_ARM_TEST_BASE_MIN_RAD,
        ROBOT_ARM_TEST_SHOULDER_MIN_RAD,
        ROBOT_ARM_TEST_ELBOW_MIN_RAD,
        -ROBOT_ARM_TEST_DEG_TO_RAD(30.0f),
        -ROBOT_ARM_TEST_DEG_TO_RAD(45.0f),
        ROBOT_ARM_TEST_GRIPPER_MIN_RAD,
    };
    static const float expected_maximum_angles[ROBOT_ARM_JOINT_COUNT] = {
        ROBOT_ARM_TEST_BASE_MAX_RAD,
        ROBOT_ARM_TEST_SHOULDER_MAX_RAD,
        ROBOT_ARM_TEST_ELBOW_MAX_RAD,
        ROBOT_ARM_TEST_DEG_TO_RAD(30.0f),
        ROBOT_ARM_TEST_DEG_TO_RAD(45.0f),
        ROBOT_ARM_TEST_GRIPPER_MAX_RAD,
    };
    robot_arm_t robot;
    robot_arm_pose_t home_pose;
    uint8_t joint_index;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_home_pose(&robot, &home_pose));
    TEST_ASSERT_FLOAT_CLOSE(0.0f, home_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, home_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, home_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, home_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, home_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_GRIPPER_HOME_RAD, home_pose.gripper_rad);

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        float home_angle_rad;
        const servo_t *servo = robot_arm_get_servo_const(&robot, (robot_arm_joint_id_t)joint_index);

        TEST_ASSERT_TRUE(servo != 0);
        TEST_ASSERT_TRUE(servo->device == &test_device);
        TEST_ASSERT_UINT8_EQUAL(expected_channels[joint_index], servo->channel);
        TEST_ASSERT_FLOAT_CLOSE(expected_minimum_angles[joint_index], servo->minimum_angle_rad);
        TEST_ASSERT_FLOAT_CLOSE(expected_maximum_angles[joint_index], servo->maximum_angle_rad);
        TEST_ASSERT_FLOAT_CLOSE(0.0f, servo->current_angle_rad);
        TEST_ASSERT_FLOAT_CLOSE(0.0f, servo->target_angle_rad);
        TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_home_angle_rad(&robot, (robot_arm_joint_id_t)joint_index, &home_angle_rad));
        TEST_ASSERT_FLOAT_CLOSE(
            (joint_index == (uint8_t)ROBOT_ARM_JOINT_GRIPPER) ? ROBOT_ARM_TEST_GRIPPER_HOME_RAD : 0.0f,
            home_angle_rad);
    }

    return true;
}

static bool test_robot_arm_set_pose_immediate_updates_all_joint_state(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_non_home_pose(&pose);

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_TRUE(fake_state->last_device == &test_device);
    TEST_ASSERT_UINT8_EQUAL(5U, fake_state->last_channel);
    TEST_ASSERT_UINT16_EQUAL(2075U, fake_state->last_pulse_width_us);

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(pose.base_rad, current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.shoulder_rad, current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.elbow_rad, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.wrist_tilt_rad, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.wrist_rotate_rad, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(pose.gripper_rad, current_pose.gripper_rad);
    return true;
}

static bool test_robot_arm_set_pose_clamps_base_to_safe_limits(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_non_home_pose(&pose);

    pose.base_rad = ROBOT_ARM_TEST_DEG_TO_RAD(-50.0f);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_BASE_MIN_RAD, current_pose.base_rad);

    pose.base_rad = ROBOT_ARM_TEST_DEG_TO_RAD(120.0f);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_BASE_MAX_RAD, current_pose.base_rad);
    return true;
}

static bool test_robot_arm_set_pose_clamps_shoulder_to_safe_limits(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_non_home_pose(&pose);

    pose.shoulder_rad = ROBOT_ARM_TEST_DEG_TO_RAD(-50.0f);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_SHOULDER_MIN_RAD, current_pose.shoulder_rad);

    pose.shoulder_rad = ROBOT_ARM_TEST_DEG_TO_RAD(220.0f);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_SHOULDER_MAX_RAD, current_pose.shoulder_rad);
    return true;
}

static bool test_robot_arm_shoulder_uses_piecewise_pulse_range(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose = { 0 };
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    pose.shoulder_rad = ROBOT_ARM_TEST_SHOULDER_MIN_RAD;
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_UINT16_EQUAL(
        ROBOT_ARM_TEST_SHOULDER_SAFE_MIN_PULSE_US,
        fake_state->last_pulse_width_us_by_channel[ROBOT_ARM_JOINT_SHOULDER]);

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    pose.shoulder_rad = ROBOT_ARM_TEST_SHOULDER_MID_RAD;
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_UINT16_EQUAL(
        ROBOT_ARM_TEST_SHOULDER_SAFE_MID_PULSE_US,
        fake_state->last_pulse_width_us_by_channel[ROBOT_ARM_JOINT_SHOULDER]);

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    pose.shoulder_rad = ROBOT_ARM_TEST_SHOULDER_MAX_RAD;
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_UINT16_EQUAL(
        ROBOT_ARM_TEST_SHOULDER_SAFE_MAX_PULSE_US,
        fake_state->last_pulse_width_us_by_channel[ROBOT_ARM_JOINT_SHOULDER]);
    return true;
}

static bool test_robot_arm_set_pose_clamps_elbow_to_safe_limits(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_non_home_pose(&pose);

    pose.elbow_rad = ROBOT_ARM_TEST_DEG_TO_RAD(-90.0f);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_ELBOW_MIN_RAD, current_pose.elbow_rad);

    pose.elbow_rad = ROBOT_ARM_TEST_DEG_TO_RAD(90.0f);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_ELBOW_MAX_RAD, current_pose.elbow_rad);
    return true;
}

static bool test_robot_arm_elbow_uses_expanded_negative_pulse_range(void)
{
    robot_arm_t robot;
    servo_t *elbow;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    elbow = robot_arm_get_servo(&robot, ROBOT_ARM_JOINT_ELBOW);

    TEST_ASSERT_TRUE(elbow != 0);

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_set_angle_immediate_rad(elbow, ROBOT_ARM_TEST_ELBOW_MIN_RAD));
    TEST_ASSERT_UINT8_EQUAL(2U, fake_state->last_channel);
    TEST_ASSERT_UINT16_EQUAL(ROBOT_ARM_TEST_ELBOW_SAFE_MIN_PULSE_US, fake_state->last_pulse_width_us);

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_set_angle_immediate_rad(elbow, ROBOT_ARM_TEST_ELBOW_MAX_RAD));
    TEST_ASSERT_UINT8_EQUAL(2U, fake_state->last_channel);
    TEST_ASSERT_UINT16_EQUAL(ROBOT_ARM_TEST_ELBOW_SAFE_MAX_PULSE_US, fake_state->last_pulse_width_us);
    return true;
}

static bool test_robot_arm_set_pose_clamps_gripper_to_safe_limits(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_non_home_pose(&pose);

    pose.gripper_rad = ROBOT_ARM_TEST_DEG_TO_RAD(-50.0f);
    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_UINT8_EQUAL(5U, fake_state->last_channel);
    TEST_ASSERT_UINT16_EQUAL(ROBOT_ARM_TEST_GRIPPER_SAFE_CLOSE_PULSE_US, fake_state->last_pulse_width_us);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_GRIPPER_MIN_RAD, current_pose.gripper_rad);

    pose.gripper_rad = ROBOT_ARM_TEST_DEG_TO_RAD(50.0f);
    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_UINT8_EQUAL(5U, fake_state->last_channel);
    TEST_ASSERT_UINT16_EQUAL(ROBOT_ARM_TEST_GRIPPER_SAFE_OPEN_PULSE_US, fake_state->last_pulse_width_us);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_GRIPPER_MAX_RAD, current_pose.gripper_rad);
    return true;
}

static bool test_robot_arm_home_returns_to_zero_pose(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_non_home_pose(&pose);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_set_pose_immediate(&robot, &pose));

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_home(&robot));
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_UINT8_EQUAL(5U, fake_state->last_channel);

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(ROBOT_ARM_TEST_GRIPPER_HOME_RAD, current_pose.gripper_rad);
    return true;
}

static bool test_robot_arm_set_pose_reports_servo_failures(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_non_home_pose(&pose);

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    fake_state->next_status = PCA9685_ERR_I2C;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_SERVO, robot_arm_set_pose_immediate(&robot, &pose));
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->call_count);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_get_current_pose(&robot, &current_pose));
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, current_pose.gripper_rad);
    return true;
}

static bool test_robot_arm_validates_arguments(void)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    float home_angle_rad;

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_init(0, &test_device));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_init(&robot, 0));

    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_OK, robot_arm_init(&robot, &test_device));
    fill_non_home_pose(&pose);

    TEST_ASSERT_TRUE(robot_arm_get_servo(0, ROBOT_ARM_JOINT_BASE) == 0);
    TEST_ASSERT_TRUE(robot_arm_get_servo_const(0, ROBOT_ARM_JOINT_BASE) == 0);
    TEST_ASSERT_TRUE(robot_arm_get_servo(&robot, ROBOT_ARM_JOINT_COUNT) == 0);
    TEST_ASSERT_TRUE(robot_arm_get_servo_const(&robot, ROBOT_ARM_JOINT_COUNT) == 0);
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_get_home_angle_rad(0, ROBOT_ARM_JOINT_BASE, &home_angle_rad));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_get_home_angle_rad(&robot, ROBOT_ARM_JOINT_COUNT, &home_angle_rad));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_get_home_angle_rad(&robot, ROBOT_ARM_JOINT_BASE, 0));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_get_home_pose(0, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_get_home_pose(&robot, 0));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_get_current_pose(0, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_get_current_pose(&robot, 0));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_set_pose_immediate(0, &pose));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_set_pose_immediate(&robot, 0));
    TEST_ASSERT_INT_EQUAL(ROBOT_ARM_ERR_INVALID_ARGUMENT, robot_arm_home(0));
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "robot_arm_init_configures_joints_and_home_pose", test_robot_arm_init_configures_joints_and_home_pose },
        { "robot_arm_set_pose_immediate_updates_all_joint_state", test_robot_arm_set_pose_immediate_updates_all_joint_state },
        { "robot_arm_set_pose_clamps_base_to_safe_limits", test_robot_arm_set_pose_clamps_base_to_safe_limits },
        { "robot_arm_set_pose_clamps_shoulder_to_safe_limits", test_robot_arm_set_pose_clamps_shoulder_to_safe_limits },
        { "robot_arm_shoulder_uses_piecewise_pulse_range", test_robot_arm_shoulder_uses_piecewise_pulse_range },
        { "robot_arm_set_pose_clamps_elbow_to_safe_limits", test_robot_arm_set_pose_clamps_elbow_to_safe_limits },
        { "robot_arm_elbow_uses_expanded_negative_pulse_range", test_robot_arm_elbow_uses_expanded_negative_pulse_range },
        { "robot_arm_set_pose_clamps_gripper_to_safe_limits", test_robot_arm_set_pose_clamps_gripper_to_safe_limits },
        { "robot_arm_home_returns_to_zero_pose", test_robot_arm_home_returns_to_zero_pose },
        { "robot_arm_set_pose_reports_servo_failures", test_robot_arm_set_pose_reports_servo_failures },
        { "robot_arm_validates_arguments", test_robot_arm_validates_arguments },
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

    printf("All %u robot arm unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}