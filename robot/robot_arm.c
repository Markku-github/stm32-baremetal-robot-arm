/**
 ******************************************************************************
 * @file    robot_arm.c
 * @brief   Robot-level servo configuration and conservative baseline limits
 ******************************************************************************
 */

#include <stdbool.h>

#include "robot_arm.h"

#define ROBOT_ARM_DEGREES_TO_RADIANS 0.01745329251994329577f

#define ROBOT_ARM_BASE_PULSE_MIN_US 1200U
#define ROBOT_ARM_BASE_PULSE_MAX_US 1800U
#define ROBOT_ARM_SHOULDER_PULSE_MIN_US 1200U
#define ROBOT_ARM_SHOULDER_PULSE_MAX_US 1800U
#define ROBOT_ARM_ELBOW_PULSE_MIN_US 1200U
#define ROBOT_ARM_ELBOW_PULSE_MAX_US 1800U
#define ROBOT_ARM_WRIST_TILT_PULSE_MIN_US 1250U
#define ROBOT_ARM_WRIST_TILT_PULSE_MAX_US 1750U
#define ROBOT_ARM_WRIST_ROTATE_PULSE_MIN_US 1200U
#define ROBOT_ARM_WRIST_ROTATE_PULSE_MAX_US 1800U
#define ROBOT_ARM_GRIPPER_PULSE_MIN_US 1300U
#define ROBOT_ARM_GRIPPER_PULSE_MAX_US 1700U

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

/* Conservative pre-calibration MG996R baseline. Channel order must be verified before powered movement tests. */
static const servo_config_t robot_arm_default_servo_configs[ROBOT_ARM_JOINT_COUNT] = {
    {
        .name = "base",
        .channel = 0U,
        .minimum_angle_rad = -45.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .maximum_angle_rad = 45.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .offset_rad = 0.0f,
        .minimum_pulse_width_us = ROBOT_ARM_BASE_PULSE_MIN_US,
        .maximum_pulse_width_us = ROBOT_ARM_BASE_PULSE_MAX_US,
    },
    {
        .name = "shoulder",
        .channel = 1U,
        .minimum_angle_rad = -35.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .maximum_angle_rad = 35.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .offset_rad = 0.0f,
        .minimum_pulse_width_us = ROBOT_ARM_SHOULDER_PULSE_MIN_US,
        .maximum_pulse_width_us = ROBOT_ARM_SHOULDER_PULSE_MAX_US,
    },
    {
        .name = "elbow",
        .channel = 2U,
        .minimum_angle_rad = -45.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .maximum_angle_rad = 45.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .offset_rad = 0.0f,
        .minimum_pulse_width_us = ROBOT_ARM_ELBOW_PULSE_MIN_US,
        .maximum_pulse_width_us = ROBOT_ARM_ELBOW_PULSE_MAX_US,
    },
    {
        .name = "wrist_tilt",
        .channel = 3U,
        .minimum_angle_rad = -30.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .maximum_angle_rad = 30.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .offset_rad = 0.0f,
        .minimum_pulse_width_us = ROBOT_ARM_WRIST_TILT_PULSE_MIN_US,
        .maximum_pulse_width_us = ROBOT_ARM_WRIST_TILT_PULSE_MAX_US,
    },
    {
        .name = "wrist_rotate",
        .channel = 4U,
        .minimum_angle_rad = -45.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .maximum_angle_rad = 45.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .offset_rad = 0.0f,
        .minimum_pulse_width_us = ROBOT_ARM_WRIST_ROTATE_PULSE_MIN_US,
        .maximum_pulse_width_us = ROBOT_ARM_WRIST_ROTATE_PULSE_MAX_US,
    },
    {
        .name = "gripper",
        .channel = 5U,
        .minimum_angle_rad = 0.0f,
        .maximum_angle_rad = 20.0f * ROBOT_ARM_DEGREES_TO_RADIANS,
        .offset_rad = 0.0f,
        .minimum_pulse_width_us = ROBOT_ARM_GRIPPER_PULSE_MIN_US,
        .maximum_pulse_width_us = ROBOT_ARM_GRIPPER_PULSE_MAX_US,
    },
};

static const float robot_arm_default_home_pose_rad[ROBOT_ARM_JOINT_COUNT] = {
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
};

robot_arm_status_t robot_arm_init(robot_arm_t *robot, const pca9685_device_t *device)
{
    uint8_t joint_index;

    if ((robot == 0) || (device == 0))
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        const robot_arm_status_t status = robot_arm_map_servo_status(
            servo_init(&robot->servos[joint_index], &robot_arm_default_servo_configs[joint_index], device));

        if (status != ROBOT_ARM_OK)
        {
            return status;
        }

        robot->home_pose_rad[joint_index] = robot_arm_default_home_pose_rad[joint_index];
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
        const robot_arm_status_t status = robot_arm_map_servo_status(
            servo_set_angle_immediate_rad(
                &robot->servos[joint_index],
                robot_arm_pose_joint_angle(pose, joint_id)));

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