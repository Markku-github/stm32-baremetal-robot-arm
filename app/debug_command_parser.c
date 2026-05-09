/**
 ******************************************************************************
 * @file    debug_command_parser.c
 * @brief   Pure helpers for the line-based debug command parser
 ******************************************************************************
 */

#include "debug_command_parser.h"

#define DEBUG_COMMAND_DEGREES_TO_RADIANS 0.01745329251994329577f

static float degrees_to_radians(float degrees)
{
    return degrees * DEBUG_COMMAND_DEGREES_TO_RADIANS;
}

static uint8_t ascii_to_upper(uint8_t value)
{
    if ((value >= (uint8_t)'a') && (value <= (uint8_t)'z'))
    {
        return (uint8_t)(value - ((uint8_t)'a' - (uint8_t)'A'));
    }

    return value;
}

static bool is_ascii_whitespace(uint8_t value)
{
    return (value == (uint8_t)' ') || (value == (uint8_t)'\t');
}

static bool is_ascii_digit(uint8_t value)
{
    return (value >= (uint8_t)'0') && (value <= (uint8_t)'9');
}

static const char *skip_ascii_whitespace_in_command(const char *cursor)
{
    if (cursor == 0)
    {
        return 0;
    }

    while ((*cursor != '\0') && is_ascii_whitespace((uint8_t)(*cursor)))
    {
        cursor++;
    }

    return cursor;
}

uint8_t debug_command_parser_trim_line(char *line, uint8_t length)
{
    uint8_t start_index = 0U;
    uint8_t end_index = length;
    uint8_t trimmed_length;
    uint8_t index;

    if (line == 0)
    {
        return 0U;
    }

    while ((start_index < length) && is_ascii_whitespace((uint8_t)line[start_index]))
    {
        start_index++;
    }

    while ((end_index > start_index) && is_ascii_whitespace((uint8_t)line[end_index - 1U]))
    {
        end_index--;
    }

    trimmed_length = (uint8_t)(end_index - start_index);
    for (index = 0U; index < trimmed_length; index++)
    {
        line[index] = line[start_index + index];
    }

    line[trimmed_length] = '\0';
    return trimmed_length;
}

bool debug_command_parser_matches_name_with_arguments(
    const char *command_line,
    const char *command_name,
    const char **arguments)
{
    uint8_t index = 0U;
    const char *cursor;

    if (arguments != 0)
    {
        *arguments = 0;
    }

    if ((command_line == 0) || (command_name == 0))
    {
        return false;
    }

    while (command_name[index] != '\0')
    {
        if (ascii_to_upper((uint8_t)command_line[index]) != ascii_to_upper((uint8_t)command_name[index]))
        {
            return false;
        }

        index++;
    }

    if ((command_line[index] != '\0') && !is_ascii_whitespace((uint8_t)command_line[index]))
    {
        return false;
    }

    cursor = skip_ascii_whitespace_in_command(&command_line[index]);
    if (arguments != 0)
    {
        *arguments = cursor;
    }

    return true;
}

bool debug_command_parser_parse_signed_int32_token(const char **cursor, int32_t *value)
{
    const char *token;
    int32_t parsed_value = 0;
    bool negative = false;

    if ((cursor == 0) || (*cursor == 0) || (value == 0))
    {
        return false;
    }

    token = skip_ascii_whitespace_in_command(*cursor);
    if ((token == 0) || (*token == '\0'))
    {
        return false;
    }

    if ((*token == '+') || (*token == '-'))
    {
        negative = *token == '-';
        token++;
    }

    if (!is_ascii_digit((uint8_t)(*token)))
    {
        return false;
    }

    while (is_ascii_digit((uint8_t)(*token)))
    {
        const int32_t digit = (int32_t)(*token - '0');

        if (parsed_value > ((2147483647 - digit) / 10))
        {
            return false;
        }

        parsed_value = (parsed_value * 10) + digit;
        token++;
    }

    if ((*token != '\0') && !is_ascii_whitespace((uint8_t)(*token)))
    {
        return false;
    }

    *value = negative ? -parsed_value : parsed_value;
    *cursor = skip_ascii_whitespace_in_command(token);
    return true;
}

bool debug_command_parser_parse_pose_arguments(const char *arguments, robot_arm_pose_t *pose)
{
    const char *cursor = arguments;
    int32_t base_deg;
    int32_t shoulder_deg;
    int32_t elbow_deg;
    int32_t wrist_tilt_deg;
    int32_t wrist_rotate_deg;
    int32_t gripper_deg;

    if ((arguments == 0) || (pose == 0))
    {
        return false;
    }

    if (!debug_command_parser_parse_signed_int32_token(&cursor, &base_deg)
        || !debug_command_parser_parse_signed_int32_token(&cursor, &shoulder_deg)
        || !debug_command_parser_parse_signed_int32_token(&cursor, &elbow_deg)
        || !debug_command_parser_parse_signed_int32_token(&cursor, &wrist_tilt_deg)
        || !debug_command_parser_parse_signed_int32_token(&cursor, &wrist_rotate_deg)
        || !debug_command_parser_parse_signed_int32_token(&cursor, &gripper_deg)
        || ((cursor != 0) && (*cursor != '\0')))
    {
        return false;
    }

    pose->base_rad = degrees_to_radians((float)base_deg);
    pose->shoulder_rad = degrees_to_radians((float)shoulder_deg);
    pose->elbow_rad = degrees_to_radians((float)elbow_deg);
    pose->wrist_tilt_rad = degrees_to_radians((float)wrist_tilt_deg);
    pose->wrist_rotate_rad = degrees_to_radians((float)wrist_rotate_deg);
    pose->gripper_rad = degrees_to_radians((float)gripper_deg);

    return true;
}