/**
 ******************************************************************************
 * @file    servo.c
 * @brief   Servo abstraction implementation on top of the PCA9685 driver
 ******************************************************************************
 */

#include <stdbool.h>

#include "servo.h"

#define SERVO_PI_F 3.14159265358979323846f
#define SERVO_RAD_PER_DEGREE (SERVO_PI_F / 180.0f)

static bool servo_is_valid_runtime(const servo_t *servo)
{
    return (servo != 0)
        && (servo->device != 0)
        && (servo->name != 0)
        && (servo->channel < PCA9685_CHANNEL_COUNT)
        && (servo->minimum_angle_rad < servo->maximum_angle_rad)
    && (servo->minimum_pulse_width_us != servo->maximum_pulse_width_us);
}

static servo_status_t servo_map_pca9685_status(pca9685_status_t status)
{
    if (status == PCA9685_OK)
    {
        return SERVO_OK;
    }

    if (status == PCA9685_ERR_INVALID_ARGUMENT)
    {
        return SERVO_ERR_INVALID_ARGUMENT;
    }

    return SERVO_ERR_PCA9685;
}

servo_status_t servo_init(servo_t *servo, const servo_config_t *config, const pca9685_device_t *device)
{
    float initial_angle_rad;

    if ((servo == 0)
        || (config == 0)
        || (device == 0)
        || (config->name == 0)
        || (config->channel >= PCA9685_CHANNEL_COUNT)
        || (config->minimum_angle_rad >= config->maximum_angle_rad)
        || (config->minimum_pulse_width_us == config->maximum_pulse_width_us))
    {
        return SERVO_ERR_INVALID_ARGUMENT;
    }

    servo->device = device;
    servo->name = config->name;
    servo->channel = config->channel;
    servo->minimum_angle_rad = config->minimum_angle_rad;
    servo->maximum_angle_rad = config->maximum_angle_rad;
    servo->offset_rad = config->offset_rad;
    servo->minimum_pulse_width_us = config->minimum_pulse_width_us;
    servo->maximum_pulse_width_us = config->maximum_pulse_width_us;

    initial_angle_rad = 0.0f;
    if (initial_angle_rad < servo->minimum_angle_rad)
    {
        initial_angle_rad = servo->minimum_angle_rad;
    }
    else if (initial_angle_rad > servo->maximum_angle_rad)
    {
        initial_angle_rad = servo->maximum_angle_rad;
    }

    servo->current_angle_rad = initial_angle_rad;
    servo->target_angle_rad = initial_angle_rad;

    return SERVO_OK;
}

servo_status_t servo_clamp_angle_rad(const servo_t *servo, float angle_rad, float *clamped_angle_rad)
{
    if ((clamped_angle_rad == 0) || !servo_is_valid_runtime(servo))
    {
        return SERVO_ERR_INVALID_ARGUMENT;
    }

    if (angle_rad < servo->minimum_angle_rad)
    {
        *clamped_angle_rad = servo->minimum_angle_rad;
        return SERVO_OK;
    }

    if (angle_rad > servo->maximum_angle_rad)
    {
        *clamped_angle_rad = servo->maximum_angle_rad;
        return SERVO_OK;
    }

    *clamped_angle_rad = angle_rad;
    return SERVO_OK;
}

servo_status_t servo_angle_rad_to_pulse_us(const servo_t *servo, float angle_rad, uint16_t *pulse_width_us)
{
    float clamped_angle_rad;
    float adjusted_angle_rad;
    float normalized_position;
    float pulse_width_f;
    float angle_range_rad;
    float pulse_range_us;
    float pulse_min_endpoint_us;
    float pulse_max_endpoint_us;
    float pulse_low_us;
    float pulse_high_us;

    if ((pulse_width_us == 0) || !servo_is_valid_runtime(servo))
    {
        return SERVO_ERR_INVALID_ARGUMENT;
    }

    if (servo_clamp_angle_rad(servo, angle_rad, &clamped_angle_rad) != SERVO_OK)
    {
        return SERVO_ERR_INVALID_ARGUMENT;
    }

    adjusted_angle_rad = clamped_angle_rad + servo->offset_rad;
    if (adjusted_angle_rad < servo->minimum_angle_rad)
    {
        adjusted_angle_rad = servo->minimum_angle_rad;
    }
    else if (adjusted_angle_rad > servo->maximum_angle_rad)
    {
        adjusted_angle_rad = servo->maximum_angle_rad;
    }

    angle_range_rad = servo->maximum_angle_rad - servo->minimum_angle_rad;
    pulse_min_endpoint_us = (float)servo->minimum_pulse_width_us;
    pulse_max_endpoint_us = (float)servo->maximum_pulse_width_us;
    pulse_range_us = pulse_max_endpoint_us - pulse_min_endpoint_us;
    normalized_position = (adjusted_angle_rad - servo->minimum_angle_rad) / angle_range_rad;
    pulse_width_f = pulse_min_endpoint_us + (normalized_position * pulse_range_us);

    pulse_low_us = pulse_min_endpoint_us;
    if (pulse_max_endpoint_us < pulse_low_us)
    {
        pulse_low_us = pulse_max_endpoint_us;
    }

    pulse_high_us = pulse_max_endpoint_us;
    if (pulse_min_endpoint_us > pulse_high_us)
    {
        pulse_high_us = pulse_min_endpoint_us;
    }

    if (pulse_width_f < pulse_low_us)
    {
        pulse_width_f = pulse_low_us;
    }
    else if (pulse_width_f > pulse_high_us)
    {
        pulse_width_f = pulse_high_us;
    }

    *pulse_width_us = (uint16_t)(pulse_width_f + 0.5f);
    return SERVO_OK;
}

servo_status_t servo_set_angle_immediate_rad(servo_t *servo, float angle_rad)
{
    float clamped_angle_rad;
    uint16_t pulse_width_us;
    const servo_status_t conversion_status = servo_angle_rad_to_pulse_us(servo, angle_rad, &pulse_width_us);

    if (conversion_status != SERVO_OK)
    {
        return conversion_status;
    }

    if (servo_clamp_angle_rad(servo, angle_rad, &clamped_angle_rad) != SERVO_OK)
    {
        return SERVO_ERR_INVALID_ARGUMENT;
    }

    if (servo_map_pca9685_status(pca9685_set_channel_pulse_us(servo->device, servo->channel, pulse_width_us)) != SERVO_OK)
    {
        return SERVO_ERR_PCA9685;
    }

    servo->current_angle_rad = clamped_angle_rad;
    servo->target_angle_rad = clamped_angle_rad;

    return SERVO_OK;
}

servo_status_t servo_set_angle_immediate_deg(servo_t *servo, float angle_deg)
{
    return servo_set_angle_immediate_rad(servo, angle_deg * SERVO_RAD_PER_DEGREE);
}