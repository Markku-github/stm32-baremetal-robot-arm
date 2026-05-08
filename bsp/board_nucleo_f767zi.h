#ifndef BOARD_NUCLEO_F767ZI_H
#define BOARD_NUCLEO_F767ZI_H

#include "bsp_gpio.h"

bsp_gpio_status_t board_nucleo_f767zi_init(void);
void board_nucleo_f767zi_set_debug_led(bool enabled);
void board_nucleo_f767zi_toggle_debug_led(void);

#endif