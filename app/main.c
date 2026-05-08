#include <stdint.h>

#include "board_nucleo_f767zi.h"

static void boot_delay(volatile uint32_t cycles)
{
    while (cycles > 0U)
    {
        cycles--;
    }
}

int main(void)
{
    if (board_nucleo_f767zi_init() != BSP_GPIO_OK)
    {
        for (;;)
        {
        }
    }

    for (;;)
    {
        board_nucleo_f767zi_toggle_debug_led();
        boot_delay(2000000U);
    }
}