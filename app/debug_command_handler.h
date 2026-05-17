/**
 ******************************************************************************
 * @file    debug_command_handler.h
 * @brief   Command execution and text formatting for the debug UART shell
 ******************************************************************************
 */

#ifndef DEBUG_COMMAND_HANDLER_H
#define DEBUG_COMMAND_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

#include "robot_arm.h"

typedef void (*debug_command_handler_write_string_fn)(void *context, const char *text);
typedef void (*debug_command_handler_write_byte_fn)(void *context, uint8_t byte);
typedef void (*debug_command_handler_write_prompt_fn)(void *context);
typedef bool (*debug_command_handler_recover_robot_fn)(void *context, robot_arm_t *robot);
typedef void (*debug_command_handler_delay_ms_fn)(void *context, uint32_t delay_ms);
typedef bool (*debug_command_handler_trigger_fault_fn)(void *context);
typedef bool (*debug_command_handler_schedule_home_fn)(void *context);
typedef bool (*debug_command_handler_schedule_pose_fn)(void *context, const robot_arm_pose_t *pose);

typedef struct
{
    debug_command_handler_write_string_fn write_string;
    debug_command_handler_write_byte_fn write_byte;
    debug_command_handler_write_prompt_fn write_prompt;
} debug_command_handler_io_t;

typedef struct
{
    bool robot_ready;
    robot_arm_t *robot;
    debug_command_handler_recover_robot_fn recover_robot;
    void *recover_context;
    debug_command_handler_delay_ms_fn delay_ms;
    void *delay_context;
    debug_command_handler_trigger_fault_fn trigger_fault;
    void *trigger_fault_context;
    debug_command_handler_schedule_home_fn schedule_home;
    debug_command_handler_schedule_pose_fn schedule_pose;
    void *motion_context;
} debug_command_handler_context_t;

void debug_command_handler_execute(
    const char *command_line,
    const debug_command_handler_context_t *command_context,
    const debug_command_handler_io_t *io,
    void *io_context);

#endif