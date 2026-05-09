/**
 ******************************************************************************
 * @file    main.c
 * @brief   Early application entry point for board bring-up, controller self-tests, and USART6 command-shell handling
 ******************************************************************************
 */

#include <stdbool.h>
#include <stdint.h>

#include "board_nucleo_f767zi.h"
#include "debug_console.h"
#include "debug_command_handler.h"
#include "debug_command_shell.h"
#include "pca9685.h"
#include "robot_arm.h"

#define MAIN_LOOP_DELAY_CYCLES 20000U
#define MAIN_LED_TOGGLE_TICKS 100U
#define PCA9685_SELF_TEST_FREQUENCY_HZ 50U
#define PCA9685_SELF_TEST_CHANNEL 0U
#define PCA9685_SELF_TEST_PULSE_US 1500U
#define MAIN_DEGREES_TO_RADIANS 0.01745329251994329577f
#define ROBOT_DIRECT_POSE_BASE_DEG 0.0f
#define ROBOT_DIRECT_POSE_SHOULDER_DEG 10.0f
#define ROBOT_DIRECT_POSE_ELBOW_DEG -10.0f
#define ROBOT_DIRECT_POSE_WRIST_TILT_DEG 5.0f
#define ROBOT_DIRECT_POSE_WRIST_ROTATE_DEG -15.0f
#define ROBOT_DIRECT_POSE_GRIPPER_DEG 10.0f

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

static uint16_t pca9685_self_test_expected_off_count(uint16_t pwm_frequency_hz, uint16_t pulse_width_us)
{
    const uint64_t pulse_counts = ((uint64_t)pulse_width_us * (uint64_t)pwm_frequency_hz * (uint64_t)PCA9685_PWM_STEPS + 500000ULL)
        / 1000000ULL;

    return (uint16_t)pulse_counts;
}

static float degrees_to_radians(float degrees)
{
    return degrees * MAIN_DEGREES_TO_RADIANS;
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

static bool read_pca9685_channel_counts(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t *on_count,
    uint16_t *off_count);

static float robot_pose_joint_angle(const robot_arm_pose_t *pose, robot_arm_joint_id_t joint_id)
{
    switch (joint_id)
    {
        case ROBOT_ARM_JOINT_BASE:
            return pose->base_rad;

        case ROBOT_ARM_JOINT_SHOULDER:
            return pose->shoulder_rad;

        case ROBOT_ARM_JOINT_ELBOW:
            return pose->elbow_rad;

        case ROBOT_ARM_JOINT_WRIST_TILT:
            return pose->wrist_tilt_rad;

        case ROBOT_ARM_JOINT_WRIST_ROTATE:
            return pose->wrist_rotate_rad;

        case ROBOT_ARM_JOINT_GRIPPER:
            return pose->gripper_rad;

        default:
            return 0.0f;
    }
}

typedef enum
{
    ROBOT_POSE_READBACK_OK = 0,
    ROBOT_POSE_READBACK_ERR_READ,
    ROBOT_POSE_READBACK_ERR_MISMATCH,
} robot_pose_readback_status_t;

static void report_robot_self_test_failure(const pca9685_device_t *device, const char *message)
{
    if (device != 0)
    {
        (void)pca9685_disable_all_outputs(device);
    }

    board_nucleo_f767zi_write_debug_string(message);
}

static void report_robot_pose_readback_failure(
    const pca9685_device_t *device,
    robot_pose_readback_status_t readback_status,
    const char *read_error_message,
    const char *mismatch_message)
{
    if (readback_status == ROBOT_POSE_READBACK_ERR_MISMATCH)
    {
        report_robot_self_test_failure(device, mismatch_message);
        return;
    }

    report_robot_self_test_failure(device, read_error_message);
}

static bool finalize_robot_self_test(
    const pca9685_device_t *device,
    const char *output_disable_failure_message,
    const char *success_message)
{
    if ((device == 0) || (pca9685_disable_all_outputs(device) != PCA9685_OK))
    {
        board_nucleo_f767zi_write_debug_string(output_disable_failure_message);
        return false;
    }

    board_nucleo_f767zi_write_debug_string(success_message);
    return true;
}

static void build_robot_direct_pose(robot_arm_pose_t *pose)
{
    if (pose == 0)
    {
        return;
    }

    pose->base_rad = degrees_to_radians(ROBOT_DIRECT_POSE_BASE_DEG);
    pose->shoulder_rad = degrees_to_radians(ROBOT_DIRECT_POSE_SHOULDER_DEG);
    pose->elbow_rad = degrees_to_radians(ROBOT_DIRECT_POSE_ELBOW_DEG);
    pose->wrist_tilt_rad = degrees_to_radians(ROBOT_DIRECT_POSE_WRIST_TILT_DEG);
    pose->wrist_rotate_rad = degrees_to_radians(ROBOT_DIRECT_POSE_WRIST_ROTATE_DEG);
    pose->gripper_rad = degrees_to_radians(ROBOT_DIRECT_POSE_GRIPPER_DEG);
}

static robot_pose_readback_status_t write_robot_pose_off_counts(
    const robot_arm_t *robot,
    const robot_arm_pose_t *pose,
    const pca9685_device_t *device)
{
    uint8_t joint_index;

    if ((robot == 0) || (pose == 0) || (device == 0))
    {
        return ROBOT_POSE_READBACK_ERR_READ;
    }

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        uint16_t pulse_width_us;
        uint16_t expected_off_count;
        uint16_t on_count;
        uint16_t off_count;
        const robot_arm_joint_id_t joint_id = (robot_arm_joint_id_t)joint_index;
        const servo_t *servo = robot_arm_get_servo_const(robot, joint_id);

        if ((servo == 0)
            || (servo_angle_rad_to_pulse_us(servo, robot_pose_joint_angle(pose, joint_id), &pulse_width_us) != SERVO_OK)
            || !read_pca9685_channel_counts(device, servo->channel, &on_count, &off_count))
        {
            return ROBOT_POSE_READBACK_ERR_READ;
        }

        expected_off_count = pca9685_self_test_expected_off_count(device->pwm_frequency_hz, pulse_width_us);
        if ((on_count != 0U) || (off_count != expected_off_count))
        {
            return ROBOT_POSE_READBACK_ERR_MISMATCH;
        }

        if (joint_index > 0U)
        {
            board_nucleo_f767zi_write_debug_string(", ");
        }

        board_nucleo_f767zi_write_debug_string(servo->name);
        board_nucleo_f767zi_write_debug_string("=0x");
        debug_console_write_hex_word(off_count);
    }

    board_nucleo_f767zi_write_debug_string("\r\n");
    return ROBOT_POSE_READBACK_OK;
}

