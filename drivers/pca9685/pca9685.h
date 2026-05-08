/**
 ******************************************************************************
 * @file    pca9685.h
 * @brief   Minimal PCA9685 register access interface for V0 communication tests
 ******************************************************************************
 */

#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

#include "bsp_i2c.h"

#define PCA9685_I2C_ADDRESS_DEFAULT 0x40U
#define PCA9685_REGISTER_MODE1 0x00U

typedef enum
{
    PCA9685_OK = 0,
    PCA9685_ERR_INVALID_ARGUMENT,
    PCA9685_ERR_I2C,
} pca9685_status_t;

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