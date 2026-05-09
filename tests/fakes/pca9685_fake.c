#include "pca9685_fake.h"

static pca9685_fake_state_t pca9685_fake_state_storage;

void pca9685_fake_reset(void)
{
    pca9685_fake_state_storage.call_count = 0U;
    pca9685_fake_state_storage.last_device = 0;
    pca9685_fake_state_storage.last_channel = 0U;
    pca9685_fake_state_storage.last_pulse_width_us = 0U;
    pca9685_fake_state_storage.next_status = PCA9685_OK;
}

pca9685_fake_state_t *pca9685_fake_state(void)
{
    return &pca9685_fake_state_storage;
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

    return pca9685_fake_state_storage.next_status;
}