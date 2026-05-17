/**
 ******************************************************************************
 * @file    pca9685.c
 * @brief   PCA9685 register access and PWM control implementation
 ******************************************************************************
 */

#include <stdbool.h>

#include "pca9685.h"

#define PCA9685_MODE1_RESTART 0x80U
#define PCA9685_MODE1_AI 0x20U
#define PCA9685_MODE1_SLEEP 0x10U
#define PCA9685_MODE2_OUTDRV 0x04U
#define PCA9685_CHANNEL_REGISTER_STRIDE 4U
#define PCA9685_FULL_OFF_BIT 0x10U
#define PCA9685_WAKE_DELAY_CYCLES 20000U

static void pca9685_delay_cycles(volatile uint32_t cycles)
{
    while (cycles > 0U)
    {
        cycles--;
    }
}

static bool pca9685_is_valid_address(uint8_t address)
{
    return address <= 0x7FU;
}

static bool pca9685_is_valid_device(const pca9685_device_t *device)
{
    return (device != 0) && pca9685_is_valid_address(device->address) && (device->oscillator_frequency_hz != 0U);
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

static uint8_t pca9685_channel_register_base(uint8_t channel)
{
    return (uint8_t)(PCA9685_REGISTER_LED0_ON_L + (channel * PCA9685_CHANNEL_REGISTER_STRIDE));
}

static pca9685_status_t pca9685_write_channel_registers(
    bsp_i2c_instance_t instance,
    uint8_t address,
    uint8_t register_base,
    uint16_t on_count,
    uint16_t off_count,
    bool full_off)
{
    uint8_t off_high_byte = (uint8_t)((off_count >> 8U) & 0x0FU);
    pca9685_status_t status;

    if (full_off)
    {
        off_high_byte |= PCA9685_FULL_OFF_BIT;
    }

    status = pca9685_write_register(instance, address, register_base, (uint8_t)(on_count & 0x00FFU));
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_write_register(instance, address, (uint8_t)(register_base + 1U), (uint8_t)((on_count >> 8U) & 0x0FU));
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_write_register(instance, address, (uint8_t)(register_base + 2U), (uint8_t)(off_count & 0x00FFU));
    if (status != PCA9685_OK)
    {
        return status;
    }

    return pca9685_write_register(instance, address, (uint8_t)(register_base + 3U), off_high_byte);
}

pca9685_status_t pca9685_init(
    pca9685_device_t *device,
    bsp_i2c_instance_t instance,
    uint8_t address)
{
    uint8_t mode1_value;
    pca9685_status_t status;

    if ((device == 0) || !pca9685_is_valid_address(address))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    device->instance = instance;
    device->address = address;
    device->oscillator_frequency_hz = PCA9685_OSCILLATOR_FREQUENCY_HZ;
    device->pwm_frequency_hz = 0U;

    status = pca9685_read_register(instance, address, PCA9685_REGISTER_MODE1, &mode1_value);
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_write_register(instance, address, PCA9685_REGISTER_MODE2, PCA9685_MODE2_OUTDRV);
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_write_register(
        instance,
        address,
        PCA9685_REGISTER_MODE1,
        (uint8_t)(mode1_value | PCA9685_MODE1_AI));

    if (status != PCA9685_OK)
    {
        return status;
    }

    return pca9685_disable_all_outputs(device);
}

pca9685_status_t pca9685_set_pwm_frequency(pca9685_device_t *device, uint16_t pwm_frequency_hz)
{
    uint8_t mode1_value;
    uint8_t prescale;
    uint8_t sleep_mode;
    uint8_t run_mode;
    uint32_t divider;
    uint32_t prescale_divisor;
    pca9685_status_t status;

    if (!pca9685_is_valid_device(device)
        || (pwm_frequency_hz < PCA9685_PWM_FREQUENCY_MIN_HZ)
        || (pwm_frequency_hz > PCA9685_PWM_FREQUENCY_MAX_HZ))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    divider = (uint32_t)PCA9685_PWM_STEPS * (uint32_t)pwm_frequency_hz;
    prescale_divisor = (device->oscillator_frequency_hz + (divider / 2U)) / divider;

    if ((prescale_divisor < 4U) || (prescale_divisor > 256U))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    prescale = (uint8_t)(prescale_divisor - 1U);

    status = pca9685_read_register(device->instance, device->address, PCA9685_REGISTER_MODE1, &mode1_value);
    if (status != PCA9685_OK)
    {
        return status;
    }

    sleep_mode = (uint8_t)((mode1_value | PCA9685_MODE1_SLEEP | PCA9685_MODE1_AI) & (uint8_t)(~PCA9685_MODE1_RESTART));
    run_mode = (uint8_t)((mode1_value | PCA9685_MODE1_AI) & (uint8_t)(~PCA9685_MODE1_SLEEP));

    status = pca9685_write_register(device->instance, device->address, PCA9685_REGISTER_MODE1, sleep_mode);
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_write_register(device->instance, device->address, PCA9685_REGISTER_PRESCALE, prescale);
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_write_register(device->instance, device->address, PCA9685_REGISTER_MODE1, run_mode);
    if (status != PCA9685_OK)
    {
        return status;
    }

    pca9685_delay_cycles(PCA9685_WAKE_DELAY_CYCLES);

    status = pca9685_write_register(
        device->instance,
        device->address,
        PCA9685_REGISTER_MODE1,
        (uint8_t)(run_mode | PCA9685_MODE1_RESTART));
    if (status != PCA9685_OK)
    {
        return status;
    }

    device->pwm_frequency_hz = pwm_frequency_hz;

    return PCA9685_OK;
}

pca9685_status_t pca9685_set_channel_pwm(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t on_count,
    uint16_t off_count)
{
    uint8_t register_base;

    if (!pca9685_is_valid_device(device)
        || (channel >= PCA9685_CHANNEL_COUNT)
        || (on_count >= PCA9685_PWM_STEPS)
        || (off_count >= PCA9685_PWM_STEPS))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    register_base = pca9685_channel_register_base(channel);

    return pca9685_write_channel_registers(
        device->instance,
        device->address,
        register_base,
        on_count,
        off_count,
        false);
}

pca9685_status_t pca9685_read_channel_pwm(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t *on_count,
    uint16_t *off_count)
{
    uint8_t register_base;
    uint8_t led_on_low;
    uint8_t led_on_high;
    uint8_t led_off_low;
    uint8_t led_off_high;
    pca9685_status_t status;

    if (!pca9685_is_valid_device(device)
        || (channel >= PCA9685_CHANNEL_COUNT)
        || (on_count == 0)
        || (off_count == 0))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    register_base = pca9685_channel_register_base(channel);

    status = pca9685_read_register(device->instance, device->address, register_base, &led_on_low);
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_read_register(device->instance, device->address, (uint8_t)(register_base + 1U), &led_on_high);
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_read_register(device->instance, device->address, (uint8_t)(register_base + 2U), &led_off_low);
    if (status != PCA9685_OK)
    {
        return status;
    }

    status = pca9685_read_register(device->instance, device->address, (uint8_t)(register_base + 3U), &led_off_high);
    if (status != PCA9685_OK)
    {
        return status;
    }

    *on_count = (uint16_t)((((uint16_t)led_on_high & 0x0FU) << 8U) | led_on_low);
    *off_count = (uint16_t)((((uint16_t)led_off_high & 0x0FU) << 8U) | led_off_low);
    return PCA9685_OK;
}

pca9685_status_t pca9685_disable_all_outputs(const pca9685_device_t *device)
{
    uint8_t channel;

    if (!pca9685_is_valid_device(device))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    for (channel = 0U; channel < PCA9685_CHANNEL_COUNT; channel++)
    {
        const pca9685_status_t status = pca9685_write_channel_registers(
            device->instance,
            device->address,
            pca9685_channel_register_base(channel),
            0U,
            0U,
            true);

        if (status != PCA9685_OK)
        {
            return status;
        }
    }

    return PCA9685_OK;
}

pca9685_status_t pca9685_set_channel_pulse_us(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t pulse_width_us)
{
    uint64_t pulse_counts;

    if (!pca9685_is_valid_device(device) || (channel >= PCA9685_CHANNEL_COUNT))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    if (device->pwm_frequency_hz == 0U)
    {
        return PCA9685_ERR_STATE;
    }

    pulse_counts = ((uint64_t)pulse_width_us * (uint64_t)device->pwm_frequency_hz * (uint64_t)PCA9685_PWM_STEPS + 500000ULL)
        / 1000000ULL;

    if (pulse_counts >= (uint64_t)PCA9685_PWM_STEPS)
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    return pca9685_set_channel_pwm(device, channel, 0U, (uint16_t)pulse_counts);
}

pca9685_status_t pca9685_set_channel_pulse_us_disabled(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t pulse_width_us)
{
    uint64_t pulse_counts;

    if (!pca9685_is_valid_device(device) || (channel >= PCA9685_CHANNEL_COUNT))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    if (device->pwm_frequency_hz == 0U)
    {
        return PCA9685_ERR_STATE;
    }

    pulse_counts = ((uint64_t)pulse_width_us * (uint64_t)device->pwm_frequency_hz * (uint64_t)PCA9685_PWM_STEPS + 500000ULL)
        / 1000000ULL;

    if (pulse_counts >= (uint64_t)PCA9685_PWM_STEPS)
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    return pca9685_write_channel_registers(
        device->instance,
        device->address,
        pca9685_channel_register_base(channel),
        0U,
        (uint16_t)pulse_counts,
        true);
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