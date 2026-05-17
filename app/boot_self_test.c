/**
 ******************************************************************************
 * @file    boot_self_test.c
 * @brief   Boot-time PCA9685 and robot self-test entry points
 ******************************************************************************
 */

#include "boot_self_test.h"

#include <stdint.h>

#include "board_nucleo_f767zi.h"
#include "pca9685.h"
#include "robot_arm.h"
#include "runtime_log.h"

#define BOOT_SELF_TEST_FREQUENCY_HZ 50U
#define BOOT_SELF_TEST_CHANNEL 0U
#define BOOT_SELF_TEST_PULSE_US 1500U
#define BOOT_SELF_TEST_DEGREES_TO_RADIANS 0.01745329251994329577f
#define BOOT_SELF_TEST_DIRECT_POSE_BASE_DEG 0.0f
#define BOOT_SELF_TEST_DIRECT_POSE_SHOULDER_DEG 10.0f
#define BOOT_SELF_TEST_DIRECT_POSE_ELBOW_DEG 94.285714286f
#define BOOT_SELF_TEST_DIRECT_POSE_WRIST_TILT_DEG 75.0f
#define BOOT_SELF_TEST_DIRECT_POSE_WRIST_ROTATE_DEG 60.0f
#define BOOT_SELF_TEST_DIRECT_POSE_GRIPPER_DEG 10.0f
#define BOOT_SELF_TEST_PCA9685_FULL_OFF_BIT 0x10U
#define BOOT_SELF_TEST_PCA9685_CHANNEL_REGISTER_STRIDE 4U

typedef enum
{
    ROBOT_POSE_READBACK_OK = 0,
    ROBOT_POSE_READBACK_ERR_READ,
    ROBOT_POSE_READBACK_ERR_MISMATCH,
} robot_pose_readback_status_t;

typedef struct
{
    bool valid;
    uint16_t pulse_width_us_by_joint[ROBOT_ARM_JOINT_COUNT];
} boot_self_test_preserved_outputs_t;

static boot_self_test_preserved_outputs_t boot_self_test_preserved_outputs = { 0 };

static uint16_t pca9685_prescale_to_frequency_hz(uint8_t prescale_value)
{
    const uint32_t divisor = (uint32_t)PCA9685_PWM_STEPS * ((uint32_t)prescale_value + 1U);

    if (divisor == 0U)
    {
        return 0U;
    }

    return (uint16_t)((PCA9685_OSCILLATOR_FREQUENCY_HZ + (divisor / 2U)) / divisor);
}

static uint16_t pca9685_off_count_to_pulse_us(uint16_t pwm_frequency_hz, uint16_t off_count)
{
    const uint64_t numerator = ((uint64_t)off_count * 1000000ULL)
        + ((((uint64_t)pwm_frequency_hz * (uint64_t)PCA9685_PWM_STEPS)) / 2ULL);
    const uint64_t denominator = (uint64_t)pwm_frequency_hz * (uint64_t)PCA9685_PWM_STEPS;

    if ((pwm_frequency_hz == 0U) || (denominator == 0ULL))
    {
        return 0U;
    }

    return (uint16_t)(numerator / denominator);
}

static void boot_self_test_clear_preserved_outputs(void)
{
    uint8_t joint_index;

    boot_self_test_preserved_outputs.valid = false;
    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        boot_self_test_preserved_outputs.pulse_width_us_by_joint[joint_index] = 0U;
    }
}

static pca9685_status_t boot_self_test_read_channel_full_off(
    const pca9685_device_t *device,
    uint8_t channel,
    bool *full_off)
{
    uint8_t off_high_value;
    const uint8_t register_base = (uint8_t)(PCA9685_REGISTER_LED0_ON_L + (channel * BOOT_SELF_TEST_PCA9685_CHANNEL_REGISTER_STRIDE));

    if ((device == 0) || (full_off == 0) || (channel >= PCA9685_CHANNEL_COUNT))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    if (pca9685_read_register(device->instance, device->address, (uint8_t)(register_base + 3U), &off_high_value) != PCA9685_OK)
    {
        return PCA9685_ERR_I2C;
    }

    *full_off = (off_high_value & BOOT_SELF_TEST_PCA9685_FULL_OFF_BIT) != 0U;
    return PCA9685_OK;
}

