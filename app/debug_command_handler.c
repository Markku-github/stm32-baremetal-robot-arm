/**
 ******************************************************************************
 * @file    debug_command_handler.c
 * @brief   Command execution and text formatting for the debug UART shell
 ******************************************************************************
 */

#include "debug_command_handler.h"

#include "debug_command_parser.h"

#define DEBUG_COMMAND_HANDLER_RADIANS_TO_DEGREES 57.2957795130823208768f
#define DEBUG_COMMAND_HANDLER_MAX_POSE_DELAY_SECONDS 30U

static bool debug_command_handler_has_valid_io(const debug_command_handler_io_t *io)
{
    return (io != 0)
        && (io->write_string != 0)
        && (io->write_byte != 0)
        && (io->write_prompt != 0);
}

static void debug_command_handler_write_unsigned_decimal(
    const debug_command_handler_io_t *io,
    void *io_context,
    uint32_t value)
{
    char digits[10];
    uint8_t digit_count = 0U;

    if (value == 0U)
    {
        io->write_byte(io_context, (uint8_t)'0');
        return;
    }

    while (value > 0U)
    {
        digits[digit_count] = (char)('0' + (value % 10U));
        digit_count++;
        value /= 10U;
    }

    while (digit_count > 0U)
    {
        digit_count--;
        io->write_byte(io_context, (uint8_t)digits[digit_count]);
    }
}

