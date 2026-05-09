/**
 ******************************************************************************
 * @file    main.c
 * @brief   Early application entry point for board bring-up, controller self-tests, and USART6 command-shell handling
 ******************************************************************************
 */

#include <stdbool.h>
#include <stdint.h>

#include "boot_self_test.h"
#include "board_nucleo_f767zi.h"
#include "debug_console.h"
#include "debug_command_handler.h"
#include "debug_command_shell.h"
#include "pca9685.h"
#include "robot_arm.h"

#define MAIN_LOOP_DELAY_CYCLES 20000U
#define MAIN_LED_TOGGLE_TICKS 100U

static void debug_command_shell_execute(void *context, const char *command_line);

/**
 * @brief  Provide a short busy-wait delay for the cooperative main loop
 * @param  cycles: loop iterations to wait
 * @retval None
 */
static void boot_delay(volatile uint32_t cycles)
{
    while (cycles > 0U)
    {
        cycles--;
    }
}

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

/**
 * @brief  Drain and process received bytes from the debug UART ring buffer
 * @param  debug_uart_rx_ready: true when USART6 RX interrupts were enabled
 * @param  robot_ready: true when the runtime robot controller state is available
 * @param  robot: baseline robot controller state used by the command shell
 * @retval None
 * @note   Line handling stays in thread context so the interrupt handler only
 *         captures received bytes.
 */
static void process_debug_uart_input(bool debug_uart_rx_ready, bool robot_ready, robot_arm_t *robot)
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

/**
 * @brief  Initialize the board, run controller self-tests, and enter the UART command loop
 * @retval int  This function does not return during normal operation.
 */
int main(void)
{
    uint32_t led_tick_counter = 0U;
    pca9685_device_t pca9685_device;
    robot_arm_t robot;
    bool robot_ready = false;

    if (board_nucleo_f767zi_init() != BSP_GPIO_OK)
    {
        for (;;)
        {
        }
    }

    const bool debug_uart_ready = board_nucleo_f767zi_init_debug_uart() == BSP_UART_OK;
    const bool debug_uart_rx_ready = debug_uart_ready && (board_nucleo_f767zi_enable_debug_uart_rx_interrupt() == BSP_UART_OK);

    if (debug_uart_ready)
    {
        board_nucleo_f767zi_write_debug_string("Booting...\r\n");
        if (boot_self_test_run_pca9685(debug_uart_ready, &pca9685_device))
        {
            boot_self_test_run_robot_home(debug_uart_ready, &pca9685_device);
            boot_self_test_run_robot_direct_pose(debug_uart_ready, &pca9685_device);

            if (robot_arm_init(&robot, &pca9685_device) == ROBOT_ARM_OK)
            {
                robot_ready = true;
            }
            else
            {
                board_nucleo_f767zi_write_debug_string("Robot runtime init failed.\r\n");
            }
        }

        if (debug_uart_rx_ready)
        {
            board_nucleo_f767zi_write_debug_string("USART6 RX command shell ready. Type HELP.\r\n");
            debug_console_write_prompt();
        }
        else
        {
            board_nucleo_f767zi_write_debug_string("USART6 RX interrupt setup failed.\r\n");
        }
    }

    for (;;)
    {
        process_debug_uart_input(debug_uart_rx_ready, robot_ready, &robot);
        boot_delay(MAIN_LOOP_DELAY_CYCLES);

        led_tick_counter++;
        if (led_tick_counter >= MAIN_LED_TOGGLE_TICKS)
        {
            board_nucleo_f767zi_toggle_debug_led();
            led_tick_counter = 0U;
        }
    }
}
