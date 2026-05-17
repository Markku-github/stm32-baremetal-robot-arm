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

typedef struct
{
    float base_rad;
    float shoulder_rad;
    float elbow_rad;
    float wrist_tilt_rad;
    float wrist_rotate_rad;
    float gripper_rad;
} robot_arm_pose_t;

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

/**
 * @brief  Read the configured baseline HOME pose as one joint-space structure
 * @param  robot: initialized robot descriptor
 * @param  pose: destination for the HOME pose in radians
 * @retval ROBOT_ARM_OK: HOME pose returned successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot or output pose invalid
 */
robot_arm_status_t robot_arm_get_home_pose(const robot_arm_t *robot, robot_arm_pose_t *pose);

/**
 * @brief  Read the current joint-space pose from the servo runtime state
 * @param  robot: initialized robot descriptor
 * @param  pose: destination for the current pose in radians
 * @retval ROBOT_ARM_OK: current pose returned successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot or output pose invalid
 */
robot_arm_status_t robot_arm_get_current_pose(const robot_arm_t *robot, robot_arm_pose_t *pose);

/**
 * @brief  Update the logical robot pose without writing any servo outputs
 * @param  robot: initialized robot descriptor
 * @param  pose: assumed current pose in radians
 * @retval ROBOT_ARM_OK: runtime pose updated successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot or pose invalid
 */
robot_arm_status_t robot_arm_assume_pose(robot_arm_t *robot, const robot_arm_pose_t *pose);

/**
 * @brief  Mark the logical robot pose as the configured HOME pose without moving the servos
 * @param  robot: initialized robot descriptor
 * @retval ROBOT_ARM_OK: runtime pose updated successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot invalid
 */
robot_arm_status_t robot_arm_assume_home(robot_arm_t *robot);

/**
 * @brief  Calculate the pulse width that one robot joint command would write through the current runtime mapping
 * @param  robot: initialized robot descriptor
 * @param  joint_id: logical joint identifier
 * @param  angle_rad: requested logical angle in radians
 * @param  pulse_width_us: destination for the resulting pulse width in microseconds
 * @retval ROBOT_ARM_OK: pulse width calculated successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot, joint identifier, or output pointer invalid
 */
robot_arm_status_t robot_arm_calculate_joint_pulse_width_us(
    const robot_arm_t *robot,
    robot_arm_joint_id_t joint_id,
    float angle_rad,
    uint16_t *pulse_width_us);

robot_arm_status_t robot_arm_restore_pose_from_pulse_widths(
    robot_arm_t *robot,
    const uint16_t pulse_width_us_by_joint[ROBOT_ARM_JOINT_COUNT]);

/**
 * @brief  Immediately apply a direct joint-space pose to all six robot servos
 * @param  robot: initialized robot descriptor
 * @param  pose: requested direct pose in radians
 * @retval ROBOT_ARM_OK: pose applied successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot or pose invalid
 * @retval ROBOT_ARM_ERR_SERVO: one of the underlying servo writes failed
 */
robot_arm_status_t robot_arm_set_pose_immediate(robot_arm_t *robot, const robot_arm_pose_t *pose);

/**
 * @brief  Immediately apply the configured baseline HOME pose
 * @param  robot: initialized robot descriptor
 * @retval ROBOT_ARM_OK: HOME pose applied successfully
 * @retval ROBOT_ARM_ERR_INVALID_ARGUMENT: robot invalid
 * @retval ROBOT_ARM_ERR_SERVO: one of the underlying servo writes failed
 */
robot_arm_status_t robot_arm_home(robot_arm_t *robot);

#endif