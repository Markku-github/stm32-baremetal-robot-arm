/**
 ******************************************************************************
 * @file    debug_command_runtime.c
 * @brief   Runtime glue between board UART transport and the command shell
 ******************************************************************************
 */

#include "debug_command_runtime.h"

#include <stdint.h>

#include "board_nucleo_f767zi.h"
#include "debug_console.h"
#include "debug_command_handler.h"
#include "debug_command_shell.h"

static const debug_command_handler_io_t debug_command_handler_io = {
    .write_string = debug_console_write_string_adapter,
    .write_byte = debug_console_write_byte_adapter,
    .write_prompt = debug_console_write_prompt_adapter,
};

static void debug_command_shell_execute(void *context, const char *command_line)
{
    debug_command_handler_context_t *execution_context = (debug_command_handler_context_t *)context;

    if (execution_context == 0)
    {
        return;
    }

    debug_command_handler_execute(command_line, execution_context, &debug_command_handler_io, 0);
}

static const debug_command_shell_io_t debug_command_shell_io = {
    .write_string = debug_console_write_string_adapter,
    .write_byte = debug_console_write_byte_adapter,
    .write_prompt = debug_console_write_prompt_adapter,
    .execute_command = debug_command_shell_execute,
};

void debug_command_runtime_process_input(bool debug_uart_rx_ready, bool robot_ready, robot_arm_t *robot)
{
    static debug_command_shell_t command_shell;
    debug_command_handler_context_t execution_context;

    if (!debug_uart_rx_ready)
    {
        return;
    }

    execution_context.robot_ready = robot_ready;
    execution_context.robot = robot_ready ? robot : 0;

    if (board_nucleo_f767zi_debug_uart_overflowed())
    {
        board_nucleo_f767zi_clear_debug_uart_overflow();
        debug_command_shell_handle_transport_overflow(&command_shell, &debug_command_shell_io, &execution_context);
    }

    for (;;)
    {
        uint8_t received_byte;
        const bsp_uart_status_t status = board_nucleo_f767zi_read_debug_byte(&received_byte);

        if (status == BSP_UART_ERR_NO_DATA)
        {
            return;
        }

        if (status != BSP_UART_OK)
        {
            debug_command_shell_handle_transport_read_error(&command_shell, &debug_command_shell_io, &execution_context);
            return;
        }

        debug_command_shell_process_byte(&command_shell, received_byte, &debug_command_shell_io, &execution_context);
    }
}