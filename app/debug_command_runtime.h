/**
 ******************************************************************************
 * @file    debug_command_runtime.h
 * @brief   Runtime glue between board UART transport and the command shell
 ******************************************************************************
 */

#ifndef DEBUG_COMMAND_RUNTIME_H
#define DEBUG_COMMAND_RUNTIME_H

#include <stdbool.h>

#include "pca9685.h"
#include "robot_arm.h"

bool debug_command_runtime_has_pending_work(bool debug_uart_rx_ready);

void debug_command_runtime_service_motion(
	bool robot_ready,
	robot_arm_t *robot,
	pca9685_device_t *pca9685_device);

void debug_command_runtime_process_input(
	bool debug_uart_rx_ready,
	bool *robot_ready,
	robot_arm_t *robot,
	pca9685_device_t *pca9685_device);

#endif