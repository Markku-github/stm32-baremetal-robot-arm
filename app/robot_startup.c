#include "robot_startup.h"

robot_startup_status_t robot_startup_initialize_and_home(
    robot_arm_t *robot,
    const pca9685_device_t *device)
{
    if ((robot == 0) || (device == 0))
    {
        return ROBOT_STARTUP_ERR_INVALID_ARGUMENT;
    }

    if (robot_arm_init(robot, device) != ROBOT_ARM_OK)
    {
        return ROBOT_STARTUP_ERR_INIT;
    }

    if (robot_arm_home(robot) != ROBOT_ARM_OK)
    {
        return ROBOT_STARTUP_ERR_HOME;
    }

    return ROBOT_STARTUP_OK;
}