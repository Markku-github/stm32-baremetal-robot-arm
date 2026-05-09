#include "debug_command_shell.h"

#include "debug_command_parser.h"

static bool debug_command_shell_has_valid_io(const debug_command_shell_io_t *io)
{
    return (io != 0)
        && (io->write_string != 0)
        && (io->write_byte != 0)
        && (io->write_prompt != 0)
        && (io->execute_command != 0);
}

static void debug_command_shell_finalize_command(
    debug_command_shell_t *shell,
    const debug_command_shell_io_t *io,
    void *context)
{
    uint8_t trimmed_length;

    if ((shell == 0) || !debug_command_shell_has_valid_io(io))
    {
        return;
    }

    if (shell->command_overflowed)
    {
        debug_command_shell_reset(shell);
        io->write_string(context, "ERR COMMAND_TOO_LONG\r\n");
        io->write_prompt(context);
        return;
    }

    trimmed_length = debug_command_parser_trim_line(shell->command_buffer, shell->command_length);
    shell->command_length = 0U;
    shell->command_overflowed = false;
    io->execute_command(context, (trimmed_length > 0U) ? shell->command_buffer : "");
    shell->command_buffer[0] = '\0';
}

void debug_command_shell_init(debug_command_shell_t *shell)
{
    debug_command_shell_reset(shell);
}

void debug_command_shell_reset(debug_command_shell_t *shell)
{
    if (shell == 0)
    {
        return;
    }

    shell->command_buffer[0] = '\0';
    shell->command_length = 0U;
    shell->command_overflowed = false;
    shell->previous_byte_was_carriage_return = false;
}

void debug_command_shell_handle_transport_overflow(
    debug_command_shell_t *shell,
    const debug_command_shell_io_t *io,
    void *context)
{
    if ((shell == 0) || !debug_command_shell_has_valid_io(io))
    {
        return;
    }

    debug_command_shell_reset(shell);
    io->write_string(context, "\r\n[RX overflow]\r\n");
    io->write_prompt(context);
}

void debug_command_shell_handle_transport_read_error(
    debug_command_shell_t *shell,
    const debug_command_shell_io_t *io,
    void *context)
{
    if ((shell == 0) || !debug_command_shell_has_valid_io(io))
    {
        return;
    }

    debug_command_shell_reset(shell);
    io->write_string(context, "\r\n[RX read error]\r\n");
    io->write_prompt(context);
}

void debug_command_shell_process_byte(
    debug_command_shell_t *shell,
    uint8_t received_byte,
    const debug_command_shell_io_t *io,
    void *context)
{
    if ((shell == 0) || !debug_command_shell_has_valid_io(io))
    {
        return;
    }

    if (received_byte == '\r')
    {
        io->write_string(context, "\r\n");
        debug_command_shell_finalize_command(shell, io, context);
        shell->previous_byte_was_carriage_return = true;
        return;
    }

    if (received_byte == '\n')
    {
        if (!shell->previous_byte_was_carriage_return)
        {
            io->write_string(context, "\r\n");
            debug_command_shell_finalize_command(shell, io, context);
        }

        shell->previous_byte_was_carriage_return = false;
        return;
    }

    shell->previous_byte_was_carriage_return = false;

    if ((received_byte == 0x08U) || (received_byte == 0x7FU))
    {
        if (!shell->command_overflowed && (shell->command_length > 0U))
        {
            shell->command_length--;
            shell->command_buffer[shell->command_length] = '\0';
            io->write_string(context, "\b \b");
        }

        return;
    }

    if ((received_byte < 0x20U) || (received_byte > 0x7EU))
    {
        return;
    }

    if (shell->command_overflowed)
    {
        return;
    }

    if (shell->command_length >= (DEBUG_COMMAND_SHELL_COMMAND_CAPACITY - 1U))
    {
        shell->command_overflowed = true;
        return;
    }

    shell->command_buffer[shell->command_length] = (char)received_byte;
    shell->command_length++;
    shell->command_buffer[shell->command_length] = '\0';

    io->write_byte(context, received_byte);
}