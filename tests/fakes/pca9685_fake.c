#include "pca9685_fake.h"

static pca9685_fake_state_t pca9685_fake_state_storage;

static uint16_t pca9685_fake_pulse_us_to_off_count(const pca9685_device_t *device, uint16_t pulse_width_us)
{
    const uint64_t pulse_counts = ((uint64_t)pulse_width_us * (uint64_t)device->pwm_frequency_hz * (uint64_t)PCA9685_PWM_STEPS + 500000ULL)
        / 1000000ULL;

    return (uint16_t)pulse_counts;
}

void pca9685_fake_reset(void)
{
    uint8_t channel;

    pca9685_fake_state_storage.call_count = 0U;
    pca9685_fake_state_storage.read_call_count = 0U;
    pca9685_fake_state_storage.disable_call_count = 0U;
    pca9685_fake_state_storage.last_device = 0;
    pca9685_fake_state_storage.last_channel = 0U;
    pca9685_fake_state_storage.last_pulse_width_us = 0U;
    for (channel = 0U; channel < PCA9685_CHANNEL_COUNT; channel++)
    {
        pca9685_fake_state_storage.current_pulse_width_us_by_channel[channel] = 0U;
        pca9685_fake_state_storage.last_pulse_width_us_by_channel[channel] = 0U;
        pca9685_fake_state_storage.last_off_count_by_channel[channel] = 0U;
    }
    pca9685_fake_state_storage.next_status = PCA9685_OK;
}

pca9685_fake_state_t *pca9685_fake_state(void)
{
    return &pca9685_fake_state_storage;
}

pca9685_status_t pca9685_init(
    pca9685_device_t *device,
    bsp_i2c_instance_t instance,
    uint8_t address)
{
    if ((device == 0) || (address > 0x7FU))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    if (pca9685_fake_state_storage.next_status != PCA9685_OK)
    {
        return pca9685_fake_state_storage.next_status;
    }

    device->instance = instance;
    device->address = address;
    if (device->oscillator_frequency_hz == 0UL)
    {
        device->oscillator_frequency_hz = PCA9685_OSCILLATOR_FREQUENCY_HZ;
    }

    return PCA9685_OK;
}

pca9685_status_t pca9685_set_pwm_frequency(pca9685_device_t *device, uint16_t pwm_frequency_hz)
{
    if ((device == 0) || (pwm_frequency_hz == 0U))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    if (pca9685_fake_state_storage.next_status != PCA9685_OK)
    {
        return pca9685_fake_state_storage.next_status;
    }

    device->pwm_frequency_hz = pwm_frequency_hz;
    return PCA9685_OK;
}

pca9685_status_t pca9685_set_channel_pulse_us(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t pulse_width_us)
{
    pca9685_fake_state_storage.call_count++;
    pca9685_fake_state_storage.last_device = device;
    pca9685_fake_state_storage.last_channel = channel;
    pca9685_fake_state_storage.last_pulse_width_us = pulse_width_us;
    pca9685_fake_state_storage.current_pulse_width_us_by_channel[channel] = pulse_width_us;
    pca9685_fake_state_storage.last_pulse_width_us_by_channel[channel] = pulse_width_us;

    return pca9685_fake_state_storage.next_status;
}

pca9685_status_t pca9685_read_channel_pwm(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t *on_count,
    uint16_t *off_count)
{
    if ((device == 0) || (channel >= PCA9685_CHANNEL_COUNT) || (on_count == 0) || (off_count == 0))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    pca9685_fake_state_storage.read_call_count++;

    if (pca9685_fake_state_storage.next_status != PCA9685_OK)
    {
        return pca9685_fake_state_storage.next_status;
    }

    *on_count = 0U;
    *off_count = pca9685_fake_pulse_us_to_off_count(device, pca9685_fake_state_storage.current_pulse_width_us_by_channel[channel]);
    pca9685_fake_state_storage.last_off_count_by_channel[channel] = *off_count;
    return PCA9685_OK;
}

pca9685_status_t pca9685_disable_all_outputs(const pca9685_device_t *device)
{
    uint8_t channel;

    if (device == 0)
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    pca9685_fake_state_storage.disable_call_count++;

    if (pca9685_fake_state_storage.next_status != PCA9685_OK)
    {
        return pca9685_fake_state_storage.next_status;
    }

    for (channel = 0U; channel < PCA9685_CHANNEL_COUNT; channel++)
    {
        pca9685_fake_state_storage.current_pulse_width_us_by_channel[channel] = 0U;
    }

    return PCA9685_OK;
}

pca9685_status_t pca9685_read_register(
    bsp_i2c_instance_t instance,
    uint8_t address,
    uint8_t register_address,
    uint8_t *value)
{
    (void)instance;

    if ((value == 0) || (address > 0x7FU))
    {
        return PCA9685_ERR_INVALID_ARGUMENT;
    }

    if (pca9685_fake_state_storage.next_status != PCA9685_OK)
    {
        return pca9685_fake_state_storage.next_status;
    }

    switch (register_address)
    {
        case PCA9685_REGISTER_MODE1:
            *value = 0x21U;
            break;

        case PCA9685_REGISTER_MODE2:
            *value = 0x04U;
            break;

        case PCA9685_REGISTER_PRESCALE:
            *value = 0x79U;
            break;

        default:
            *value = 0x00U;
            break;
    }

    return PCA9685_OK;
}