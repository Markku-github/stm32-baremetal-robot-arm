#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "board_nucleo_f767zi.h"
#include "boot_self_test.h"
#include "debug_console.h"
#include "pca9685_fake.h"
#include "robot_arm.h"

#define TEST_BOOT_SELF_TEST_FLOAT_TOLERANCE 0.0020f

static pca9685_device_t test_device = {
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

#define TEST_ASSERT_UINT16_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %u, got %u\n", __FILE__, __LINE__, (unsigned int)(expected), (unsigned int)(actual)); \
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
        if (!float_is_close((actual), (expected), TEST_BOOT_SELF_TEST_FLOAT_TOLERANCE)) \
        { \
            printf("Assertion failed at %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, (double)(expected), (double)(actual)); \
            return false; \
        } \
    } while (0)

bsp_i2c_status_t board_nucleo_f767zi_init_pca9685_i2c(void)
{
    return BSP_I2C_OK;
}

bsp_uart_status_t board_nucleo_f767zi_init_log_uart(void)
{
    return BSP_UART_OK;
}

void board_nucleo_f767zi_write_debug_string(const char *message)
{
    (void)message;
}

void board_nucleo_f767zi_write_log_string(const char *message)
{
    (void)message;
}

void debug_console_write_prompt(void)
{
}

void debug_console_write_hex_byte(uint8_t value)
{
    (void)value;
}

void debug_console_write_hex_word(uint16_t value)
{
    (void)value;
}

void debug_console_write_string_adapter(void *context, const char *text)
{
    (void)context;
    (void)text;
}

void debug_console_write_byte_adapter(void *context, uint8_t byte)
{
    (void)context;
    (void)byte;
}

void debug_console_write_prompt_adapter(void *context)
{
    (void)context;
}

static bool test_boot_self_test_run_robot_direct_pose_uses_runtime_readback_mapping(void)
{
    pca9685_fake_state_t *fake_state;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_TRUE(boot_self_test_run_robot_direct_pose(&test_device));
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->readback_only_call_count);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->read_call_count);
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->disable_call_count);
    TEST_ASSERT_UINT16_EQUAL(1322U, fake_state->last_pulse_width_us_by_channel[ROBOT_ARM_JOINT_SHOULDER]);
    TEST_ASSERT_UINT16_EQUAL(0x010FU, fake_state->last_off_count_by_channel[ROBOT_ARM_JOINT_SHOULDER]);
    return true;
}

static bool test_boot_self_test_run_robot_home_uses_disabled_readback_mapping(void)
{
    pca9685_fake_state_t *fake_state;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_TRUE(boot_self_test_run_robot_home(&test_device));
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->readback_only_call_count);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->read_call_count);
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->disable_call_count);
    TEST_ASSERT_UINT16_EQUAL(1200U, fake_state->last_pulse_width_us_by_channel[ROBOT_ARM_JOINT_SHOULDER]);
    TEST_ASSERT_UINT16_EQUAL(0x00F6U, fake_state->last_off_count_by_channel[ROBOT_ARM_JOINT_SHOULDER]);
    return true;
}

static bool test_boot_self_test_run_pca9685_keeps_output_disabled_during_readback(void)
{
    pca9685_fake_state_t *fake_state;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_TRUE(boot_self_test_run_pca9685(&test_device));
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->call_count);
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->readback_only_call_count);
    TEST_ASSERT_UINT32_EQUAL(2U, fake_state->read_call_count);
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->disable_call_count);
    TEST_ASSERT_UINT16_EQUAL(1500U, fake_state->last_pulse_width_us_by_channel[0U]);
    return true;
}