static void boot_self_test_capture_preserved_outputs(void)
{
    pca9685_device_t snapshot_device = {
        .instance = BSP_I2C_INSTANCE_I2C1,
        .address = PCA9685_I2C_ADDRESS_DEFAULT,
        .oscillator_frequency_hz = PCA9685_OSCILLATOR_FREQUENCY_HZ,
        .pwm_frequency_hz = 0U,
    };
    robot_arm_t robot;
    uint8_t prescale_value = 0U;
    uint16_t pwm_frequency_hz;
    uint8_t joint_index;

    boot_self_test_clear_preserved_outputs();

    if (pca9685_read_register(
            snapshot_device.instance,
            snapshot_device.address,
            PCA9685_REGISTER_PRESCALE,
            &prescale_value) != PCA9685_OK)
    {
        return;
    }

    pwm_frequency_hz = pca9685_prescale_to_frequency_hz(prescale_value);
    if (pwm_frequency_hz == 0U)
    {
        return;
    }

    snapshot_device.pwm_frequency_hz = pwm_frequency_hz;
    if (robot_arm_init(&robot, &snapshot_device) != ROBOT_ARM_OK)
    {
        return;
    }

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        uint16_t on_count;
        uint16_t off_count;
        uint16_t pulse_width_us;
        bool full_off = false;
        const servo_t *servo = robot_arm_get_servo_const(&robot, (robot_arm_joint_id_t)joint_index);

        if ((servo == 0)
            || (pca9685_read_channel_pwm(&snapshot_device, servo->channel, &on_count, &off_count) != PCA9685_OK)
            || (boot_self_test_read_channel_full_off(&snapshot_device, servo->channel, &full_off) != PCA9685_OK)
            || full_off
            || (on_count != 0U)
            || (off_count == 0U))
        {
            boot_self_test_clear_preserved_outputs();
            return;
        }

        pulse_width_us = pca9685_off_count_to_pulse_us(pwm_frequency_hz, off_count);
        if (pulse_width_us == 0U)
        {
            boot_self_test_clear_preserved_outputs();
            return;
        }

        boot_self_test_preserved_outputs.pulse_width_us_by_joint[joint_index] = pulse_width_us;
    }

    boot_self_test_preserved_outputs.valid = true;
}

static uint16_t pca9685_self_test_expected_off_count(uint16_t pwm_frequency_hz, uint16_t pulse_width_us)
{
    const uint64_t pulse_counts = ((uint64_t)pulse_width_us * (uint64_t)pwm_frequency_hz * (uint64_t)PCA9685_PWM_STEPS + 500000ULL)
        / 1000000ULL;

    return (uint16_t)pulse_counts;
}

static float degrees_to_radians(float degrees)
{
    return degrees * BOOT_SELF_TEST_DEGREES_TO_RADIANS;
}

static float robot_pose_joint_angle(const robot_arm_pose_t *pose, robot_arm_joint_id_t joint_id)
{
    switch (joint_id)
    {
        case ROBOT_ARM_JOINT_BASE:
            return pose->base_rad;

        case ROBOT_ARM_JOINT_SHOULDER:
            return pose->shoulder_rad;

        case ROBOT_ARM_JOINT_ELBOW:
            return pose->elbow_rad;

        case ROBOT_ARM_JOINT_WRIST_TILT:
            return pose->wrist_tilt_rad;

        case ROBOT_ARM_JOINT_WRIST_ROTATE:
            return pose->wrist_rotate_rad;

        case ROBOT_ARM_JOINT_GRIPPER:
            return pose->gripper_rad;

        default:
            return 0.0f;
    }
}

