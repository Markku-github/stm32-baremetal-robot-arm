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
