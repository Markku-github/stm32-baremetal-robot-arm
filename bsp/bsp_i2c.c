/**
 ******************************************************************************
 * @file    bsp_i2c.c
 * @brief   Minimal blocking I2C driver implementation for STM32F767 bring-up
 ******************************************************************************
 */

#include "bsp_i2c.h"

#include <stdbool.h>

#include "stm32f767_registers.h"

#define BSP_I2C_POLL_TIMEOUT_CYCLES 1000000U

static bool bsp_i2c_is_supported_instance(bsp_i2c_instance_t instance)
{
    return instance == BSP_I2C_INSTANCE_I2C1;
}

static stm32_i2c_registers_t *bsp_i2c_registers(bsp_i2c_instance_t instance)
{
    if (instance == BSP_I2C_INSTANCE_I2C1)
    {
        return I2C1;
    }

    return 0;
}

static void bsp_i2c_enable_clock(bsp_i2c_instance_t instance)
{
    if (instance == BSP_I2C_INSTANCE_I2C1)
    {
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    }
}

static void bsp_i2c_clear_status_flags(stm32_i2c_registers_t *i2c)
{
    i2c->ICR = I2C_ICR_ADDRCF
        | I2C_ICR_NACKCF
        | I2C_ICR_STOPCF
        | I2C_ICR_BERRCF
        | I2C_ICR_ARLOCF
        | I2C_ICR_OVRCF
        | I2C_ICR_TIMOUTCF;
}

static void bsp_i2c_request_stop(stm32_i2c_registers_t *i2c)
{
    if ((i2c->ISR & I2C_ISR_BUSY) != 0U)
    {
        i2c->CR2 |= I2C_CR2_STOP;
    }
}

static bsp_i2c_status_t bsp_i2c_check_error_flags(stm32_i2c_registers_t *i2c)
{
    const uint32_t isr = i2c->ISR;

    if ((isr & I2C_ISR_NACKF) != 0U)
    {
        bsp_i2c_request_stop(i2c);
        bsp_i2c_clear_status_flags(i2c);
        return BSP_I2C_ERR_NACK;
    }

    if ((isr & (I2C_ISR_BERR | I2C_ISR_ARLO | I2C_ISR_OVR | I2C_ISR_TIMEOUT)) != 0U)
    {
        bsp_i2c_request_stop(i2c);
        bsp_i2c_clear_status_flags(i2c);
        return BSP_I2C_ERR_BUS;
    }

    return BSP_I2C_OK;
}

static bsp_i2c_status_t bsp_i2c_wait_for_flag(stm32_i2c_registers_t *i2c, uint32_t flag_mask)
{
    uint32_t timeout = BSP_I2C_POLL_TIMEOUT_CYCLES;

    while ((i2c->ISR & flag_mask) == 0U)
    {
        const bsp_i2c_status_t error_status = bsp_i2c_check_error_flags(i2c);

        if (error_status != BSP_I2C_OK)
        {
            return error_status;
        }

        if (timeout == 0U)
        {
            return BSP_I2C_ERR_TIMEOUT;
        }

        timeout--;
    }

    return BSP_I2C_OK;
}

static bsp_i2c_status_t bsp_i2c_wait_until_idle(stm32_i2c_registers_t *i2c)
{
    uint32_t timeout = BSP_I2C_POLL_TIMEOUT_CYCLES;

    while ((i2c->ISR & I2C_ISR_BUSY) != 0U)
    {
        const bsp_i2c_status_t error_status = bsp_i2c_check_error_flags(i2c);

        if (error_status != BSP_I2C_OK)
        {
            return error_status;
        }

        if (timeout == 0U)
        {
            return BSP_I2C_ERR_TIMEOUT;
        }

        timeout--;
    }

    return BSP_I2C_OK;
}

static void bsp_i2c_begin_transfer(
    stm32_i2c_registers_t *i2c,
    uint8_t address,
    uint8_t length,
    bool read,
    bool auto_end)
{
    uint32_t transfer = (((uint32_t)address << 1U) & I2C_CR2_SADD_MASK)
        | (((uint32_t)length << I2C_CR2_NBYTES_SHIFT) & I2C_CR2_NBYTES_MASK)
        | I2C_CR2_START;

    if (read)
    {
        transfer |= I2C_CR2_RD_WRN;
    }

    if (auto_end)
    {
        transfer |= I2C_CR2_AUTOEND;
    }

    i2c->CR2 = transfer;
}

