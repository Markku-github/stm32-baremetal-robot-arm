/**
 ******************************************************************************
 * @file    robot_arm.c
 * @brief   Robot-level direct-pose helpers built on the robot calibration module
 ******************************************************************************
 */

#include <stdbool.h>

#include "robot_arm.h"
#include "robot_arm_joint_calibration.h"

static bool robot_arm_is_valid_joint(robot_arm_joint_id_t joint_id)
{
    return joint_id < ROBOT_ARM_JOINT_COUNT;
}

static bool robot_arm_is_valid_runtime(const robot_arm_t *robot)
{
    return robot != 0;
}

static void robot_arm_fill_pose_from_array(robot_arm_pose_t *pose, const float joint_angles_rad[ROBOT_ARM_JOINT_COUNT])
{
    pose->base_rad = joint_angles_rad[ROBOT_ARM_JOINT_BASE];
    pose->shoulder_rad = joint_angles_rad[ROBOT_ARM_JOINT_SHOULDER];
    pose->elbow_rad = joint_angles_rad[ROBOT_ARM_JOINT_ELBOW];
    pose->wrist_tilt_rad = joint_angles_rad[ROBOT_ARM_JOINT_WRIST_TILT];
    pose->wrist_rotate_rad = joint_angles_rad[ROBOT_ARM_JOINT_WRIST_ROTATE];
    pose->gripper_rad = joint_angles_rad[ROBOT_ARM_JOINT_GRIPPER];
}

static void robot_arm_fill_pose_from_servos(robot_arm_pose_t *pose, const servo_t servos[ROBOT_ARM_JOINT_COUNT])
{
    pose->base_rad = servos[ROBOT_ARM_JOINT_BASE].current_angle_rad;
    pose->shoulder_rad = servos[ROBOT_ARM_JOINT_SHOULDER].current_angle_rad;
    pose->elbow_rad = servos[ROBOT_ARM_JOINT_ELBOW].current_angle_rad;
    pose->wrist_tilt_rad = servos[ROBOT_ARM_JOINT_WRIST_TILT].current_angle_rad;
    pose->wrist_rotate_rad = servos[ROBOT_ARM_JOINT_WRIST_ROTATE].current_angle_rad;
    pose->gripper_rad = servos[ROBOT_ARM_JOINT_GRIPPER].current_angle_rad;
}

static float robot_arm_pose_joint_angle(const robot_arm_pose_t *pose, robot_arm_joint_id_t joint_id)
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

static robot_arm_status_t robot_arm_map_servo_status(servo_status_t status)
{
    if (status == SERVO_OK)
    {
        return ROBOT_ARM_OK;
    }

    if (status == SERVO_ERR_INVALID_ARGUMENT)
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    return ROBOT_ARM_ERR_SERVO;
}

static robot_arm_status_t robot_arm_map_pca9685_status(pca9685_status_t status)
{
    if (status == PCA9685_OK)
    {
        return ROBOT_ARM_OK;
    }

    if ((status == PCA9685_ERR_INVALID_ARGUMENT) || (status == PCA9685_ERR_STATE))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    return ROBOT_ARM_ERR_SERVO;
}

static uint16_t robot_arm_interpolate_pulse_us(
    float angle_rad,
    float minimum_angle_rad,
    float maximum_angle_rad,
    uint16_t minimum_pulse_width_us,
    uint16_t maximum_pulse_width_us)
{
    float normalized_position;
    float pulse_width_f;
    float pulse_low_us;
    float pulse_high_us;

    normalized_position = (angle_rad - minimum_angle_rad) / (maximum_angle_rad - minimum_angle_rad);
    pulse_width_f = (float)minimum_pulse_width_us
        + (normalized_position * ((float)maximum_pulse_width_us - (float)minimum_pulse_width_us));

    pulse_low_us = (float)minimum_pulse_width_us;
    if ((float)maximum_pulse_width_us < pulse_low_us)
    {
        pulse_low_us = (float)maximum_pulse_width_us;
    }

    pulse_high_us = (float)maximum_pulse_width_us;
    if ((float)minimum_pulse_width_us > pulse_high_us)
    {
        pulse_high_us = (float)minimum_pulse_width_us;
    }

    if (pulse_width_f < pulse_low_us)
    {
        pulse_width_f = pulse_low_us;
    }
    else if (pulse_width_f > pulse_high_us)
    {
        pulse_width_f = pulse_high_us;
    }

    return (uint16_t)(pulse_width_f + 0.5f);
}

static robot_arm_status_t robot_arm_calculate_shoulder_pulse_width_us(
    const servo_t *servo,
    float angle_rad,
    float *clamped_angle_rad,
    uint16_t *pulse_width_us)
{
    float clamped_angle_rad_local;
    const float shoulder_mid_angle_rad = robot_arm_joint_calibration_shoulder_mid_angle_rad();
    const uint16_t shoulder_mid_pulse_us = robot_arm_joint_calibration_shoulder_mid_pulse_us();

    if ((servo == 0) || (clamped_angle_rad == 0) || (pulse_width_us == 0))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    if (robot_arm_map_servo_status(servo_clamp_angle_rad(servo, angle_rad, &clamped_angle_rad_local)) != ROBOT_ARM_OK)
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    if (clamped_angle_rad_local <= shoulder_mid_angle_rad)
    {
        *pulse_width_us = robot_arm_interpolate_pulse_us(
            clamped_angle_rad_local,
            servo->minimum_angle_rad,
            shoulder_mid_angle_rad,
            servo->minimum_pulse_width_us,
            shoulder_mid_pulse_us);
    }
    else
    {
        *pulse_width_us = robot_arm_interpolate_pulse_us(
            clamped_angle_rad_local,
            shoulder_mid_angle_rad,
            servo->maximum_angle_rad,
            shoulder_mid_pulse_us,
            servo->maximum_pulse_width_us);
    }

    *clamped_angle_rad = clamped_angle_rad_local;
    return ROBOT_ARM_OK;
}

