#include "board_nucleo_f767zi.h"

#define BOARD_NUCLEO_F767ZI_DEBUG_LED_PORT BSP_GPIO_PORT_B
#define BOARD_NUCLEO_F767ZI_DEBUG_LED_PIN 0U

bsp_gpio_status_t board_nucleo_f767zi_init(void)
{
    const bsp_gpio_output_config_t debug_led_config = {
        .port = BOARD_NUCLEO_F767ZI_DEBUG_LED_PORT,
        .pin = BOARD_NUCLEO_F767ZI_DEBUG_LED_PIN,
        .output_type = BSP_GPIO_OUTPUT_PUSH_PULL,
        .pull = BSP_GPIO_PULL_NONE,
        .speed = BSP_GPIO_SPEED_LOW,
    };
    bsp_gpio_status_t status;

    status = bsp_gpio_init_output(&debug_led_config);
    if (status != BSP_GPIO_OK)
    {
        return status;
    }

    return bsp_gpio_write_pin(BOARD_NUCLEO_F767ZI_DEBUG_LED_PORT, BOARD_NUCLEO_F767ZI_DEBUG_LED_PIN, false);
}

void board_nucleo_f767zi_set_debug_led(bool enabled)
{
    (void)bsp_gpio_write_pin(BOARD_NUCLEO_F767ZI_DEBUG_LED_PORT, BOARD_NUCLEO_F767ZI_DEBUG_LED_PIN, enabled);
}

void board_nucleo_f767zi_toggle_debug_led(void)
{
    (void)bsp_gpio_toggle_pin(BOARD_NUCLEO_F767ZI_DEBUG_LED_PORT, BOARD_NUCLEO_F767ZI_DEBUG_LED_PIN);
}