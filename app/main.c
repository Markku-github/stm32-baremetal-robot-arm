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
#include "debug_command_runtime.h"
#include "pca9685.h"
#include "robot_arm.h"
#include "robot_startup.h"
#include "runtime_log.h"

#define MAIN_LOOP_DELAY_CYCLES 20000U
#define MAIN_LED_TOGGLE_TICKS 100U

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

/**
 * @brief  Initialize the board, run controller self-tests, and enter the UART command loop
 * @retval int  This function does not return during normal operation.
 */
int main(void)
{
    uint32_t led_tick_counter = 0U;
    pca9685_device_t pca9685_device;
    robot_arm_t robot;
    bool robot_self_tests_ok = false;
    bool robot_ready = false;

    if (board_nucleo_f767zi_init() != BSP_GPIO_OK)
    {
        for (;;)
        {
        }
    }

    const bool log_uart_ready = runtime_log_init();
    const bool debug_uart_ready = board_nucleo_f767zi_init_debug_uart() == BSP_UART_OK;
    const bool debug_uart_rx_ready = debug_uart_ready && (board_nucleo_f767zi_enable_debug_uart_rx_interrupt() == BSP_UART_OK);

    runtime_log_enable_debug_fallback(!log_uart_ready && debug_uart_ready);

    if (log_uart_ready || debug_uart_ready)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "Booting...");
    }

    if (boot_self_test_run_pca9685(&pca9685_device))
    {
        const bool robot_home_self_test_ok = boot_self_test_run_robot_home(&pca9685_device);
        const bool robot_direct_pose_self_test_ok = boot_self_test_run_robot_direct_pose(&pca9685_device);

        robot_self_tests_ok = robot_home_self_test_ok && robot_direct_pose_self_test_ok;

        if (robot_self_tests_ok)
        {
            const robot_startup_status_t robot_startup_status =
                robot_startup_initialize_and_home(&robot, &pca9685_device);

            if (robot_startup_status == ROBOT_STARTUP_OK)
            {
                robot_ready = true;
                runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "Robot runtime ready at HOME.");
            }
            else if (robot_startup_status == ROBOT_STARTUP_ERR_HOME)
            {
                runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "Robot startup HOME failed.");
            }
            else
            {
                runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "Robot runtime init failed.");
            }
        }
        else
        {
            runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "Robot self-test sequence failed. Controller will remain not ready.");
        }
    }

    if (!debug_uart_ready)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_WARNING, "USART6 command shell unavailable.");
    }
    else if (debug_uart_rx_ready)
    {
        if (robot_ready)
        {
            runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "USART6 RX command shell ready. Type HELP.");
        }
        else
        {
            runtime_log_write_line(RUNTIME_LOG_LEVEL_WARNING, "USART6 RX command shell ready for diagnostics. Controller not ready.");
        }

        debug_console_write_prompt();
    }
    else
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "USART6 RX interrupt setup failed.");
    }

    for (;;)
    {
        debug_command_runtime_process_input(debug_uart_rx_ready, robot_ready, &robot, &pca9685_device);
        boot_delay(MAIN_LOOP_DELAY_CYCLES);

        led_tick_counter++;
        if (led_tick_counter >= MAIN_LED_TOGGLE_TICKS)
        {
            board_nucleo_f767zi_toggle_debug_led();
            led_tick_counter = 0U;
        }
    }
}
