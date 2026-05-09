/**
 ******************************************************************************
 * @file    main.c
 * @brief   Early application entry point for board bring-up, controller self-tests, and USART6 command-shell handling
 ******************************************************************************
 */

#include <stdbool.h>
#include <stdint.h>

#include "board_nucleo_f767zi.h"
#include "pca9685.h"
#include "robot_arm.h"

#define MAIN_LOOP_DELAY_CYCLES 20000U
#define MAIN_LED_TOGGLE_TICKS 100U
#define PCA9685_SELF_TEST_FREQUENCY_HZ 50U
#define PCA9685_SELF_TEST_CHANNEL 0U
#define PCA9685_SELF_TEST_PULSE_US 1500U
#define MAIN_DEGREES_TO_RADIANS 0.01745329251994329577f
#define MAIN_RADIANS_TO_DEGREES 57.2957795130823208768f
#define MAIN_COMMAND_BUFFER_CAPACITY 64U
#define ROBOT_DIRECT_POSE_BASE_DEG 0.0f
#define ROBOT_DIRECT_POSE_SHOULDER_DEG 10.0f
#define ROBOT_DIRECT_POSE_ELBOW_DEG -10.0f
#define ROBOT_DIRECT_POSE_WRIST_TILT_DEG 5.0f
#define ROBOT_DIRECT_POSE_WRIST_ROTATE_DEG -15.0f
#define ROBOT_DIRECT_POSE_GRIPPER_DEG 10.0f

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

static void write_hex_nibble(uint8_t nibble)
{
    const uint8_t character = (nibble < 10U) ? (uint8_t)('0' + nibble) : (uint8_t)('A' + (nibble - 10U));

    (void)board_nucleo_f767zi_write_debug_byte(character);
}

static void write_hex_byte(uint8_t value)
{
    write_hex_nibble((uint8_t)(value >> 4U));
    write_hex_nibble((uint8_t)(value & 0x0FU));
}

