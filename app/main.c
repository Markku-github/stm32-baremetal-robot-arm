#include <stdbool.h>
#include <stdint.h>

#include "board_nucleo_f767zi.h"

#define MAIN_LOOP_DELAY_CYCLES 20000U
#define MAIN_LED_TOGGLE_TICKS 100U

static void boot_delay(volatile uint32_t cycles)
{
    while (cycles > 0U)
    {
        cycles--;
    }
}

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
