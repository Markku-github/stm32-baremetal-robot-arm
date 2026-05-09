/**
 ******************************************************************************
 * @file    robot_arm.h
 * @brief   Robot-level servo configuration and conservative baseline limits
 ******************************************************************************
 */

#ifndef ROBOT_ARM_H
#define ROBOT_ARM_H

#include "servo.h"

typedef enum
{
    ROBOT_ARM_OK = 0,
    ROBOT_ARM_ERR_INVALID_ARGUMENT,
    ROBOT_ARM_ERR_SERVO,
} robot_arm_status_t;

typedef enum
{
    ROBOT_ARM_JOINT_BASE = 0,
    ROBOT_ARM_JOINT_SHOULDER,
    ROBOT_ARM_JOINT_ELBOW,
    ROBOT_ARM_JOINT_WRIST_TILT,
    ROBOT_ARM_JOINT_WRIST_ROTATE,
    ROBOT_ARM_JOINT_GRIPPER,
    ROBOT_ARM_JOINT_COUNT,
} robot_arm_joint_id_t;

typedef struct
{
    servo_t servos[ROBOT_ARM_JOINT_COUNT];
    float home_pose_rad[ROBOT_ARM_JOINT_COUNT];
} robot_arm_t;

/**
 * @brief  Initialize the baseline six-servo robot configuration
 * @param  robot: destination for robot-level servo state
 * @param  device: initialized PCA9685 device backing all servo channels
 * @retval ROBOT_ARM_OK: robot servo configuration initialized successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot or device invalid
 * @retval ROBOT_ARM_ERR_SERVO: one of the underlying servo descriptors failed to initialize
 */
robot_arm_status_t robot_arm_init(robot_arm_t *robot, const pca9685_device_t *device);

/**
 * @brief  Return a mutable servo descriptor for one named robot joint
 * @param  robot: initialized robot descriptor
 * @param  joint_id: logical joint identifier
 * @retval servo_t*: matching servo descriptor
 * @retval 0: robot or joint identifier invalid
 */
servo_t *robot_arm_get_servo(robot_arm_t *robot, robot_arm_joint_id_t joint_id);

/**
 * @brief  Return an immutable servo descriptor for one named robot joint
 * @param  robot: initialized robot descriptor
 * @param  joint_id: logical joint identifier
 * @retval const servo_t*: matching servo descriptor
 * @retval 0: robot or joint identifier invalid
 */
const servo_t *robot_arm_get_servo_const(const robot_arm_t *robot, robot_arm_joint_id_t joint_id);

/**
 * @brief  Read the baseline HOME angle for one named robot joint
 * @param  robot: initialized robot descriptor
 * @param  joint_id: logical joint identifier
 * @param  home_angle_rad: destination for the configured HOME angle in radians
 * @retval ROBOT_ARM_OK: HOME angle returned successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot, joint identifier, or output pointer invalid
 */
robot_arm_status_t robot_arm_get_home_angle_rad(
    const robot_arm_t *robot,
    robot_arm_joint_id_t joint_id,
    float *home_angle_rad);

#endif