bsp_i2c_status_t bsp_i2c_init(const bsp_i2c_config_t *config)
{
    stm32_i2c_registers_t *i2c;
    bsp_gpio_status_t gpio_status;

    if ((config == 0) || !bsp_i2c_is_supported_instance(config->instance) || (config->timing == 0U))
    {
        return BSP_I2C_ERR_INVALID_ARGUMENT;
    }

    gpio_status = bsp_gpio_init_alternate_function(&config->scl_pin);
    if (gpio_status != BSP_GPIO_OK)
    {
        return BSP_I2C_ERR_INVALID_ARGUMENT;
    }

    gpio_status = bsp_gpio_init_alternate_function(&config->sda_pin);
    if (gpio_status != BSP_GPIO_OK)
    {
        return BSP_I2C_ERR_INVALID_ARGUMENT;
    }

    bsp_i2c_enable_clock(config->instance);
    i2c = bsp_i2c_registers(config->instance);
    if (i2c == 0)
    {
        return BSP_I2C_ERR_UNSUPPORTED_INSTANCE;
    }

    i2c->CR1 &= ~I2C_CR1_PE;
    i2c->CR2 = 0U;
    i2c->OAR1 = 0U;
    i2c->OAR2 = 0U;
    i2c->TIMINGR = config->timing;
    i2c->TIMEOUTR = 0U;
    bsp_i2c_clear_status_flags(i2c);
    i2c->CR1 = I2C_CR1_PE;

    return BSP_I2C_OK;
}

bsp_i2c_status_t bsp_i2c_write(bsp_i2c_instance_t instance, uint8_t address, const uint8_t *data, uint8_t length)
{
    stm32_i2c_registers_t *i2c = bsp_i2c_registers(instance);
    uint8_t index;
    bsp_i2c_status_t status;

    if ((i2c == 0) || (address > 0x7FU) || (data == 0) || (length == 0U))
    {
        return (i2c == 0) ? BSP_I2C_ERR_UNSUPPORTED_INSTANCE : BSP_I2C_ERR_INVALID_ARGUMENT;
    }

    status = bsp_i2c_wait_until_idle(i2c);
    if (status != BSP_I2C_OK)
    {
        return status;
    }

    bsp_i2c_clear_status_flags(i2c);
    bsp_i2c_begin_transfer(i2c, address, length, false, true);

    for (index = 0U; index < length; index++)
    {
        status = bsp_i2c_wait_for_flag(i2c, I2C_ISR_TXIS);
        if (status != BSP_I2C_OK)
        {
            return status;
        }

        i2c->TXDR = data[index];
    }

    status = bsp_i2c_wait_for_flag(i2c, I2C_ISR_STOPF);
    if (status != BSP_I2C_OK)
    {
        return status;
    }

    i2c->ICR = I2C_ICR_STOPCF;

    return BSP_I2C_OK;
}

bsp_i2c_status_t bsp_i2c_write_read(
    bsp_i2c_instance_t instance,
    uint8_t address,
    const uint8_t *write_data,
    uint8_t write_length,
    uint8_t *read_data,
    uint8_t read_length)
{
    stm32_i2c_registers_t *i2c = bsp_i2c_registers(instance);
    uint8_t index;
    bsp_i2c_status_t status;

    if ((i2c == 0) || (address > 0x7FU) || (write_data == 0) || (write_length == 0U) || (read_data == 0) || (read_length == 0U))
    {
        return (i2c == 0) ? BSP_I2C_ERR_UNSUPPORTED_INSTANCE : BSP_I2C_ERR_INVALID_ARGUMENT;
    }

    status = bsp_i2c_wait_until_idle(i2c);
    if (status != BSP_I2C_OK)
    {
        return status;
    }

    bsp_i2c_clear_status_flags(i2c);
    bsp_i2c_begin_transfer(i2c, address, write_length, false, false);

    for (index = 0U; index < write_length; index++)
    {
        status = bsp_i2c_wait_for_flag(i2c, I2C_ISR_TXIS);
        if (status != BSP_I2C_OK)
        {
            return status;
        }

        i2c->TXDR = write_data[index];
    }

    status = bsp_i2c_wait_for_flag(i2c, I2C_ISR_TC);
    if (status != BSP_I2C_OK)
    {
        return status;
    }

    bsp_i2c_begin_transfer(i2c, address, read_length, true, true);

    for (index = 0U; index < read_length; index++)
    {
        status = bsp_i2c_wait_for_flag(i2c, I2C_ISR_RXNE);
        if (status != BSP_I2C_OK)
        {
            return status;
        }

        read_data[index] = (uint8_t)i2c->RXDR;
    }

    status = bsp_i2c_wait_for_flag(i2c, I2C_ISR_STOPF);
    if (status != BSP_I2C_OK)
    {
        return status;
    }

    i2c->ICR = I2C_ICR_STOPCF;

    return BSP_I2C_OK;
}