static void write_hex_word(uint16_t value)
{
    write_hex_byte((uint8_t)(value >> 8U));
    write_hex_byte((uint8_t)(value & 0x00FFU));
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

static float radians_to_degrees(float radians)
{
    return radians * MAIN_RADIANS_TO_DEGREES;
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

static uint8_t trim_command_line(char *line, uint8_t length)
{
    uint8_t start_index = 0U;
    uint8_t end_index = length;
    uint8_t trimmed_length;
    uint8_t index;

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

static bool command_matches_name_with_arguments(
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

static bool parse_signed_int32_token(const char **cursor, int32_t *value)
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

static void write_prompt(void)
{
    board_nucleo_f767zi_write_debug_string("> ");
}

static void write_unsigned_decimal(uint32_t value)
{
    char digits[10];
    uint8_t digit_count = 0U;

    if (value == 0U)
    {
        (void)board_nucleo_f767zi_write_debug_byte((uint8_t)'0');
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
        (void)board_nucleo_f767zi_write_debug_byte((uint8_t)digits[digit_count]);
    }
}

static void write_signed_decimal(int32_t value)
{
    uint32_t magnitude;

    if (value < 0)
    {
        (void)board_nucleo_f767zi_write_debug_byte((uint8_t)'-');
        magnitude = (uint32_t)(-value);
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    write_unsigned_decimal(magnitude);
}

static int32_t round_float_to_int32(float value)
{
    if (value < 0.0f)
    {
        return (int32_t)(value - 0.5f);
    }

    return (int32_t)(value + 0.5f);
}

static void write_joint_status_line(const char *joint_name, float angle_rad)
{
    board_nucleo_f767zi_write_debug_string(joint_name);
    board_nucleo_f767zi_write_debug_string("=");
    write_signed_decimal(round_float_to_int32(radians_to_degrees(angle_rad)));
    board_nucleo_f767zi_write_debug_string(" deg\r\n");
}

static void write_help_text(void)
{
    board_nucleo_f767zi_write_debug_string("Commands:\r\n");
    board_nucleo_f767zi_write_debug_string("HELP\r\n");
    board_nucleo_f767zi_write_debug_string("HOME\r\n");
    board_nucleo_f767zi_write_debug_string("POSE <base_deg> <shoulder_deg> <elbow_deg> <wrist_tilt_deg> <wrist_rotate_deg> <gripper_deg>\r\n");
    board_nucleo_f767zi_write_debug_string("STATUS\r\n");
}

static void write_command_ok(const char *command_name)
{
    board_nucleo_f767zi_write_debug_string("OK ");
    board_nucleo_f767zi_write_debug_string(command_name);
    board_nucleo_f767zi_write_debug_string("\r\n");
}

static bool parse_pose_arguments(const char *arguments, robot_arm_pose_t *pose)
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

    if (!parse_signed_int32_token(&cursor, &base_deg)
        || !parse_signed_int32_token(&cursor, &shoulder_deg)
        || !parse_signed_int32_token(&cursor, &elbow_deg)
        || !parse_signed_int32_token(&cursor, &wrist_tilt_deg)
        || !parse_signed_int32_token(&cursor, &wrist_rotate_deg)
        || !parse_signed_int32_token(&cursor, &gripper_deg)
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

static void write_status_text(const robot_arm_t *robot)
{
    robot_arm_pose_t pose;

    if ((robot == 0) || (robot_arm_get_current_pose(robot, &pose) != ROBOT_ARM_OK))
    {
        board_nucleo_f767zi_write_debug_string("ERR CONTROLLER_NOT_READY\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("STATUS\r\n");
    write_joint_status_line("base", pose.base_rad);
    write_joint_status_line("shoulder", pose.shoulder_rad);
    write_joint_status_line("elbow", pose.elbow_rad);
    write_joint_status_line("wrist_tilt", pose.wrist_tilt_rad);
    write_joint_status_line("wrist_rotate", pose.wrist_rotate_rad);
    write_joint_status_line("gripper", pose.gripper_rad);
}

static void execute_debug_command(const char *command_line, bool robot_ready, robot_arm_t *robot)
{
    const char *arguments = 0;

    if ((command_line == 0) || (command_line[0] == '\0'))
    {
        write_prompt();
        return;
    }

    if (command_matches_name_with_arguments(command_line, "HELP", &arguments))
    {
        if ((arguments != 0) && (arguments[0] != '\0'))
        {
            board_nucleo_f767zi_write_debug_string("ERR INVALID_ARGUMENT\r\n");
        }
        else
        {
            write_help_text();
        }

        write_prompt();
        return;
    }

    if (command_matches_name_with_arguments(command_line, "STATUS", &arguments))
    {
        if ((arguments != 0) && (arguments[0] != '\0'))
        {
            board_nucleo_f767zi_write_debug_string("ERR INVALID_ARGUMENT\r\n");
        }
        else if (!robot_ready)
        {
            board_nucleo_f767zi_write_debug_string("ERR CONTROLLER_NOT_READY\r\n");
        }
        else
        {
            write_status_text(robot);
        }

        write_prompt();
        return;
    }

    if (command_matches_name_with_arguments(command_line, "HOME", &arguments))
    {
        if ((arguments != 0) && (arguments[0] != '\0'))
        {
            board_nucleo_f767zi_write_debug_string("ERR INVALID_ARGUMENT\r\n");
        }
        else if (!robot_ready || (robot == 0))
        {
            board_nucleo_f767zi_write_debug_string("ERR CONTROLLER_NOT_READY\r\n");
        }
        else if (robot_arm_home(robot) != ROBOT_ARM_OK)
        {
            board_nucleo_f767zi_write_debug_string("ERR COMMAND_FAILED\r\n");
        }
        else
        {
            write_command_ok("HOME");
        }

        write_prompt();
        return;
    }

    if (command_matches_name_with_arguments(command_line, "POSE", &arguments))
    {
        robot_arm_pose_t pose;

        if (!robot_ready || (robot == 0))
        {
            board_nucleo_f767zi_write_debug_string("ERR CONTROLLER_NOT_READY\r\n");
        }
        else if (!parse_pose_arguments(arguments, &pose))
        {
            board_nucleo_f767zi_write_debug_string("ERR INVALID_ARGUMENT\r\n");
        }
        else if (robot_arm_set_pose_immediate(robot, &pose) != ROBOT_ARM_OK)
        {
            board_nucleo_f767zi_write_debug_string("ERR COMMAND_FAILED\r\n");
        }
        else
        {
            write_command_ok("POSE");
        }

        write_prompt();
        return;
    }

    board_nucleo_f767zi_write_debug_string("ERR UNKNOWN_COMMAND\r\n");
    write_prompt();
}

static void finalize_debug_command(
    char *command_buffer,
    uint8_t *command_length,
    bool *command_overflowed,
    bool robot_ready,
    robot_arm_t *robot)
{
    uint8_t trimmed_length;

    if ((command_buffer == 0) || (command_length == 0) || (command_overflowed == 0))
    {
        return;
    }

    if (*command_overflowed)
    {
        board_nucleo_f767zi_write_debug_string("ERR COMMAND_TOO_LONG\r\n");
        *command_length = 0U;
        *command_overflowed = false;
        command_buffer[0] = '\0';
        write_prompt();
        return;
    }

    command_buffer[*command_length] = '\0';
    trimmed_length = trim_command_line(command_buffer, *command_length);
    *command_length = 0U;
    execute_debug_command((trimmed_length > 0U) ? command_buffer : "", robot_ready, robot);
}

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
    write_hex_byte(mode1_value);
    board_nucleo_f767zi_write_debug_string(", MODE2 = 0x");
    write_hex_byte(mode2_value);
    board_nucleo_f767zi_write_debug_string("\r\n");

    board_nucleo_f767zi_write_debug_string("PCA9685 PRESCALE = 0x");
    write_hex_byte(prescale_value);
    board_nucleo_f767zi_write_debug_string(" (expected about 0x79 for 50 Hz @ 25 MHz)\r\n");

    board_nucleo_f767zi_write_debug_string("PCA9685 CH0 ON = 0x");
    write_hex_word(on_count);
    board_nucleo_f767zi_write_debug_string(", OFF = 0x");
    write_hex_word(off_count);
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
    uint8_t joint_index;

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
        (void)pca9685_disable_all_outputs(device);
        board_nucleo_f767zi_write_debug_string("Robot HOME command failed.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("Robot HOME OFF counts: ");

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        float home_angle_rad;
        uint16_t pulse_width_us;
        uint16_t expected_off_count;
        uint16_t on_count;
        uint16_t off_count;
        const robot_arm_joint_id_t joint_id = (robot_arm_joint_id_t)joint_index;
        const servo_t *servo = robot_arm_get_servo_const(&robot, joint_id);

        if ((servo == 0)
            || (robot_arm_get_home_angle_rad(&robot, joint_id, &home_angle_rad) != ROBOT_ARM_OK)
            || (servo_angle_rad_to_pulse_us(servo, home_angle_rad, &pulse_width_us) != SERVO_OK)
            || !read_pca9685_channel_counts(device, servo->channel, &on_count, &off_count))
        {
            (void)pca9685_disable_all_outputs(device);
            board_nucleo_f767zi_write_debug_string("Robot HOME readback failed.\r\n");
            return;
        }

        expected_off_count = pca9685_self_test_expected_off_count(device->pwm_frequency_hz, pulse_width_us);
        if ((on_count != 0U) || (off_count != expected_off_count))
        {
            (void)pca9685_disable_all_outputs(device);
            board_nucleo_f767zi_write_debug_string("Robot HOME register readback mismatch.\r\n");
            return;
        }

        if (joint_index > 0U)
        {
            board_nucleo_f767zi_write_debug_string(", ");
        }

        board_nucleo_f767zi_write_debug_string(servo->name);
        board_nucleo_f767zi_write_debug_string("=0x");
        write_hex_word(off_count);
    }

    board_nucleo_f767zi_write_debug_string("\r\n");

    if (pca9685_disable_all_outputs(device) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("Robot HOME output disable failed after self-test.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("Robot HOME integration self-test OK. Outputs returned to the disabled state. External servo power is still not required for this register-level check.\r\n");
}

static void run_robot_direct_pose_self_test(bool debug_uart_ready, pca9685_device_t *device)
{
    robot_arm_t robot;
    robot_arm_pose_t pose;
    robot_arm_pose_t current_pose;
    uint8_t joint_index;

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

    pose.base_rad = degrees_to_radians(ROBOT_DIRECT_POSE_BASE_DEG);
    pose.shoulder_rad = degrees_to_radians(ROBOT_DIRECT_POSE_SHOULDER_DEG);
    pose.elbow_rad = degrees_to_radians(ROBOT_DIRECT_POSE_ELBOW_DEG);
    pose.wrist_tilt_rad = degrees_to_radians(ROBOT_DIRECT_POSE_WRIST_TILT_DEG);
    pose.wrist_rotate_rad = degrees_to_radians(ROBOT_DIRECT_POSE_WRIST_ROTATE_DEG);
    pose.gripper_rad = degrees_to_radians(ROBOT_DIRECT_POSE_GRIPPER_DEG);

    if (robot_arm_set_pose_immediate(&robot, &pose) != ROBOT_ARM_OK)
    {
        (void)pca9685_disable_all_outputs(device);
        board_nucleo_f767zi_write_debug_string("Robot direct pose command failed.\r\n");
        return;
    }

    if (robot_arm_get_current_pose(&robot, &current_pose) != ROBOT_ARM_OK)
    {
        (void)pca9685_disable_all_outputs(device);
        board_nucleo_f767zi_write_debug_string("Robot current pose readback failed.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("Robot direct pose OFF counts: ");

    for (joint_index = 0U; joint_index < (uint8_t)ROBOT_ARM_JOINT_COUNT; joint_index++)
    {
        uint16_t pulse_width_us;
        uint16_t expected_off_count;
        uint16_t on_count;
        uint16_t off_count;
        const robot_arm_joint_id_t joint_id = (robot_arm_joint_id_t)joint_index;
        const servo_t *servo = robot_arm_get_servo_const(&robot, joint_id);

        if ((servo == 0)
            || (servo_angle_rad_to_pulse_us(servo, robot_pose_joint_angle(&current_pose, joint_id), &pulse_width_us) != SERVO_OK)
            || !read_pca9685_channel_counts(device, servo->channel, &on_count, &off_count))
        {
            (void)pca9685_disable_all_outputs(device);
            board_nucleo_f767zi_write_debug_string("Robot direct pose readback failed.\r\n");
            return;
        }

        expected_off_count = pca9685_self_test_expected_off_count(device->pwm_frequency_hz, pulse_width_us);
        if ((on_count != 0U) || (off_count != expected_off_count))
        {
            (void)pca9685_disable_all_outputs(device);
            board_nucleo_f767zi_write_debug_string("Robot direct pose register readback mismatch.\r\n");
            return;
        }

        if (joint_index > 0U)
        {
            board_nucleo_f767zi_write_debug_string(", ");
        }

        board_nucleo_f767zi_write_debug_string(servo->name);
        board_nucleo_f767zi_write_debug_string("=0x");
        write_hex_word(off_count);
    }

    board_nucleo_f767zi_write_debug_string("\r\n");

    if (pca9685_disable_all_outputs(device) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("Robot direct pose output disable failed after self-test.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("Robot direct pose integration self-test OK. Outputs returned to the disabled state. External servo power is still not required for this register-level check.\r\n");
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
    static char command_buffer[MAIN_COMMAND_BUFFER_CAPACITY];
    static uint8_t command_length = 0U;
    static bool command_overflowed = false;
    static bool previous_byte_was_carriage_return = false;

    if (!debug_uart_rx_ready)
    {
        return;
    }

    if (board_nucleo_f767zi_debug_uart_overflowed())
    {
        board_nucleo_f767zi_clear_debug_uart_overflow();
        command_length = 0U;
        command_overflowed = false;
        command_buffer[0] = '\0';
        board_nucleo_f767zi_write_debug_string("\r\n[RX overflow]\r\n");
        write_prompt();
        previous_byte_was_carriage_return = false;
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
            command_length = 0U;
            command_overflowed = false;
            command_buffer[0] = '\0';
            board_nucleo_f767zi_write_debug_string("\r\n[RX read error]\r\n");
            write_prompt();
            previous_byte_was_carriage_return = false;
            return;
        }

        if (received_byte == '\r')
        {
            board_nucleo_f767zi_write_debug_string("\r\n");
            finalize_debug_command(command_buffer, &command_length, &command_overflowed, robot_ready, robot);
            previous_byte_was_carriage_return = true;
            continue;
        }

        if (received_byte == '\n')
        {
            if (!previous_byte_was_carriage_return)
            {
                board_nucleo_f767zi_write_debug_string("\r\n");
                finalize_debug_command(command_buffer, &command_length, &command_overflowed, robot_ready, robot);
            }

            previous_byte_was_carriage_return = false;
            continue;
        }

        previous_byte_was_carriage_return = false;

        if ((received_byte == 0x08U) || (received_byte == 0x7FU))
        {
            if (!command_overflowed && (command_length > 0U))
            {
                command_length--;
                command_buffer[command_length] = '\0';
                board_nucleo_f767zi_write_debug_string("\b \b");
            }

            continue;
        }

        if ((received_byte < 0x20U) || (received_byte > 0x7EU))
        {
            continue;
        }

        if (command_overflowed)
        {
            continue;
        }

        if (command_length >= (MAIN_COMMAND_BUFFER_CAPACITY - 1U))
        {
            command_overflowed = true;
            continue;
        }

        command_buffer[command_length] = (char)received_byte;
        command_length++;
        command_buffer[command_length] = '\0';

        (void)board_nucleo_f767zi_write_debug_byte(received_byte);
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
            write_prompt();
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
