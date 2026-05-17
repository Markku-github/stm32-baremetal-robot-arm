#ifndef ROBOT_STARTUP_H
#define ROBOT_STARTUP_H

#include "pca9685.h"
#include "robot_arm.h"

typedef enum
{
    ROBOT_STARTUP_OK = 0,
    ROBOT_STARTUP_ERR_INVALID_ARGUMENT,
    ROBOT_STARTUP_ERR_INIT,
    ROBOT_STARTUP_ERR_HOME,
} robot_startup_status_t;

robot_startup_status_t robot_startup_initialize(
    robot_arm_t *robot,
    const pca9685_device_t *device);

robot_startup_status_t robot_startup_initialize_and_home(
    robot_arm_t *robot,
    const pca9685_device_t *device);

#endif