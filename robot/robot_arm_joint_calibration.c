/**
 ******************************************************************************
 * @file    robot_arm_joint_calibration.c
 * @brief   Robot-specific joint calibration data and servo-config builders
 ******************************************************************************
 */

#include "robot_arm_joint_calibration.h"

#define ROBOT_ARM_DEGREES_TO_RADIANS 0.01745329251994329577f
#define ROBOT_ARM_BASE_HOME_ANGLE_RAD (90.0f * ROBOT_ARM_DEGREES_TO_RADIANS)
#define ROBOT_ARM_SHOULDER_MID_ANGLE_RAD (90.0f * ROBOT_ARM_DEGREES_TO_RADIANS)
#define ROBOT_ARM_SHOULDER_SAFE_MIN_PULSE_US 1200U
#define ROBOT_ARM_SHOULDER_SAFE_MID_PULSE_US 2300U
#define ROBOT_ARM_SHOULDER_SAFE_MAX_PULSE_US 3200U
#define ROBOT_ARM_ELBOW_SAFE_MIN_ANGLE_RAD 0.0f
#define ROBOT_ARM_ELBOW_SAFE_MAX_ANGLE_RAD (180.0f * ROBOT_ARM_DEGREES_TO_RADIANS)
#define ROBOT_ARM_ELBOW_HOME_ANGLE_RAD (180.0f * ROBOT_ARM_DEGREES_TO_RADIANS)
#define ROBOT_ARM_ELBOW_SAFE_MIN_PULSE_US 450U
#define ROBOT_ARM_ELBOW_SAFE_MAX_PULSE_US 2500U
#define ROBOT_ARM_WRIST_TILT_SAFE_MIN_ANGLE_RAD 0.0f
#define ROBOT_ARM_WRIST_TILT_SAFE_MAX_ANGLE_RAD (180.0f * ROBOT_ARM_DEGREES_TO_RADIANS)
#define ROBOT_ARM_WRIST_TILT_HOME_ANGLE_RAD (180.0f * ROBOT_ARM_DEGREES_TO_RADIANS)
#define ROBOT_ARM_WRIST_TILT_SAFE_MIN_PULSE_US 2800U
#define ROBOT_ARM_WRIST_TILT_SAFE_MAX_PULSE_US 600U
#define ROBOT_ARM_WRIST_ROTATE_SAFE_MIN_ANGLE_RAD 0.0f
#define ROBOT_ARM_WRIST_ROTATE_SAFE_MAX_ANGLE_RAD (180.0f * ROBOT_ARM_DEGREES_TO_RADIANS)
#define ROBOT_ARM_WRIST_ROTATE_HOME_ANGLE_RAD (90.0f * ROBOT_ARM_DEGREES_TO_RADIANS)
#define ROBOT_ARM_WRIST_ROTATE_SAFE_MIN_PULSE_US 450U
#define ROBOT_ARM_WRIST_ROTATE_SAFE_MAX_PULSE_US 3000U
#define ROBOT_ARM_GRIPPER_HOME_ANGLE_RAD 0.0f
#define ROBOT_ARM_GRIPPER_SAFE_CLOSE_PULSE_US 2450U
#define ROBOT_ARM_GRIPPER_SAFE_OPEN_PULSE_US 1700U

static bool robot_arm_joint_calibration_is_valid_joint(robot_arm_joint_id_t joint_id)
{
    return joint_id < ROBOT_ARM_JOINT_COUNT;
}