static bool test_boot_self_test_restore_preserved_robot_outputs_restores_home_hold(void)
{
    robot_arm_t robot;
    robot_arm_pose_t home_pose;
    robot_arm_pose_t current_pose;
    pca9685_fake_state_t *fake_state;
    uint16_t pulse_width_us = 0U;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_TRUE(robot_arm_init(&robot, &test_device) == ROBOT_ARM_OK);
    TEST_ASSERT_TRUE(robot_arm_get_home_pose(&robot, &home_pose) == ROBOT_ARM_OK);

    TEST_ASSERT_TRUE(robot_arm_calculate_joint_pulse_width_us(&robot, ROBOT_ARM_JOINT_BASE, home_pose.base_rad, &pulse_width_us) == ROBOT_ARM_OK);
    fake_state->current_pulse_width_us_by_channel[ROBOT_ARM_JOINT_BASE] = pulse_width_us;
    fake_state->current_full_off_by_channel[ROBOT_ARM_JOINT_BASE] = 0U;
    TEST_ASSERT_TRUE(robot_arm_calculate_joint_pulse_width_us(&robot, ROBOT_ARM_JOINT_SHOULDER, home_pose.shoulder_rad, &pulse_width_us) == ROBOT_ARM_OK);
    fake_state->current_pulse_width_us_by_channel[ROBOT_ARM_JOINT_SHOULDER] = pulse_width_us;
    fake_state->current_full_off_by_channel[ROBOT_ARM_JOINT_SHOULDER] = 0U;
    TEST_ASSERT_TRUE(robot_arm_calculate_joint_pulse_width_us(&robot, ROBOT_ARM_JOINT_ELBOW, home_pose.elbow_rad, &pulse_width_us) == ROBOT_ARM_OK);
    fake_state->current_pulse_width_us_by_channel[ROBOT_ARM_JOINT_ELBOW] = pulse_width_us;
    fake_state->current_full_off_by_channel[ROBOT_ARM_JOINT_ELBOW] = 0U;
    TEST_ASSERT_TRUE(robot_arm_calculate_joint_pulse_width_us(&robot, ROBOT_ARM_JOINT_WRIST_TILT, home_pose.wrist_tilt_rad, &pulse_width_us) == ROBOT_ARM_OK);
    fake_state->current_pulse_width_us_by_channel[ROBOT_ARM_JOINT_WRIST_TILT] = pulse_width_us;
    fake_state->current_full_off_by_channel[ROBOT_ARM_JOINT_WRIST_TILT] = 0U;
    TEST_ASSERT_TRUE(robot_arm_calculate_joint_pulse_width_us(&robot, ROBOT_ARM_JOINT_WRIST_ROTATE, home_pose.wrist_rotate_rad, &pulse_width_us) == ROBOT_ARM_OK);
    fake_state->current_pulse_width_us_by_channel[ROBOT_ARM_JOINT_WRIST_ROTATE] = pulse_width_us;
    fake_state->current_full_off_by_channel[ROBOT_ARM_JOINT_WRIST_ROTATE] = 0U;
    TEST_ASSERT_TRUE(robot_arm_calculate_joint_pulse_width_us(&robot, ROBOT_ARM_JOINT_GRIPPER, home_pose.gripper_rad, &pulse_width_us) == ROBOT_ARM_OK);
    fake_state->current_pulse_width_us_by_channel[ROBOT_ARM_JOINT_GRIPPER] = pulse_width_us;
    fake_state->current_full_off_by_channel[ROBOT_ARM_JOINT_GRIPPER] = 0U;

    TEST_ASSERT_TRUE(boot_self_test_run_pca9685(&test_device));

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    TEST_ASSERT_TRUE(robot_arm_init(&robot, &test_device) == ROBOT_ARM_OK);
    TEST_ASSERT_TRUE(boot_self_test_restore_preserved_robot_outputs(&robot));
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_TRUE(robot_arm_get_current_pose(&robot, &current_pose) == ROBOT_ARM_OK);
    TEST_ASSERT_UINT16_EQUAL(1802U, fake_state->last_pulse_width_us_by_channel[ROBOT_ARM_JOINT_BASE]);
    TEST_ASSERT_UINT16_EQUAL(0U, fake_state->current_full_off_by_channel[ROBOT_ARM_JOINT_BASE]);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.base_rad, current_pose.base_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.shoulder_rad, current_pose.shoulder_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.elbow_rad, current_pose.elbow_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.wrist_tilt_rad, current_pose.wrist_tilt_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.wrist_rotate_rad, current_pose.wrist_rotate_rad);
    TEST_ASSERT_FLOAT_CLOSE(home_pose.gripper_rad, current_pose.gripper_rad);
    return true;
}

static bool test_boot_self_test_run_robot_direct_pose_validates_arguments(void)
{
    TEST_ASSERT_TRUE(!boot_self_test_run_robot_direct_pose(0));
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "boot_self_test_restore_preserved_robot_outputs_restores_home_hold", test_boot_self_test_restore_preserved_robot_outputs_restores_home_hold },
        { "boot_self_test_run_pca9685_keeps_output_disabled_during_readback", test_boot_self_test_run_pca9685_keeps_output_disabled_during_readback },
        { "boot_self_test_run_robot_home_uses_disabled_readback_mapping", test_boot_self_test_run_robot_home_uses_disabled_readback_mapping },
        { "boot_self_test_run_robot_direct_pose_uses_runtime_readback_mapping", test_boot_self_test_run_robot_direct_pose_uses_runtime_readback_mapping },
        { "boot_self_test_run_robot_direct_pose_validates_arguments", test_boot_self_test_run_robot_direct_pose_validates_arguments },
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

    printf("All boot_self_test tests passed.\n");
    return 0;
}