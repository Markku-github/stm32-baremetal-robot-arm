/**
 ******************************************************************************
 * @file    pca9685.c
 * @brief   Minimal PCA9685 register access implementation for V0 smoke tests
 ******************************************************************************
 */

#include "pca9685.h"

static bool pca9685_is_valid_address(uint8_t address)
{
    return address <= 0x7FU;
}

static pca9685_status_t pca9685_map_i2c_status(bsp_i2c_status_t status)
{
    if (status == BSP_I2C_OK)
    {
        return PCA9685_OK;
    }

    if ((status == BSP_I2C_ERR_INVALID_ARGUMENT) || (status == BSP_I2C_ERR_UNSUPPORTED_INSTANCE))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    return PCA9685_ERR_I2C;
}

pca9685_status_t pca9685_write_register(
    bsp_i2c_instance_t instance,
    uint8_t address,
    uint8_t register_address,
    uint8_t value)
{
    const uint8_t transaction[2] = { register_address, value };

    if (!pca9685_is_valid_address(address))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    return pca9685_map_i2c_status(bsp_i2c_write(instance, address, transaction, (uint8_t)sizeof(transaction)));
}

pca9685_status_t pca9685_read_register(
    bsp_i2c_instance_t instance,
    uint8_t address,
    uint8_t register_address,
    uint8_t *value)
{
    if ((value == 0) || !pca9685_is_valid_address(address))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    return pca9685_map_i2c_status(bsp_i2c_write_read(instance, address, &register_address, 1U, value, 1U));
}

pca9685_status_t pca9685_probe(bsp_i2c_instance_t instance, uint8_t address, uint8_t *mode1_value)
{
    return pca9685_read_register(instance, address, PCA9685_REGISTER_MODE1, mode1_value);
}