static const robot_arm_joint_calibration_t robot_arm_default_joint_calibrations[ROBOT_ARM_JOINT_COUNT] = {
    {
        .name = "base",
        .channel = 0U,
        .minimum_angle_rad = 0.0f,
        .maximum_angle_rad = 90.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .home_angle_rad = ROBOT_ARM_BASE_HOME_ANGLE_RAD,
        .offset_rad = 0.0f,
        .pulse_width_at_min_angle_us = 600U,
        .pulse_width_at_max_angle_us = 1800U,
    },
    {
        .name = "shoulder",
        .channel = 1U,
        .minimum_angle_rad = 0.0f,
        .maximum_angle_rad = 180.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .home_angle_rad = 0.0f,
        .offset_rad = 0.0f,
        .pulse_width_at_min_angle_us = ROBOT_ARM_SHOULDER_SAFE_MIN_PULSE_US,
        .pulse_width_at_max_angle_us = ROBOT_ARM_SHOULDER_SAFE_MAX_PULSE_US,
    },
    {
        .name = "elbow",
        .channel = 2U,
        .minimum_angle_rad = ROBOT_ARM_ELBOW_SAFE_MIN_ANGLE_RAD,
        .maximum_angle_rad = ROBOT_ARM_ELBOW_SAFE_MAX_ANGLE_RAD,
        .home_angle_rad = ROBOT_ARM_ELBOW_HOME_ANGLE_RAD,
        .offset_rad = 0.0f,
        .pulse_width_at_min_angle_us = ROBOT_ARM_ELBOW_SAFE_MIN_PULSE_US,
        .pulse_width_at_max_angle_us = ROBOT_ARM_ELBOW_SAFE_MAX_PULSE_US,
    },
    {
        .name = "wrist_tilt",
        .channel = 3U,
        .minimum_angle_rad = ROBOT_ARM_WRIST_TILT_SAFE_MIN_ANGLE_RAD,
        .maximum_angle_rad = ROBOT_ARM_WRIST_TILT_SAFE_MAX_ANGLE_RAD,
        .home_angle_rad = ROBOT_ARM_WRIST_TILT_HOME_ANGLE_RAD,
        .offset_rad = 0.0f,
        .pulse_width_at_min_angle_us = ROBOT_ARM_WRIST_TILT_SAFE_MIN_PULSE_US,
        .pulse_width_at_max_angle_us = ROBOT_ARM_WRIST_TILT_SAFE_MAX_PULSE_US,
    },
    {
        .name = "wrist_rotate",
        .channel = 4U,
        .minimum_angle_rad = ROBOT_ARM_WRIST_ROTATE_SAFE_MIN_ANGLE_RAD,
        .maximum_angle_rad = ROBOT_ARM_WRIST_ROTATE_SAFE_MAX_ANGLE_RAD,
        .home_angle_rad = ROBOT_ARM_WRIST_ROTATE_HOME_ANGLE_RAD,
        .offset_rad = 0.0f,
        .pulse_width_at_min_angle_us = ROBOT_ARM_WRIST_ROTATE_SAFE_MIN_PULSE_US,
        .pulse_width_at_max_angle_us = ROBOT_ARM_WRIST_ROTATE_SAFE_MAX_PULSE_US,
    },
    {
        .name = "gripper",
        .channel = 5U,
        .minimum_angle_rad = 0.0f,
        .maximum_angle_rad = 20.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .home_angle_rad = ROBOT_ARM_GRIPPER_HOME_ANGLE_RAD,
        .offset_rad = 0.0f,
        .pulse_width_at_min_angle_us = ROBOT_ARM_GRIPPER_SAFE_CLOSE_PULSE_US,
        .pulse_width_at_max_angle_us = ROBOT_ARM_GRIPPER_SAFE_OPEN_PULSE_US,
    },
};

const robot_arm_joint_calibration_t *robot_arm_joint_calibration_get(robot_arm_joint_id_t joint_id)
{
    if (!robot_arm_joint_calibration_is_valid_joint(joint_id))
    {
        return 0;
    }

    return &robot_arm_default_joint_calibrations[(uint8_t)joint_id];
}

void robot_arm_joint_calibration_build_servo_config(
    const robot_arm_joint_calibration_t *calibration,
    servo_config_t *config)
{
    if ((calibration == 0) || (config == 0))
    {
        return;
    }

    config->name = calibration->name;
    config->channel = calibration->channel;
    config->minimum_angle_rad = calibration->minimum_angle_rad;
    config->maximum_angle_rad = calibration->maximum_angle_rad;
    config->offset_rad = calibration->offset_rad;
    config->minimum_pulse_width_us = calibration->pulse_width_at_min_angle_us;
    config->maximum_pulse_width_us = calibration->pulse_width_at_max_angle_us;
}

float robot_arm_joint_calibration_shoulder_mid_angle_rad(void)
{
    return ROBOT_ARM_SHOULDER_MID_ANGLE_RAD;
}

uint16_t robot_arm_joint_calibration_shoulder_mid_pulse_us(void)
{
    return ROBOT_ARM_SHOULDER_SAFE_MID_PULSE_US;
}