static void report_robot_self_test_failure(const pca9685_device_t *device, const char *message)
{
    if (device != 0)
    {
        (void)pca9685_disable_all_outputs(device);
    }

    runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, message);
}

static void report_robot_pose_readback_failure(
    const pca9685_device_t *device,
    robot_pose_readback_status_t readback_status,
    const char *read_error_message,
    const char *mismatch_message)
{
    if (readback_status == ROBOT_POSE_READBACK_ERR_MISMATCH)
    {
        report_robot_self_test_failure(device, mismatch_message);
        return;
    }

    report_robot_self_test_failure(device, read_error_message);
}

static bool finalize_robot_self_test(
    const pca9685_device_t *device,
    const char *output_disable_failure_message,
    const char *success_message)
{
    if ((device == 0) || (pca9685_disable_all_outputs(device) != PCA9685_OK))
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, output_disable_failure_message);
        return false;
    }

    runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, success_message);
    return true;
}

static void build_robot_direct_pose(robot_arm_pose_t *pose)
{
    if (pose == 0)
    {
        return;
    }

    pose->base_rad = degrees_to_radians(BOOT_SELF_TEST_DIRECT_POSE_BASE_DEG);
    pose->shoulder_rad = degrees_to_radians(BOOT_SELF_TEST_DIRECT_POSE_SHOULDER_DEG);
    pose->elbow_rad = degrees_to_radians(BOOT_SELF_TEST_DIRECT_POSE_ELBOW_DEG);
    pose->wrist_tilt_rad = degrees_to_radians(BOOT_SELF_TEST_DIRECT_POSE_WRIST_TILT_DEG);
    pose->wrist_rotate_rad = degrees_to_radians(BOOT_SELF_TEST_DIRECT_POSE_WRIST_ROTATE_DEG);
    pose->gripper_rad = degrees_to_radians(BOOT_SELF_TEST_DIRECT_POSE_GRIPPER_DEG);
}

static robot_pose_readback_status_t program_robot_pose_readback_only(
    const robot_arm_t *robot,
    const robot_arm_pose_t *pose,
    const pca9685_device_t *device)
{
    uint8_t joint_index;

    if ((robot == 0) || (pose == 0) || (device == 0))
    {
        return ROBOT_POSE_READBACK_ERR_READ;
    }

    /* Keep boot-time pose verification register-only so powered servos do not twitch during self-tests. */
    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        uint16_t pulse_width_us;
        const robot_arm_joint_id_t joint_id = (robot_arm_joint_id_t)joint_index;
        const servo_t *servo = robot_arm_get_servo_const(robot, joint_id);

        if ((servo == 0)
            || (robot_arm_calculate_joint_pulse_width_us(robot, joint_id, robot_pose_joint_angle(pose, joint_id), &pulse_width_us) != ROBOT_ARM_OK)
            || (pca9685_set_channel_pulse_us_disabled(device, servo->channel, pulse_width_us) != PCA9685_OK))
        {
            return ROBOT_POSE_READBACK_ERR_READ;
        }
    }

    return ROBOT_POSE_READBACK_OK;
}

