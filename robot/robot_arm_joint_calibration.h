/**
 ******************************************************************************
 * @file    robot_arm_joint_calibration.h
 * @brief   Robot-specific joint calibration data and servo-config builders
 ******************************************************************************
 */

#ifndef ROBOT_ARM_JOINT_CALIBRATION_H
#define ROBOT_ARM_JOINT_CALIBRATION_H

#include <stdbool.h>
#include <stdint.h>

#include "robot_arm.h"

typedef struct
{
    const char *name;
    uint8_t channel;
    float minimum_angle_rad;
    float maximum_angle_rad;
    float home_angle_rad;
    float offset_rad;
    uint16_t pulse_width_at_min_angle_us;
    uint16_t pulse_width_at_max_angle_us;
} robot_arm_joint_calibration_t;

/**
 * @brief  Return the calibration descriptor for one robot joint
 * @param  joint_id: logical joint identifier
 * @retval const robot_arm_joint_calibration_t*: matching calibration descriptor
 * @retval 0: joint identifier invalid
 */
const robot_arm_joint_calibration_t *robot_arm_joint_calibration_get(robot_arm_joint_id_t joint_id);

/**
 * @brief  Build one generic servo configuration from one robot calibration descriptor
 * @param  calibration: source robot calibration descriptor
 * @param  config: destination generic servo configuration
 * @retval None
 */
void robot_arm_joint_calibration_build_servo_config(
    const robot_arm_joint_calibration_t *calibration,
    servo_config_t *config);

/**
 * @brief  Return the current shoulder midpoint angle used by the piecewise shoulder mapping
 * @retval float: shoulder midpoint angle in radians
 */
float robot_arm_joint_calibration_shoulder_mid_angle_rad(void);

/**
 * @brief  Return the current shoulder midpoint pulse width used by the piecewise shoulder mapping
 * @retval uint16_t: shoulder midpoint pulse width in microseconds
 */
uint16_t robot_arm_joint_calibration_shoulder_mid_pulse_us(void);

#endif