/**
 ******************************************************************************
 * @file    bsp_i2c.h
 * @brief   Minimal blocking I2C driver interface for STM32F767 bring-up
 ******************************************************************************
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>

#include "bsp_gpio.h"

typedef enum
{
    BSP_I2C_OK = 0,
    BSP_I2C_ERR_INVALID_ARGUMENT,
    BSP_I2C_ERR_UNSUPPORTED_INSTANCE,
    BSP_I2C_ERR_TIMEOUT,
    BSP_I2C_ERR_NACK,
    BSP_I2C_ERR_BUS,
} bsp_i2c_status_t;

typedef enum
{
    BSP_I2C_INSTANCE_I2C1 = 0,
} bsp_i2c_instance_t;

typedef struct
{
    bsp_i2c_instance_t instance;
    uint32_t timing;
    bsp_gpio_alternate_function_config_t scl_pin;
    bsp_gpio_alternate_function_config_t sda_pin;
} bsp_i2c_config_t;

/**
 * @brief  Initialize a supported I2C peripheral and its GPIO pins
 * @param  config: pointer to the I2C configuration structure
 * @retval BSP_I2C_OK: I2C initialized successfully
 * @retval BSP_I2C_ERR_INVALID_ARGUMENT: configuration pointer or values invalid
 * @retval BSP_I2C_ERR_UNSUPPORTED_INSTANCE: requested I2C instance unsupported
 */
bsp_i2c_status_t bsp_i2c_init(const bsp_i2c_config_t *config);

/**
 * @brief  Write one or more bytes to a 7-bit I2C device address
 * @param  instance: I2C instance to use
 * @param  address: 7-bit slave address
 * @param  data: pointer to the bytes to send
 * @param  length: number of bytes to send, limited to one hardware transfer
 * @retval BSP_I2C_OK: transfer completed successfully
 * @retval BSP_I2C_ERR_INVALID_ARGUMENT: address, buffer, or length invalid
 * @retval BSP_I2C_ERR_UNSUPPORTED_INSTANCE: requested I2C instance unsupported
 * @retval BSP_I2C_ERR_TIMEOUT: bus did not reach the required state in time
 * @retval BSP_I2C_ERR_NACK: slave did not acknowledge the transfer
 * @retval BSP_I2C_ERR_BUS: arbitration, overrun, or bus error detected
 */
bsp_i2c_status_t bsp_i2c_write(bsp_i2c_instance_t instance, uint8_t address, const uint8_t *data, uint8_t length);

/**
 * @brief  Write a short command phase and then read bytes from a 7-bit I2C slave
 * @param  instance: I2C instance to use
 * @param  address: 7-bit slave address
 * @param  write_data: bytes to send before the repeated start
 * @param  write_length: number of bytes in the write phase
 * @param  read_data: destination for the received bytes
 * @param  read_length: number of bytes to read after the repeated start
 * @retval BSP_I2C_OK: transfer completed successfully
 * @retval BSP_I2C_ERR_INVALID_ARGUMENT: address, buffer, or length invalid
 * @retval BSP_I2C_ERR_UNSUPPORTED_INSTANCE: requested I2C instance unsupported
 * @retval BSP_I2C_ERR_TIMEOUT: bus did not reach the required state in time
 * @retval BSP_I2C_ERR_NACK: slave did not acknowledge the transfer
 * @retval BSP_I2C_ERR_BUS: arbitration, overrun, or bus error detected
 */
bsp_i2c_status_t bsp_i2c_write_read(
    bsp_i2c_instance_t instance,
    uint8_t address,
    const uint8_t *write_data,
    uint8_t write_length,
    uint8_t *read_data,
    uint8_t read_length);

#endif