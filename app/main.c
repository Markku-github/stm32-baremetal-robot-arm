/**
 ******************************************************************************
 * @file    main.c
 * @brief   V0 application entry point for board bring-up, PCA9685 probing, and USART6 echo testing
 ******************************************************************************
 */

#include <stdbool.h>
#include <stdint.h>

#include "board_nucleo_f767zi.h"
#include "pca9685.h"

#define MAIN_LOOP_DELAY_CYCLES 20000U
#define MAIN_LED_TOGGLE_TICKS 100U
#define PCA9685_SELF_TEST_FREQUENCY_HZ 50U
#define PCA9685_SELF_TEST_CHANNEL 0U
#define PCA9685_SELF_TEST_PULSE_US 1500U

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

static void run_pca9685_smoke_test(bool debug_uart_ready)
{
    pca9685_device_t device;
    uint8_t mode1_value;
    uint8_t mode2_value;
    uint8_t prescale_value;
    uint8_t led_on_low;
    uint8_t led_on_high;
    uint8_t led_off_low;
    uint8_t led_off_high;
    uint16_t on_count;
    uint16_t off_count;
    uint16_t expected_off_count;

    if (!debug_uart_ready)
    {
        return;
    }

    if (board_nucleo_f767zi_init_pca9685_i2c() != BSP_I2C_OK)
    {
        board_nucleo_f767zi_write_debug_string("I2C1 init failed for PCA9685 smoke test.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("I2C1 ready. Running PCA9685 driver self-test at 0x40...\r\n");

    if (pca9685_init(&device, BSP_I2C_INSTANCE_I2C1, PCA9685_I2C_ADDRESS_DEFAULT) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 init failed. Check address, pull-ups, and wiring.\r\n");
        return;
    }

    if (pca9685_set_pwm_frequency(&device, PCA9685_SELF_TEST_FREQUENCY_HZ) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 frequency setup failed.\r\n");
        return;
    }

    if (pca9685_set_channel_pulse_us(&device, PCA9685_SELF_TEST_CHANNEL, PCA9685_SELF_TEST_PULSE_US) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 pulse-width write failed.\r\n");
        return;
    }

    if ((pca9685_read_register(device.instance, device.address, PCA9685_REGISTER_MODE1, &mode1_value) != PCA9685_OK)
        || (pca9685_read_register(device.instance, device.address, PCA9685_REGISTER_MODE2, &mode2_value) != PCA9685_OK)
        || (pca9685_read_register(device.instance, device.address, PCA9685_REGISTER_PRESCALE, &prescale_value) != PCA9685_OK)
        || (pca9685_read_register(device.instance, device.address, PCA9685_REGISTER_LED0_ON_L, &led_on_low) != PCA9685_OK)
        || (pca9685_read_register(device.instance, device.address, (uint8_t)(PCA9685_REGISTER_LED0_ON_L + 1U), &led_on_high) != PCA9685_OK)
        || (pca9685_read_register(device.instance, device.address, (uint8_t)(PCA9685_REGISTER_LED0_ON_L + 2U), &led_off_low) != PCA9685_OK)
        || (pca9685_read_register(device.instance, device.address, (uint8_t)(PCA9685_REGISTER_LED0_ON_L + 3U), &led_off_high) != PCA9685_OK))
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 readback failed after self-test writes.\r\n");
        return;
    }

    on_count = (uint16_t)(((uint16_t)led_on_high << 8U) | led_on_low);
    off_count = (uint16_t)(((uint16_t)led_off_high << 8U) | led_off_low);
    expected_off_count = pca9685_self_test_expected_off_count(device.pwm_frequency_hz, PCA9685_SELF_TEST_PULSE_US);

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
        return;
    }

    board_nucleo_f767zi_write_debug_string("PCA9685 driver self-test OK. External servo power is not required for this register-level check.\r\n");
}

/**
 * @brief  Drain and echo received bytes from the debug UART ring buffer
 * @param  debug_uart_rx_ready: true when USART6 RX interrupts were enabled
 * @retval None
 * @note   Line handling stays in thread context so the interrupt handler only
 *         captures received bytes.
 */
static void process_debug_uart_input(bool debug_uart_rx_ready)
{
    static bool previous_byte_was_carriage_return = false;

    if (!debug_uart_rx_ready)
    {
        return;
    }

    if (board_nucleo_f767zi_debug_uart_overflowed())
    {
        board_nucleo_f767zi_clear_debug_uart_overflow();
        board_nucleo_f767zi_write_debug_string("\r\n[RX overflow]\r\n> ");
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
            board_nucleo_f767zi_write_debug_string("\r\n[RX read error]\r\n> ");
            previous_byte_was_carriage_return = false;
            return;
        }

        if (received_byte == '\r')
        {
            board_nucleo_f767zi_write_debug_string("\r\n> ");
            previous_byte_was_carriage_return = true;
            continue;
        }

        if (received_byte == '\n')
        {
            if (!previous_byte_was_carriage_return)
            {
                board_nucleo_f767zi_write_debug_string("\r\n> ");
            }

            previous_byte_was_carriage_return = false;
            continue;
        }

        previous_byte_was_carriage_return = false;

        if ((received_byte == 0x08U) || (received_byte == 0x7FU))
        {
            board_nucleo_f767zi_write_debug_string("\b \b");
            continue;
        }

        (void)board_nucleo_f767zi_write_debug_byte(received_byte);
    }
}

/**
 * @brief  Initialize the board, verify PCA9685 communication, and run the V0 UART echo loop
 * @retval int  This function does not return during normal operation.
 */
int main(void)
{
    uint32_t led_tick_counter = 0U;

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
        run_pca9685_smoke_test(debug_uart_ready);

        if (debug_uart_rx_ready)
        {
            board_nucleo_f767zi_write_debug_string("USART6 RX echo ready. Type into the terminal.\r\n> ");
        }
        else
        {
            board_nucleo_f767zi_write_debug_string("USART6 RX interrupt setup failed.\r\n");
        }
    }

    for (;;)
    {
        process_debug_uart_input(debug_uart_rx_ready);
        boot_delay(MAIN_LOOP_DELAY_CYCLES);

        led_tick_counter++;
        if (led_tick_counter >= MAIN_LED_TOGGLE_TICKS)
        {
            board_nucleo_f767zi_toggle_debug_led();
            led_tick_counter = 0U;
        }
    }
}
