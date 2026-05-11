/**
 ******************************************************************************
 * @file    boot_self_test.h
 * @brief   Boot-time PCA9685 and robot self-test entry points
 ******************************************************************************
 */

#ifndef BOOT_SELF_TEST_H
#define BOOT_SELF_TEST_H

#include <stdbool.h>

#include "pca9685.h"

bool boot_self_test_run_pca9685(bool debug_uart_ready, pca9685_device_t *device);
bool boot_self_test_run_robot_home(bool debug_uart_ready, pca9685_device_t *device);
bool boot_self_test_run_robot_direct_pose(bool debug_uart_ready, pca9685_device_t *device);

#endif