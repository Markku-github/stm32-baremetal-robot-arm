#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_gpio.h"

typedef enum
{
    BSP_UART_OK = 0,
    BSP_UART_ERR_INVALID_ARGUMENT,
    BSP_UART_ERR_UNSUPPORTED_INSTANCE,
} bsp_uart_status_t;

typedef enum
{
    BSP_UART_INSTANCE_USART2 = 0,
    BSP_UART_INSTANCE_USART6,
} bsp_uart_instance_t;

typedef struct
{
    bsp_uart_instance_t instance;
    uint32_t baud_rate;
    uint32_t peripheral_clock_hz;
    bsp_gpio_alternate_function_config_t tx_pin;
    bsp_gpio_alternate_function_config_t rx_pin;
    bool enable_rx;
} bsp_uart_config_t;

bsp_uart_status_t bsp_uart_init(const bsp_uart_config_t *config);
bsp_uart_status_t bsp_uart_write_byte(bsp_uart_instance_t instance, uint8_t byte);
bsp_uart_status_t bsp_uart_write_string(bsp_uart_instance_t instance, const char *message);

#endif