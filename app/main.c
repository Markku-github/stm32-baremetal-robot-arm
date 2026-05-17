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
#include "runtime_contract.h"
#include "runtime_led.h"
#include "runtime_log.h"
#include "runtime_status.h"
#include "runtime_tick.h"

/**
 * @brief  Initialize the board, run controller self-tests, and enter the UART command loop
 * @retval int  This function does not return during normal operation.
 */
int main(void)
{
    uint32_t last_periodic_tick_ms = 0U;
    pca9685_device_t pca9685_device;
    robot_arm_t robot;
    runtime_led_t runtime_led;
    bool robot_self_tests_ok = false;
    bool robot_initialized = false;
    bool robot_ready = false;

    if (board_nucleo_f767zi_init() != BSP_GPIO_OK)
    {
        for (;;)
        {
        }
    }

    runtime_led_init(&runtime_led);
    runtime_led_set_state(&runtime_led, RUNTIME_LED_STATE_STARTUP);

    const bool log_uart_ready = runtime_log_init();
    const bool debug_uart_ready = board_nucleo_f767zi_init_debug_uart() == BSP_UART_OK;
    const bool debug_uart_rx_ready = debug_uart_ready && (board_nucleo_f767zi_enable_debug_uart_rx_interrupt() == BSP_UART_OK);

    runtime_log_enable_debug_fallback(!log_uart_ready && debug_uart_ready);

    if (!runtime_tick_init())
    {
        if (log_uart_ready || debug_uart_ready)
        {
            runtime_log_write_line(RUNTIME_LOG_LEVEL_ERROR, "SysTick init failed.");
        }

        for (;;)
        {
        }
    }

    last_periodic_tick_ms = runtime_tick_now_ms();

    if (log_uart_ready || debug_uart_ready)
    {
        runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "Booting...");
        runtime_status_log_boot_snapshot();
        runtime_contract_log_current_baseline();
    }

    if (boot_self_test_run_pca9685(&pca9685_device))
    {
        const bool robot_home_self_test_ok = boot_self_test_run_robot_home(&pca9685_device);
        const bool robot_direct_pose_self_test_ok = boot_self_test_run_robot_direct_pose(&pca9685_device);

        robot_self_tests_ok = robot_home_self_test_ok && robot_direct_pose_self_test_ok;

        if (robot_self_tests_ok)
        {
            const robot_startup_status_t robot_startup_status =
                robot_startup_initialize(&robot, &pca9685_device);

            if (robot_startup_status == ROBOT_STARTUP_OK)
            {
                robot_initialized = true;
                if (boot_self_test_restore_preserved_robot_outputs(&robot))
                {
                    robot_ready = true;
                    runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "Restored robot holding pose from active PCA9685 outputs.");
                    runtime_log_write_line(RUNTIME_LOG_LEVEL_INFO, "Controller ready without startup motion.");
                }
                else
                {
                    runtime_log_write_line(RUNTIME_LOG_LEVEL_WARNING, "Robot runtime initialized without startup motion.");
                    runtime_log_write_line(RUNTIME_LOG_LEVEL_WARNING, "Controller not ready. Place the arm physically at HOME and type HOME once to establish the startup reference without motion.");
                }
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
        else if (robot_initialized)
        {
            runtime_log_write_line(RUNTIME_LOG_LEVEL_WARNING, "USART6 RX command shell ready. Place the arm physically at HOME and type HOME once to establish the startup reference without motion.");
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

    if (runtime_status_has_fault_record())
    {
        runtime_led_set_state(&runtime_led, RUNTIME_LED_STATE_FAULT_LATCHED);
    }
    else if (robot_ready)
    {
        runtime_led_set_state(&runtime_led, RUNTIME_LED_STATE_READY_IDLE);
    }
    else
    {
        runtime_led_set_state(&runtime_led, RUNTIME_LED_STATE_DEGRADED);
    }

    for (;;)
    {
        const uint32_t now_ms = runtime_tick_now_ms();

        if (debug_command_runtime_has_pending_work(debug_uart_rx_ready))
        {
            debug_command_runtime_process_input(
                debug_uart_rx_ready,
                &robot_ready,
                robot_initialized ? &robot : 0,
                &pca9685_device);
        }

        if (runtime_tick_periodic_due(now_ms, RUNTIME_CONTRACT_MAIN_SERVICE_INTERVAL_MS, &last_periodic_tick_ms))
        {
            const runtime_led_state_t desired_led_state = runtime_status_has_fault_record()
                ? RUNTIME_LED_STATE_FAULT_LATCHED
                : (robot_ready ? RUNTIME_LED_STATE_READY_IDLE : RUNTIME_LED_STATE_DEGRADED);

            debug_command_runtime_service_motion(robot_ready, robot_initialized ? &robot : 0, &pca9685_device);

            if (runtime_led.state != desired_led_state)
            {
                runtime_led_set_state(&runtime_led, desired_led_state);
            }

            runtime_led_tick(&runtime_led);
        }
    }
}
