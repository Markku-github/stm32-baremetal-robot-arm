/**
 ******************************************************************************
 * @file    debug_command_runtime.h
 * @brief   Runtime glue between board UART transport and the command shell
 ******************************************************************************
 */

#ifndef DEBUG_COMMAND_RUNTIME_H
#define DEBUG_COMMAND_RUNTIME_H

#include <stdbool.h>

#include "robot_arm.h"

void debug_command_runtime_process_input(bool debug_uart_rx_ready, bool robot_ready, robot_arm_t *robot);

#endif