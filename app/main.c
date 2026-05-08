/**
 ******************************************************************************
 * @file    main.c
 * @brief   V0 application entry point for board bring-up and USART6 echo testing
 ******************************************************************************
 */

#include <stdbool.h>
#include <stdint.h>

#include "board_nucleo_f767zi.h"
#include "pca9685.h"

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

static void run_pca9685_smoke_test(bool debug_uart_ready)
{
    uint8_t mode1_value;

    if (!debug_uart_ready)
    {
        return;
    }

    if (board_nucleo_f767zi_init_pca9685_i2c() != BSP_I2C_OK)
    {
        board_nucleo_f767zi_write_debug_string("I2C1 init failed for PCA9685 smoke test.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("I2C1 ready. Probing PCA9685 MODE1 register at 0x40...\r\n");

    if (pca9685_probe(BSP_I2C_INSTANCE_I2C1, PCA9685_I2C_ADDRESS_DEFAULT, &mode1_value) != PCA9685_OK)
    {
        board_nucleo_f767zi_write_debug_string("PCA9685 probe failed. Check address, pull-ups, and wiring.\r\n");
        return;
    }

    board_nucleo_f767zi_write_debug_string("PCA9685 MODE1 = 0x");
    write_hex_byte(mode1_value);
    board_nucleo_f767zi_write_debug_string("\r\n");
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
 * @brief  Initialize the board and run the V0 UART echo bring-up loop
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
