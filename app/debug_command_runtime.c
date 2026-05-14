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
#include "pca9685.h"
#include "runtime_status.h"
#include "system_stm32f7xx.h"

#define DEBUG_COMMAND_RUNTIME_PCA9685_PWM_FREQUENCY_HZ 50U
#define DEBUG_COMMAND_RUNTIME_BUSY_WAIT_DIVISOR 4000U

typedef struct
{
    pca9685_device_t *device;
} debug_command_runtime_recovery_context_t;

static const debug_command_handler_io_t debug_command_handler_io = {
    .write_string = debug_console_write_string_adapter,
    .write_byte = debug_console_write_byte_adapter,
    .write_prompt = debug_console_write_prompt_adapter,
};

static void debug_command_runtime_busy_wait_cycles(volatile uint32_t cycles)
{
    while (cycles > 0U)
    {
        cycles--;
    }
}

static void debug_command_runtime_delay_ms(void *context, uint32_t delay_ms)
{
    uint32_t milliseconds_remaining = delay_ms;
    uint32_t cycles_per_millisecond = SystemCoreClock / DEBUG_COMMAND_RUNTIME_BUSY_WAIT_DIVISOR;

    (void)context;

    if (cycles_per_millisecond == 0U)
    {
        cycles_per_millisecond = 1U;
    }

    while (milliseconds_remaining > 0U)
    {
        debug_command_runtime_busy_wait_cycles(cycles_per_millisecond);
        milliseconds_remaining--;
    }
}

static bool debug_command_runtime_recover_robot(void *context, robot_arm_t *robot)
{
    debug_command_runtime_recovery_context_t *recovery_context = (debug_command_runtime_recovery_context_t *)context;
    pca9685_device_t *device;
    uint16_t pwm_frequency_hz;

    if ((recovery_context == 0) || (robot == 0) || (recovery_context->device == 0))
    {
        return false;
    }

    device = recovery_context->device;
    pwm_frequency_hz = device->pwm_frequency_hz;
    if (pwm_frequency_hz == 0U)
    {
        pwm_frequency_hz = DEBUG_COMMAND_RUNTIME_PCA9685_PWM_FREQUENCY_HZ;
    }

    if (board_nucleo_f767zi_init_pca9685_i2c() != BSP_I2C_OK)
    {
        return false;
    }

    if (pca9685_init(device, device->instance, device->address) != PCA9685_OK)
    {
        return false;
    }

    if (pca9685_set_pwm_frequency(device, pwm_frequency_hz) != PCA9685_OK)
    {
        return false;
    }

    return robot_arm_init(robot, device) == ROBOT_ARM_OK;
}

static void debug_command_shell_execute(void *context, const char *command_line)
{
    debug_command_handler_context_t *execution_context = (debug_command_handler_context_t *)context;

    if (execution_context == 0)
    {
        return;
    }

    debug_command_handler_execute(command_line, execution_context, &debug_command_handler_io, 0);
}

static bool debug_command_runtime_trigger_usage_fault(void *context)
{
    (void)context;

    runtime_status_capture_fault_and_reset(RUNTIME_STATUS_FAULT_USAGEFAULT);
    return true;
}

static const debug_command_shell_io_t debug_command_shell_io = {
    .write_string = debug_console_write_string_adapter,
    .write_byte = debug_console_write_byte_adapter,
    .write_prompt = debug_console_write_prompt_adapter,
    .execute_command = debug_command_shell_execute,
};

void debug_command_runtime_process_input(
    bool debug_uart_rx_ready,
    bool robot_ready,
    robot_arm_t *robot,
    pca9685_device_t *pca9685_device)
{
    static debug_command_shell_t command_shell;
    debug_command_handler_context_t execution_context;
    debug_command_runtime_recovery_context_t recovery_context;

    if (!debug_uart_rx_ready)
    {
        return;
    }

    recovery_context.device = pca9685_device;
    execution_context.robot_ready = robot_ready;
    execution_context.robot = robot_ready ? robot : 0;
    execution_context.recover_robot = robot_ready ? debug_command_runtime_recover_robot : 0;
    execution_context.recover_context = robot_ready ? &recovery_context : 0;
    execution_context.delay_ms = robot_ready ? debug_command_runtime_delay_ms : 0;
    execution_context.delay_context = 0;
    execution_context.trigger_fault = debug_command_runtime_trigger_usage_fault;
    execution_context.trigger_fault_context = 0;

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