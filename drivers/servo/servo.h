/**
 ******************************************************************************
 * @file    servo.h
 * @brief   Servo abstraction layer on top of the PCA9685 driver
 ******************************************************************************
 */

#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

#include "pca9685.h"

typedef enum
{
    SERVO_OK = 0,
    SERVO_ERR_INVALID_ARGUMENT,
    SERVO_ERR_PCA9685,
} servo_status_t;

typedef struct
{
    const char *name;
    uint8_t channel;
    float minimum_angle_rad;
    float maximum_angle_rad;
    float offset_rad;
    uint16_t minimum_pulse_width_us;
    uint16_t maximum_pulse_width_us;
} servo_config_t;

typedef struct
{
    const pca9685_device_t *device;
    const char *name;
    uint8_t channel;
    float minimum_angle_rad;
    float maximum_angle_rad;
    float offset_rad;
    uint16_t minimum_pulse_width_us;
    uint16_t maximum_pulse_width_us;
    float current_angle_rad;
    float target_angle_rad;
} servo_t;

/**
 * @brief  Initialize one servo descriptor on top of a PCA9685 device
 * @param  servo: destination for the servo runtime state
 * @param  config: static servo calibration and channel configuration; pulse endpoints map to the
 *                 logical minimum and maximum angles and may be reversed for inverted mechanisms
 * @param  device: initialized PCA9685 device used to drive this servo
 * @retval SERVO_OK: servo descriptor initialized successfully
 * @retval SERVO_ERR_INVALID_ARGUMENT: servo, config, or device invalid
 */
servo_status_t servo_init(servo_t *servo, const servo_config_t *config, const pca9685_device_t *device);

/**
 * @brief  Clamp a requested servo angle to the configured logical range
 * @param  servo: initialized servo descriptor
 * @param  angle_rad: requested logical angle in radians
 * @param  clamped_angle_rad: destination for the clamped angle value
 * @retval SERVO_OK: angle was clamped successfully
 * @retval SERVO_ERR_INVALID_ARGUMENT: servo or output pointer invalid
 */
servo_status_t servo_clamp_angle_rad(const servo_t *servo, float angle_rad, float *clamped_angle_rad);

/**
 * @brief  Convert one logical servo angle in radians to a pulse width
 * @param  servo: initialized servo descriptor
 * @param  angle_rad: requested logical angle in radians
 * @param  pulse_width_us: destination for the pulse width in microseconds
 * @retval SERVO_OK: pulse width computed successfully
 * @retval SERVO_ERR_INVALID_ARGUMENT: servo or output pointer invalid
 */
servo_status_t servo_angle_rad_to_pulse_us(const servo_t *servo, float angle_rad, uint16_t *pulse_width_us);

/**
 * @brief  Immediately write one logical angle in radians to the PCA9685 channel
 * @param  servo: initialized servo descriptor
 * @param  angle_rad: requested logical angle in radians
 * @retval SERVO_OK: pulse width update completed successfully
 * @retval SERVO_ERR_INVALID_ARGUMENT: servo invalid or angle conversion failed
 * @retval SERVO_ERR_PCA9685: underlying PCA9685 transaction failed
 */
servo_status_t servo_set_angle_immediate_rad(servo_t *servo, float angle_rad);

/**
 * @brief  Immediately write one logical angle in degrees to the PCA9685 channel
 * @param  servo: initialized servo descriptor
 * @param  angle_deg: requested logical angle in degrees
 * @retval SERVO_OK: pulse width update completed successfully
 * @retval SERVO_ERR_INVALID_ARGUMENT: servo invalid or angle conversion failed
 * @retval SERVO_ERR_PCA9685: underlying PCA9685 transaction failed
 */
servo_status_t servo_set_angle_immediate_deg(servo_t *servo, float angle_deg);

#endif