static void debug_command_handler_write_signed_decimal(
    const debug_command_handler_io_t *io,
    void *io_context,
    int32_t value)
{
    uint32_t magnitude;

    if (value < 0)
    {
        io->write_byte(io_context, (uint8_t)'-');
        magnitude = (uint32_t)(-value);
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    debug_command_handler_write_unsigned_decimal(io, io_context, magnitude);
}

static int32_t debug_command_handler_round_float_to_int32(float value)
{
    if (value < 0.0f)
    {
        return (int32_t)(value - 0.5f);
    }

    return (int32_t)(value + 0.5f);
}

static float debug_command_handler_radians_to_degrees(float radians)
{
    return radians * DEBUG_COMMAND_HANDLER_RADIANS_TO_DEGREES;
}

static void debug_command_handler_write_joint_status_line(
    const debug_command_handler_io_t *io,
    void *io_context,
    const char *joint_name,
    float angle_rad)
{
    io->write_string(io_context, joint_name);
    io->write_string(io_context, "=");
    debug_command_handler_write_signed_decimal(
        io,
        io_context,
        debug_command_handler_round_float_to_int32(debug_command_handler_radians_to_degrees(angle_rad)));
    io->write_string(io_context, " deg\r\n");
}

static void debug_command_handler_write_help_text(const debug_command_handler_io_t *io, void *io_context)
{
    io->write_string(io_context, "Commands:\r\n");
    io->write_string(io_context, "HELP\r\n");
    io->write_string(io_context, "FAULT USAGE\r\n");
    io->write_string(io_context, "HOME\r\n");
    io->write_string(io_context, "POSE <base_deg> <shoulder_deg> <elbow_deg> <wrist_tilt_deg> <wrist_rotate_deg> <gripper_deg>\r\n");
    io->write_string(io_context, "POSE_DELAY <delay_s> <base_deg> <shoulder_deg> <elbow_deg> <wrist_tilt_deg> <wrist_rotate_deg> <gripper_deg>\r\n");
    io->write_string(io_context, "STATUS\r\n");
}

static void debug_command_handler_write_pose_delay_notice(
    const debug_command_handler_io_t *io,
    void *io_context,
    uint32_t delay_seconds)
{
    io->write_string(io_context, "WAIT POSE ");
    debug_command_handler_write_unsigned_decimal(io, io_context, delay_seconds);
    io->write_string(io_context, " s\r\n");
}

static void debug_command_handler_write_command_ok(
    const debug_command_handler_io_t *io,
    void *io_context,
    const char *command_name)
{
    io->write_string(io_context, "OK ");
    io->write_string(io_context, command_name);
    io->write_string(io_context, "\r\n");
}

static void debug_command_handler_write_status_text(
    const debug_command_handler_io_t *io,
    void *io_context,
    const robot_arm_t *robot)
{
    robot_arm_pose_t pose;

    if ((robot == 0) || (robot_arm_get_current_pose(robot, &pose) != ROBOT_ARM_OK))
    {
        io->write_string(io_context, "ERR CONTROLLER_NOT_READY\r\n");
        return;
    }

    io->write_string(io_context, "STATUS\r\n");
    debug_command_handler_write_joint_status_line(io, io_context, "base", pose.base_rad);
    debug_command_handler_write_joint_status_line(io, io_context, "shoulder", pose.shoulder_rad);
    debug_command_handler_write_joint_status_line(io, io_context, "elbow", pose.elbow_rad);
    debug_command_handler_write_joint_status_line(io, io_context, "wrist_tilt", pose.wrist_tilt_rad);
    debug_command_handler_write_joint_status_line(io, io_context, "wrist_rotate", pose.wrist_rotate_rad);
    debug_command_handler_write_joint_status_line(io, io_context, "gripper", pose.gripper_rad);
}

static bool debug_command_handler_try_recover_robot(const debug_command_handler_context_t *command_context)
{
    return (command_context != 0)
        && (command_context->robot != 0)
        && (command_context->recover_robot != 0)
        && command_context->recover_robot(command_context->recover_context, command_context->robot);
}

static bool debug_command_handler_has_delay_support(const debug_command_handler_context_t *command_context)
{
    return (command_context != 0) && (command_context->delay_ms != 0);
}

static bool debug_command_handler_trigger_usage_fault(const debug_command_handler_context_t *command_context)
{
    return (command_context != 0)
        && (command_context->trigger_fault != 0)
        && command_context->trigger_fault(command_context->trigger_fault_context);
}

static void debug_command_handler_wait_before_pose(
    const debug_command_handler_context_t *command_context,
    uint32_t delay_seconds)
{
    if ((command_context == 0) || (command_context->delay_ms == 0))
    {
        return;
    }

    command_context->delay_ms(command_context->delay_context, delay_seconds * 1000U);
}

static bool debug_command_handler_execute_home_with_recovery(const debug_command_handler_context_t *command_context)
{
    if ((command_context == 0) || (command_context->robot == 0))
    {
        return false;
    }

    if (robot_arm_home(command_context->robot) == ROBOT_ARM_OK)
    {
        return true;
    }

    if (!debug_command_handler_try_recover_robot(command_context))
    {
        return false;
    }

    return robot_arm_home(command_context->robot) == ROBOT_ARM_OK;
}

static bool debug_command_handler_execute_pose_with_recovery(
    const debug_command_handler_context_t *command_context,
    const robot_arm_pose_t *pose)
{
    if ((command_context == 0) || (command_context->robot == 0) || (pose == 0))
    {
        return false;
    }

    if (robot_arm_set_pose_immediate(command_context->robot, pose) == ROBOT_ARM_OK)
    {
        return true;
    }

    if (!debug_command_handler_try_recover_robot(command_context))
    {
        return false;
    }

    return robot_arm_set_pose_immediate(command_context->robot, pose) == ROBOT_ARM_OK;
}

void debug_command_handler_execute(
    const char *command_line,
    const debug_command_handler_context_t *command_context,
    const debug_command_handler_io_t *io,
    void *io_context)
{
    const char *arguments = 0;
    const bool robot_ready = (command_context != 0) && command_context->robot_ready;
    robot_arm_t *robot = (command_context != 0) ? command_context->robot : 0;

    if (!debug_command_handler_has_valid_io(io))
    {
        return;
    }

    if ((command_line == 0) || (command_line[0] == '\0'))
    {
        io->write_prompt(io_context);
        return;
    }

    if (debug_command_parser_matches_name_with_arguments(command_line, "HELP", &arguments))
    {
        if ((arguments != 0) && (arguments[0] != '\0'))
        {
            io->write_string(io_context, "ERR INVALID_ARGUMENT\r\n");
        }
        else
        {
            debug_command_handler_write_help_text(io, io_context);
        }

        io->write_prompt(io_context);
        return;
    }

    if (debug_command_parser_matches_name_with_arguments(command_line, "STATUS", &arguments))
    {
        if ((arguments != 0) && (arguments[0] != '\0'))
        {
            io->write_string(io_context, "ERR INVALID_ARGUMENT\r\n");
        }
        else if (!robot_ready)
        {
            io->write_string(io_context, "ERR CONTROLLER_NOT_READY\r\n");
        }
        else
        {
            debug_command_handler_write_status_text(io, io_context, robot);
        }

        io->write_prompt(io_context);
        return;
    }

    if (debug_command_parser_matches_name_with_arguments(command_line, "FAULT", &arguments))
    {
        const char *fault_arguments = 0;

        if (!debug_command_parser_matches_name_with_arguments(arguments, "USAGE", &fault_arguments)
            || ((fault_arguments != 0) && (fault_arguments[0] != '\0')))
        {
            io->write_string(io_context, "ERR INVALID_ARGUMENT\r\n");
            io->write_prompt(io_context);
            return;
        }

        io->write_string(io_context, "TRIGGER FAULT USAGE\r\n");

        if (!debug_command_handler_trigger_usage_fault(command_context))
        {
            io->write_string(io_context, "ERR COMMAND_FAILED\r\n");
        }

        io->write_prompt(io_context);

        return;
    }

    if (debug_command_parser_matches_name_with_arguments(command_line, "HOME", &arguments))
    {
        if ((arguments != 0) && (arguments[0] != '\0'))
        {
            io->write_string(io_context, "ERR INVALID_ARGUMENT\r\n");
        }
        else if (!robot_ready || (robot == 0))
        {
            io->write_string(io_context, "ERR CONTROLLER_NOT_READY\r\n");
        }
        else if (!debug_command_handler_execute_home_with_recovery(command_context))
        {
            io->write_string(io_context, "ERR COMMAND_FAILED\r\n");
        }
        else
        {
            debug_command_handler_write_command_ok(io, io_context, "HOME");
        }

        io->write_prompt(io_context);
        return;
    }

    if (debug_command_parser_matches_name_with_arguments(command_line, "POSE", &arguments))
    {
        robot_arm_pose_t pose;

        if (!robot_ready || (robot == 0))
        {
            io->write_string(io_context, "ERR CONTROLLER_NOT_READY\r\n");
        }
        else if (!debug_command_parser_parse_pose_arguments(arguments, &pose))
        {
            io->write_string(io_context, "ERR INVALID_ARGUMENT\r\n");
        }
        else if (!debug_command_handler_execute_pose_with_recovery(command_context, &pose))
        {
            io->write_string(io_context, "ERR COMMAND_FAILED\r\n");
        }
        else
        {
            debug_command_handler_write_command_ok(io, io_context, "POSE");
        }

        io->write_prompt(io_context);
        return;
    }

    if (debug_command_parser_matches_name_with_arguments(command_line, "POSE_DELAY", &arguments))
    {
        robot_arm_pose_t pose;
        uint32_t delay_seconds = 0U;

        if (!robot_ready || (robot == 0))
        {
            io->write_string(io_context, "ERR CONTROLLER_NOT_READY\r\n");
        }
        else if (!debug_command_handler_has_delay_support(command_context))
        {
            io->write_string(io_context, "ERR COMMAND_FAILED\r\n");
        }
        else if (!debug_command_parser_parse_delayed_pose_arguments(arguments, &delay_seconds, &pose)
            || (delay_seconds > DEBUG_COMMAND_HANDLER_MAX_POSE_DELAY_SECONDS))
        {
            io->write_string(io_context, "ERR INVALID_ARGUMENT\r\n");
        }
        else
        {
            debug_command_handler_write_pose_delay_notice(io, io_context, delay_seconds);
            debug_command_handler_wait_before_pose(command_context, delay_seconds);

            if (!debug_command_handler_execute_pose_with_recovery(command_context, &pose))
            {
                io->write_string(io_context, "ERR COMMAND_FAILED\r\n");
            }
            else
            {
                debug_command_handler_write_command_ok(io, io_context, "POSE_DELAY");
            }
        }

        io->write_prompt(io_context);
        return;
    }

    io->write_string(io_context, "ERR UNKNOWN_COMMAND\r\n");
    io->write_prompt(io_context);
}