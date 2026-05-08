#ifndef BOARD_NUCLEO_F767ZI_H
#define BOARD_NUCLEO_F767ZI_H

#include <stdbool.h>

#include "bsp_gpio.h"
#include "bsp_uart.h"

bsp_gpio_status_t board_nucleo_f767zi_init(void);
void board_nucleo_f767zi_set_debug_led(bool enabled);
void board_nucleo_f767zi_toggle_debug_led(void);
bsp_uart_status_t board_nucleo_f767zi_init_debug_uart(void);
bsp_uart_status_t board_nucleo_f767zi_enable_debug_uart_rx_interrupt(void);
bsp_uart_status_t board_nucleo_f767zi_read_debug_byte(uint8_t *byte);
bsp_uart_status_t board_nucleo_f767zi_write_debug_byte(uint8_t byte);
void board_nucleo_f767zi_write_debug_string(const char *message);
bool board_nucleo_f767zi_debug_uart_overflowed(void);
void board_nucleo_f767zi_clear_debug_uart_overflow(void);

#endif