static robot_pose_readback_status_t write_robot_pose_off_counts(
    const robot_arm_t *robot,
    const robot_arm_pose_t *pose,
    const pca9685_device_t *device,
    const char *label)
{
    uint8_t joint_index;

    if ((robot == 0) || (pose == 0) || (device == 0) || (label == 0))
    {
        return ROBOT_POSE_READBACK_ERR_READ;
    }

    if (!runtime_log_begin_line(RUNTIME_LOG_LEVEL_DEBUG))
    {
        return ROBOT_POSE_READBACK_OK;
    }

    runtime_log_write_raw(label);

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        uint16_t pulse_width_us;
        uint16_t expected_off_count;
        uint16_t on_count;
        uint16_t off_count;
        const robot_arm_joint_id_t joint_id = (robot_arm_joint_id_t)joint_index;
        const servo_t *servo = robot_arm_get_servo_const(robot, joint_id);

        if ((servo == 0)
            || (robot_arm_calculate_joint_pulse_width_us(robot, joint_id, robot_pose_joint_angle(pose, joint_id), &pulse_width_us) != ROBOT_ARM_OK)
            || (pca9685_read_channel_pwm(device, servo->channel, &on_count, &off_count) != PCA9685_OK))
        {
            return ROBOT_POSE_READBACK_ERR_READ;
        }

        expected_off_count = pca9685_self_test_expected_off_count(device->pwm_frequency_hz, pulse_width_us);
        if ((on_count != 0U) || (off_count != expected_off_count))
        {
            return ROBOT_POSE_READBACK_ERR_MISMATCH;
        }

        if (joint_index > 0U)
        {
            runtime_log_write_raw(", ");
        }

        runtime_log_write_raw(servo->name);
        runtime_log_write_raw("=0x");
        runtime_log_write_hex_word(off_count);
    }

    runtime_log_end_line();
    return ROBOT_POSE_READBACK_OK;
}

bool boot_self_test_run_pca9685(pca9685_device_t *device)
{
    uint8_t mode1_value;
    uint8_t mode2_value;
    uint8_t prescale_value;
    uint16_t on_count;
    uint16_t off_count;
    uint16_t expected_off_count;

    if (device == 0)
    {
        return false;
    }

    if (board_nucleo_f767zi_init_pca9685_i2c() != BSP_I2C_OK)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "I2C1 init failed for PCA9685 self-test.");
        return false;
    }

    boot_self_test_capture_preserved_outputs();

    runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "I2C1 ready. Running PCA9685 driver self-test at 0x40...");

    if (pca9685_init(device, BSP_I2C_INSTANCE_I2C1, PCA9685_I2C_ADDRESS_DEFAULT) != PCA9685_OK)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "PCA9685 init failed. Check address, pull-ups, and wiring.");
        return false;
    }

    if (pca9685_set_pwm_frequency(device, BOOT_SELF_TEST_FREQUENCY_HZ) != PCA9685_OK)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "PCA9685 frequency setup failed.");
        return false;
    }

    if (pca9685_set_channel_pulse_us_disabled(device, BOOT_SELF_TEST_CHANNEL, BOOT_SELF_TEST_PULSE_US) != PCA9685_OK)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "PCA9685 pulse-width write failed.");
        return false;
    }

    if ((pca9685_read_register(device->instance, device->address, PCA9685_REGISTER_MODE1, &mode1_value) != PCA9685_OK)
        || (pca9685_read_register(device->instance, device->address, PCA9685_REGISTER_MODE2, &mode2_value) != PCA9685_OK)
        || (pca9685_read_register(device->instance, device->address, PCA9685_REGISTER_PRESCALE, &prescale_value) != PCA9685_OK)
        || (pca9685_read_channel_pwm(device, BOOT_SELF_TEST_CHANNEL, &on_count, &off_count) != PCA9685_OK))
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "PCA9685 readback failed after self-test writes.");
        return false;
    }

    expected_off_count = pca9685_self_test_expected_off_count(device->pwm_frequency_hz, BOOT_SELF_TEST_PULSE_US);

    if (runtime_log_begin_line(RUNTIME_LOG_LEVEL_DEBUG))
    {
        runtime_log_write_raw("PCA9685 MODE1 = 0x");
        runtime_log_write_hex_byte(mode1_value);
        runtime_log_write_raw(", MODE2 = 0x");
        runtime_log_write_hex_byte(mode2_value);
        runtime_log_end_line();
    }

    if (runtime_log_begin_line(RUNTIME_LOG_LEVEL_DEBUG))
    {
        runtime_log_write_raw("PCA9685 PRESCALE = 0x");
        runtime_log_write_hex_byte(prescale_value);
        runtime_log_write_raw(" (expected about 0x79 for 50 Hz @ 25 MHz)");
        runtime_log_end_line();
    }

    if (runtime_log_begin_line(RUNTIME_LOG_LEVEL_DEBUG))
    {
        runtime_log_write_raw("PCA9685 CH0 ON = 0x");
        runtime_log_write_hex_word(on_count);
        runtime_log_write_raw(", OFF = 0x");
        runtime_log_write_hex_word(off_count);
        runtime_log_end_line();
    }

    if ((on_count != 0U) || (off_count != expected_off_count))
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "PCA9685 register readback mismatch.");
        return false;
    }

    if (pca9685_disable_all_outputs(device) != PCA9685_OK)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "PCA9685 output disable failed after self-test.");
        return false;
    }

    runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "PCA9685 driver self-test OK. Outputs remained disabled during this register-level check. External servo power is not required.");
    return true;
}

