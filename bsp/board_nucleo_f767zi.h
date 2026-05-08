#ifndef BOARD_NUCLEO_F767ZI_H
#define BOARD_NUCLEO_F767ZI_H

#include <stdbool.h>

#include "bsp_gpio.h"
#include "bsp_uart.h"

bsp_gpio_status_t board_nucleo_f767zi_init(void);
void board_nucleo_f767zi_set_debug_led(bool enabled);
void board_nucleo_f767zi_toggle_debug_led(void);
bsp_uart_status_t board_nucleo_f767zi_init_debug_uart(void);
void board_nucleo_f767zi_write_debug_string(const char *message);

#endif