static bool read_pca9685_channel_counts(
    const pca9685_device_t *device,
    uint8_t channel,
    uint16_t *on_count,
    uint16_t *off_count)
{
    uint8_t register_base;
    uint8_t led_on_low;
    uint8_t led_on_high;
    uint8_t led_off_low;
    uint8_t led_off_high;

    if ((device == 0) || (on_count == 0) || (off_count == 0) || (channel >= PCA9685_CHANNEL_COUNT))
    {
        return false;
    }

    register_base = (uint8_t)(PCA9685_REGISTER_LED0_ON_L + (channel * 4U));

    if ((pca9685_read_register(device->instance, device->address, register_base, &led_on_low) != PCA9685_OK)
        || (pca9685_read_register(device->instance, device->address, (uint8_t)(register_base + 1U), &led_on_high) != PCA9685_OK)
        || (pca9685_read_register(device->instance, device->address, (uint8_t)(register_base + 2U), &led_off_low) != PCA9685_OK)
        || (pca9685_read_register(device->instance, device->address, (uint8_t)(register_base + 3U), &led_off_high) != PCA9685_OK))
    {
        return false;
    }

    *on_count = (uint16_t)((((uint16_t)led_on_high & 0x0FU) << 8U) | led_on_low);
    *off_count = (uint16_t)((((uint16_t)led_off_high & 0x0FU) << 8U) | led_off_low);
    return true;
}

static bool run_pca9685_self_test(bool debug_uart_ready, pca9685_device_t *device)
{
    uint8_t mode1_value;
    uint8_t mode2_value;
    uint8_t prescale_value;
    uint16_t on_count;
    uint16_t off_count;
    uint16_t expected_off_count;

    if (!debug_uart_ready || (device == 0))
    {
        return false;
    }

    if (board_nucleo_f767zi_init_pca9685_i2c() != BSP_I2C_OK)
    {
        board_nucleo_f767zi_write_debug_string("I2C1 init failed for PCA9685 self-test.\r\n");
        return false;
    }

    board_nucleo_f767zi_write_debug_string("I2C1 ready. Running PCA9685 driver self-test at 0x40...\r\n");

    if (pca9685_init(device, BSP_I2C_INSTANCE_I2C1, PCA9685_I2C_ADDRESS_DEFAULT) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 init failed. Check address, pull-ups, and wiring.\r\n");
        return false;
    }

    if (pca9685_set_pwm_frequency(device, PCA9685_SELF_TEST_FREQUENCY_HZ) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 frequency setup failed.\r\n");
        return false;
    }

    if (pca9685_set_channel_pulse_us(device, PCA9685_SELF_TEST_CHANNEL, PCA9685_SELF_TEST_PULSE_US) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 pulse-width write failed.\r\n");
        return false;
    }

    if ((pca9685_read_register(device->instance, device->address, PCA9685_REGISTER_MODE1, &mode1_value) != PCA9685_OK)
        || (pca9685_read_register(device->instance, device->address, PCA9685_REGISTER_MODE2, &mode2_value) != PCA9685_OK)
        || (pca9685_read_register(device->instance, device->address, PCA9685_REGISTER_PRESCALE, &prescale_value) != PCA9685_OK)
        || !read_pca9685_channel_counts(device, PCA9685_SELF_TEST_CHANNEL, &on_count, &off_count))
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 readback failed after self-test writes.\r\n");
        return false;
    }

    expected_off_count = pca9685_self_test_expected_off_count(device->pwm_frequency_hz, PCA9685_SELF_TEST_PULSE_US);

    board_nucleo_f767zi_write_debug_string("PCA9685 MODE1 = 0x");
    debug_console_write_hex_byte(mode1_value);
    board_nucleo_f767zi_write_debug_string(", MODE2 = 0x");
    debug_console_write_hex_byte(mode2_value);
    board_nucleo_f767zi_write_debug_string("\r\n");

    board_nucleo_f767zi_write_debug_string("PCA9685 PRESCALE = 0x");
    debug_console_write_hex_byte(prescale_value);
    board_nucleo_f767zi_write_debug_string(" (expected about 0x79 for 50 Hz @ 25 MHz)\r\n");

    board_nucleo_f767zi_write_debug_string("PCA9685 CH0 ON = 0x");
    debug_console_write_hex_word(on_count);
    board_nucleo_f767zi_write_debug_string(", OFF = 0x");
    debug_console_write_hex_word(off_count);
    board_nucleo_f767zi_write_debug_string("\r\n");

    if ((on_count != 0U) || (off_count != expected_off_count))
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 register readback mismatch.\r\n");
        return false;
    }

    if (pca9685_disable_all_outputs(device) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 output disable failed after self-test.\r\n");
        return false;
    }

    board_nucleo_f767zi_write_debug_string("PCA9685 driver self-test OK. Outputs returned to the disabled state. External servo power is not required for this register-level check.\r\n");
    return true;
}

