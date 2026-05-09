/**
 ******************************************************************************
 * @file    debug_command_parser.h
 * @brief   Pure helpers for the line-based debug command parser
 ******************************************************************************
 */

#ifndef DEBUG_COMMAND_PARSER_H
#define DEBUG_COMMAND_PARSER_H

#include <stdbool.h>
#include <stdint.h>

#include "robot_arm.h"

/**
 * @brief  Trim leading and trailing ASCII command whitespace in-place
 * @param  line: command buffer to normalize
 * @param  length: current command length excluding the terminator
 * @retval uint8_t: trimmed command length
 */
uint8_t debug_command_parser_trim_line(char *line, uint8_t length);

/**
 * @brief  Match one command name and return the remaining argument cursor
 * @param  command_line: full trimmed command line
 * @param  command_name: expected command token
 * @param  arguments: destination for the remaining argument substring
 * @retval true: command name matched and arguments cursor updated
 * @retval false: command name did not match or input invalid
 */
bool debug_command_parser_matches_name_with_arguments(
    const char *command_line,
    const char *command_name,
    const char **arguments);

/**
 * @brief  Parse one signed decimal token from a command argument cursor
 * @param  cursor: source and destination cursor for the token stream
 * @param  value: destination for the parsed integer value
 * @retval true: token parsed successfully
 * @retval false: token missing, malformed, or overflowed
 */
bool debug_command_parser_parse_signed_int32_token(const char **cursor, int32_t *value);

/**
 * @brief  Parse one six-joint POSE argument list in degrees
 * @param  arguments: argument substring following the POSE command
 * @param  pose: destination for the parsed pose in radians
 * @retval true: pose parsed successfully
 * @retval false: arguments missing, malformed, or incomplete
 */
bool debug_command_parser_parse_pose_arguments(const char *arguments, robot_arm_pose_t *pose);

#endif