bool boot_self_test_run_robot_home(pca9685_device_t *device)
{
    robot_arm_t robot;
    robot_arm_pose_t home_pose;
    robot_pose_readback_status_t readback_status;

    if (device == 0)
    {
        return false;
    }

    runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "Running robot HOME integration self-test...");

    if (robot_arm_init(&robot, device) != ROBOT_ARM_OK)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "Robot servo baseline init failed.");
        return false;
    }

    if (robot_arm_get_home_pose(&robot, &home_pose) != ROBOT_ARM_OK)
    {
        report_robot_self_test_failure(device, "Robot HOME pose lookup failed.");
        return false;
    }

    if (program_robot_pose_readback_only(&robot, &home_pose, device) != ROBOT_POSE_READBACK_OK)
    {
        report_robot_self_test_failure(device, "Robot HOME register programming failed.");
        return false;
    }

    readback_status = write_robot_pose_off_counts(&robot, &home_pose, device, "Robot HOME OFF counts: ");

    if (readback_status != ROBOT_POSE_READBACK_OK)
    {
        report_robot_pose_readback_failure(
            device,
            readback_status,
            "Robot HOME readback failed.",
            "Robot HOME register readback mismatch.");
        return false;
    }

    return finalize_robot_self_test(
        device,
        "Robot HOME output disable failed after self-test.",
        "Robot HOME integration self-test OK. Outputs remained disabled during this register-level check. External servo power is still not required.");
}

bool boot_self_test_run_robot_direct_pose(pca9685_device_t *device)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_pose_readback_status_t readback_status;

    if (device == 0)
    {
        return false;
    }

    runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "Running robot direct pose integration self-test...");

    if (robot_arm_init(&robot, device) != ROBOT_ARM_OK)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "Robot servo baseline init failed for direct pose test.");
        return false;
    }

    build_robot_direct_pose(&pose);

    if (program_robot_pose_readback_only(&robot, &pose, device) != ROBOT_POSE_READBACK_OK)
    {
        report_robot_self_test_failure(device, "Robot direct pose register programming failed.");
        return false;
    }

    readback_status = write_robot_pose_off_counts(&robot, &pose, device, "Robot direct pose OFF counts: ");

    if (readback_status != ROBOT_POSE_READBACK_OK)
    {
        report_robot_pose_readback_failure(
            device,
            readback_status,
            "Robot direct pose readback failed.",
            "Robot direct pose register readback mismatch.");
        return false;
    }

    return finalize_robot_self_test(
        device,
        "Robot direct pose output disable failed after self-test.",
        "Robot direct pose integration self-test OK. Outputs remained disabled during this register-level check. External servo power is still not required.");
}

bool boot_self_test_restore_preserved_robot_outputs(robot_arm_t *robot)
{
    const bool had_preserved_outputs = boot_self_test_preserved_outputs.valid;

    if (!had_preserved_outputs || (robot == 0))
    {
        return false;
    }

    boot_self_test_preserved_outputs.valid = false;

    return robot_arm_restore_pose_from_pulse_widths(
               robot,
               boot_self_test_preserved_outputs.pulse_width_us_by_joint) == ROBOT_ARM_OK;
}