static void run_robot_home_self_test(bool debug_uart_ready, pca9685_device_t *device)
{
    robot_arm_t robot;
    robot_arm_pose_t home_pose;
    robot_pose_readback_status_t readback_status;

    if (!debug_uart_ready || (device == 0))
    {
        return;
    }

    board_nucleo_f767zi_write_debug_string("Running robot HOME integration self-test...\r\n");

    if (robot_arm_init(&robot, device) != ROBOT_ARM_OK)
    {
        board_nucleo_f767zi_write_debug_string("Robot servo baseline init failed.\r\n");
        return;
    }

    if (robot_arm_home(&robot) != ROBOT_ARM_OK)
    {
        report_robot_self_test_failure(device, "Robot HOME command failed.\r\n");
        return;
    }

    if (robot_arm_get_home_pose(&robot, &home_pose) != ROBOT_ARM_OK)
    {
        report_robot_self_test_failure(device, "Robot HOME readback failed.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("Robot HOME OFF counts: ");
    readback_status = write_robot_pose_off_counts(&robot, &home_pose, device);

    if (readback_status != ROBOT_POSE_READBACK_OK)
    {
        report_robot_pose_readback_failure(
            device,
            readback_status,
            "Robot HOME readback failed.\r\n",
            "Robot HOME register readback mismatch.\r\n");
        return;
    }

    (void)finalize_robot_self_test(
        device,
        "Robot HOME output disable failed after self-test.\r\n",
        "Robot HOME integration self-test OK. Outputs returned to the disabled state. External servo power is still not required for this register-level check.\r\n");
}

static void run_robot_direct_pose_self_test(bool debug_uart_ready, pca9685_device_t *device)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;
    robot_pose_readback_status_t readback_status;

    if (!debug_uart_ready || (device == 0))
    {
        return;
    }

    board_nucleo_f767zi_write_debug_string("Running robot direct pose integration self-test...\r\n");

    if (robot_arm_init(&robot, device) != ROBOT_ARM_OK)
    {
        board_nucleo_f767zi_write_debug_string("Robot servo baseline init failed for direct pose test.\r\n");
        return;
    }

    build_robot_direct_pose(&pose);

    if (robot_arm_set_pose_immediate(&robot, &pose) != ROBOT_ARM_OK)
    {
        report_robot_self_test_failure(device, "Robot direct pose command failed.\r\n");
        return;
    }

    if (robot_arm_get_current_pose(&robot, &current_pose) != ROBOT_ARM_OK)
    {
        report_robot_self_test_failure(device, "Robot current pose readback failed.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("Robot direct pose OFF counts: ");
    readback_status = write_robot_pose_off_counts(&robot, &current_pose, device);

    if (readback_status != ROBOT_POSE_READBACK_OK)
    {
        report_robot_pose_readback_failure(
            device,
            readback_status,
            "Robot direct pose readback failed.\r\n",
            "Robot direct pose register readback mismatch.\r\n");
        return;
    }

    (void)finalize_robot_self_test(
        device,
        "Robot direct pose output disable failed after self-test.\r\n",
        "Robot direct pose integration self-test OK. Outputs returned to the disabled state. External servo power is still not required for this register-level check.\r\n");
}

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
        if (run_pca9685_self_test(debug_uart_ready, &pca9685_device))
        {
            run_robot_home_self_test(debug_uart_ready, &pca9685_device);
            run_robot_direct_pose_self_test(debug_uart_ready, &pca9685_device);

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
