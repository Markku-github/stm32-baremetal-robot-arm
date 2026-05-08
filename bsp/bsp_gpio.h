#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_GPIO_OK = 0,
    BSP_GPIO_ERR_INVALID_ARGUMENT,
} bsp_gpio_status_t;

typedef enum
{
    BSP_GPIO_PORT_A = 0,
    BSP_GPIO_PORT_B,
    BSP_GPIO_PORT_C,
    BSP_GPIO_PORT_D,
    BSP_GPIO_PORT_E,
    BSP_GPIO_PORT_F,
    BSP_GPIO_PORT_G,
    BSP_GPIO_PORT_H,
    BSP_GPIO_PORT_I,
    BSP_GPIO_PORT_J,
    BSP_GPIO_PORT_K,
} bsp_gpio_port_t;

typedef enum
{
    BSP_GPIO_OUTPUT_PUSH_PULL = 0,
    BSP_GPIO_OUTPUT_OPEN_DRAIN = 1,
} bsp_gpio_output_type_t;

typedef enum
{
    BSP_GPIO_PULL_NONE = 0,
    BSP_GPIO_PULL_UP = 1,
    BSP_GPIO_PULL_DOWN = 2,
} bsp_gpio_pull_t;

typedef enum
{
    BSP_GPIO_SPEED_LOW = 0,
    BSP_GPIO_SPEED_MEDIUM = 1,
    BSP_GPIO_SPEED_HIGH = 2,
    BSP_GPIO_SPEED_VERY_HIGH = 3,
} bsp_gpio_speed_t;

typedef struct
{
    bsp_gpio_port_t port;
    uint8_t pin;
    bsp_gpio_output_type_t output_type;
    bsp_gpio_pull_t pull;
    bsp_gpio_speed_t speed;
} bsp_gpio_output_config_t;

bsp_gpio_status_t bsp_gpio_init_output(const bsp_gpio_output_config_t *config);
bsp_gpio_status_t bsp_gpio_write_pin(bsp_gpio_port_t port, uint8_t pin, bool level);
bsp_gpio_status_t bsp_gpio_toggle_pin(bsp_gpio_port_t port, uint8_t pin);

#endif