static robot_arm_status_t robot_arm_set_shoulder_angle_immediate(servo_t *servo, float angle_rad)
{
    float clamped_angle_rad;
    uint16_t pulse_width_us;

    if ((servo == 0) || (servo->device == 0))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    if (robot_arm_calculate_shoulder_pulse_width_us(servo, angle_rad, &clamped_angle_rad, &pulse_width_us) != ROBOT_ARM_OK)
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    if (robot_arm_map_pca9685_status(pca9685_set_channel_pulse_us(servo->device, servo->channel, pulse_width_us)) != ROBOT_ARM_OK)
    {
        return ROBOT_ARM_ERR_SERVO;
    }

    servo->current_angle_rad = clamped_angle_rad;
    servo->target_angle_rad = clamped_angle_rad;
    return ROBOT_ARM_OK;
}
robot_arm_status_t robot_arm_init(robot_arm_t *robot, const pca9685_device_t *device)
{
    uint8_t joint_index;

    if ((robot == 0) || (device == 0))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        servo_config_t servo_config;
        const robot_arm_joint_calibration_t *calibration = robot_arm_joint_calibration_get((robot_arm_joint_id_t)joint_index);

        if (calibration == 0)
        {
            return ROBOT_ARM_ERR_INVALID_ARGUMENT;
        }

        robot_arm_joint_calibration_build_servo_config(calibration, &servo_config);

        const robot_arm_status_t status = robot_arm_map_servo_status(
            servo_init(&robot->servos[joint_index], &servo_config, device));

        if (status != ROBOT_ARM_OK)
        {
            return status;
        }

        robot->home_pose_rad[joint_index] = calibration->home_angle_rad;
    }

    return ROBOT_ARM_OK;
}

servo_t *robot_arm_get_servo(robot_arm_t *robot, robot_arm_joint_id_t joint_id)
{
    if ((robot == 0) || !robot_arm_is_valid_joint(joint_id))
    {
        return 0;
    }

    return &robot->servos[(uint8_t)joint_id];
}

const servo_t *robot_arm_get_servo_const(const robot_arm_t *robot, robot_arm_joint_id_t joint_id)
{
    if ((robot == 0) || !robot_arm_is_valid_joint(joint_id))
    {
        return 0;
    }

    return &robot->servos[(uint8_t)joint_id];
}

robot_arm_status_t robot_arm_get_home_angle_rad(
    const robot_arm_t *robot,
    robot_arm_joint_id_t joint_id,
    float *home_angle_rad)
{
    if ((robot == 0) || (home_angle_rad == 0) || !robot_arm_is_valid_joint(joint_id))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    *home_angle_rad = robot->home_pose_rad[(uint8_t)joint_id];
    return ROBOT_ARM_OK;
}

robot_arm_status_t robot_arm_get_home_pose(const robot_arm_t *robot, robot_arm_pose_t *pose)
{
    if (!robot_arm_is_valid_runtime(robot) || (pose == 0))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    robot_arm_fill_pose_from_array(pose, robot->home_pose_rad);
    return ROBOT_ARM_OK;
}

robot_arm_status_t robot_arm_get_current_pose(const robot_arm_t *robot, robot_arm_pose_t *pose)
{
    if (!robot_arm_is_valid_runtime(robot) || (pose == 0))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    robot_arm_fill_pose_from_servos(pose, robot->servos);
    return ROBOT_ARM_OK;
}

robot_arm_status_t robot_arm_calculate_joint_pulse_width_us(
    const robot_arm_t *robot,
    robot_arm_joint_id_t joint_id,
    float angle_rad,
    uint16_t *pulse_width_us)
{
    const servo_t *servo;
    float clamped_angle_rad;

    if (!robot_arm_is_valid_runtime(robot) || !robot_arm_is_valid_joint(joint_id) || (pulse_width_us == 0))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    servo = &robot->servos[(uint8_t)joint_id];

    if (joint_id == ROBOT_ARM_JOINT_SHOULDER)
    {
        return robot_arm_calculate_shoulder_pulse_width_us(servo, angle_rad, &clamped_angle_rad, pulse_width_us);
    }

    return robot_arm_map_servo_status(servo_angle_rad_to_pulse_us(servo, angle_rad, pulse_width_us));
}

robot_arm_status_t robot_arm_set_pose_immediate(robot_arm_t *robot, const robot_arm_pose_t *pose)
{
    uint8_t joint_index;

    if (!robot_arm_is_valid_runtime(robot) || (pose == 0))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        const robot_arm_joint_id_t joint_id = (robot_arm_joint_id_t)joint_index;
        robot_arm_status_t status;

        if (joint_id == ROBOT_ARM_JOINT_SHOULDER)
        {
            status = robot_arm_set_shoulder_angle_immediate(
                &robot->servos[joint_index],
                robot_arm_pose_joint_angle(pose, joint_id));
        }
        else
        {
            status = robot_arm_map_servo_status(
                servo_set_angle_immediate_rad(
                    &robot->servos[joint_index],
                    robot_arm_pose_joint_angle(pose, joint_id)));
        }

        if (status != ROBOT_ARM_OK)
        {
            return status;
        }
    }

    return ROBOT_ARM_OK;
}

robot_arm_status_t robot_arm_home(robot_arm_t *robot)
{
    robot_arm_pose_t home_pose;

    if (!robot_arm_is_valid_runtime(robot))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    robot_arm_fill_pose_from_array(&home_pose, robot->home_pose_rad);
    return robot_arm_set_pose_immediate(robot, &home_pose);
}