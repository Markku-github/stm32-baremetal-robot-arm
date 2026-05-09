#ifndef PCA9685_FAKE_H
#define PCA9685_FAKE_H

#include <stdint.h>

#include "pca9685.h"

typedef struct
{
    uint32_t call_count;
    const pca9685_device_t *last_device;
    uint8_t last_channel;
    uint16_t last_pulse_width_us;
    pca9685_status_t next_status;
} pca9685_fake_state_t;

void pca9685_fake_reset(void);
pca9685_fake_state_t *pca9685_fake_state(void);

#endif