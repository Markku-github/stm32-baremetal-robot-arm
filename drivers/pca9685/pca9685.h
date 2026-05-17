/**
 ******************************************************************************
 * @file    pca9685.h
 * @brief   PCA9685 register access and PWM control interface
 ******************************************************************************
 */

#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

#include "bsp_i2c.h"

#define PCA9685_I2C_ADDRESS_DEFAULT 0x40U
#define PCA9685_CHANNEL_COUNT 16U
#define PCA9685_PWM_STEPS 4096U
#define PCA9685_OSCILLATOR_FREQUENCY_HZ 25000000UL
#define PCA9685_PWM_FREQUENCY_MIN_HZ 24U
#define PCA9685_PWM_FREQUENCY_MAX_HZ 1526U

#define PCA9685_REGISTER_MODE1 0x00U
#define PCA9685_REGISTER_MODE2 0x01U
#define PCA9685_REGISTER_LED0_ON_L 0x06U
#define PCA9685_REGISTER_PRESCALE 0xFEU

typedef enum
{
    PCA9685_OK = 0,
    PCA9685_ERR_INVALID_ARGUMENT,
    PCA9685_ERR_STATE,
    PCA9685_ERR_I2C,
} pca9685_status_t;

typedef struct
{
    bsp_i2c_instance_t instance;
    uint8_t address;
    uint32_t oscillator_frequency_hz;
    uint16_t pwm_frequency_hz;
} pca9685_device_t;

/**
 * @brief  Initialize a PCA9685 device descriptor and basic control registers
 * @param  device: destination for the PCA9685 device state
 * @param  instance: I2C instance connected to the PCA9685 device
 * @param  address: 7-bit PCA9685 slave address
 * @retval PCA9685_OK: device responded and control registers were configured
 * @retval PCA9685_ERR_INVALID_ARGUMENT: device pointer or address invalid
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_init(
    pca9685_device_t *device,
    bsp_i2c_instance_t instance,
    uint8_t address);

/**
 * @brief  Configure the PCA9685 PWM update frequency
 * @param  device: initialized PCA9685 device descriptor
 * @param  pwm_frequency_hz: requested PWM frequency in hertz
 * @retval PCA9685_OK: frequency was configured successfully
 * @retval PCA9685_ERR_INVALID_ARGUMENT: device invalid or frequency out of range
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_set_pwm_frequency(pca9685_device_t *device, uint16_t pwm_frequency_hz);

/**
 * @brief  Program raw PWM counter values for one PCA9685 output channel
 * @param  device: initialized PCA9685 device descriptor
 * @param  channel: output channel index from 0 to 15
 * @param  on_count: counter value where the pulse turns on
 * @param  off_count: counter value where the pulse turns off
 * @retval PCA9685_OK: channel registers updated successfully
 * @retval PCA9685_ERR_INVALID_ARGUMENT: device, channel, or counter values invalid
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_set_channel_pwm(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t on_count,
    uint16_t off_count);

/**
 * @brief  Read raw PWM counter values from one PCA9685 output channel
 * @param  device: initialized PCA9685 device descriptor
 * @param  channel: output channel index from 0 to 15
 * @param  on_count: destination for the turn-on counter value
 * @param  off_count: destination for the turn-off counter value
 * @retval PCA9685_OK: channel registers were read successfully
 * @retval PCA9685_ERR_INVALID_ARGUMENT: device, channel, or output pointers invalid
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_read_channel_pwm(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t *on_count,
    uint16_t *off_count);

/**
 * @brief  Program one PCA9685 output pulse width in microseconds
 * @param  device: initialized PCA9685 device descriptor with a configured PWM frequency
 * @param  channel: output channel index from 0 to 15
 * @param  pulse_width_us: requested pulse width in microseconds
 * @retval PCA9685_OK: pulse width updated successfully
 * @retval PCA9685_ERR_INVALID_ARGUMENT: device, channel, or pulse width invalid
 * @retval PCA9685_ERR_STATE: PWM frequency has not been configured yet
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_set_channel_pulse_us(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t pulse_width_us);

/**
 * @brief  Program one PCA9685 output pulse width while keeping the channel in full-off state
 * @param  device: initialized PCA9685 device descriptor with a configured PWM frequency
 * @param  channel: output channel index from 0 to 15
 * @param  pulse_width_us: requested pulse width in microseconds
 * @retval PCA9685_OK: register values updated successfully while the output remained disabled
 * @retval PCA9685_ERR_INVALID_ARGUMENT: device, channel, or pulse width invalid
 * @retval PCA9685_ERR_STATE: PWM frequency has not been configured yet
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_set_channel_pulse_us_disabled(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t pulse_width_us);

/**
 * @brief  Force all PCA9685 outputs into a disabled full-off state
 * @param  device: initialized PCA9685 device descriptor
 * @retval PCA9685_OK: all output channels were disabled successfully
 * @retval PCA9685_ERR_INVALID_ARGUMENT: device descriptor invalid
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_disable_all_outputs(const pca9685_device_t *device);

/**
 * @brief  Write one PCA9685 register through the BSP I2C layer
 * @param  instance: I2C instance connected to the PCA9685 device
 * @param  address: 7-bit PCA9685 slave address
 * @param  register_address: device register to write
 * @param  value: byte to write into the register
 * @retval PCA9685_OK: register write completed successfully
 * @retval PCA9685_ERR_INVALID_ARGUMENT: address invalid or instance unsupported
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_write_register(
    bsp_i2c_instance_t instance,
    uint8_t address,
    uint8_t register_address,
    uint8_t value);

/**
 * @brief  Read one PCA9685 register through the BSP I2C layer
 * @param  instance: I2C instance connected to the PCA9685 device
 * @param  address: 7-bit PCA9685 slave address
 * @param  register_address: device register to read
 * @param  value: destination for the register value
 * @retval PCA9685_OK: register read completed successfully
 * @retval PCA9685_ERR_INVALID_ARGUMENT: address or output pointer invalid
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_read_register(
    bsp_i2c_instance_t instance,
    uint8_t address,
    uint8_t register_address,
    uint8_t *value);

/**
 * @brief  Read the MODE1 register as a minimal communication smoke test
 * @param  instance: I2C instance connected to the PCA9685 device
 * @param  address: 7-bit PCA9685 slave address
 * @param  mode1_value: destination for the MODE1 register contents
 * @retval PCA9685_OK: MODE1 was read successfully
 * @retval PCA9685_ERR_INVALID_ARGUMENT: address or output pointer invalid
 * @retval PCA9685_ERR_I2C: underlying I2C transaction failed
 */
pca9685_status_t pca9685_probe(bsp_i2c_instance_t instance, uint8_t address, uint8_t *mode1_value);

#endif