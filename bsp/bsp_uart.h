/**
 ******************************************************************************
 * @file    bsp_uart.h
 * @brief   Minimal UART driver interface for early bring-up on STM32F767
 ******************************************************************************
 */

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
    BSP_UART_ERR_NO_DATA,
    BSP_UART_ERR_TIMEOUT,
} bsp_uart_status_t;

typedef enum
{
    BSP_UART_INSTANCE_USART2 = 0,
    BSP_UART_INSTANCE_USART3,
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

/**
 * @brief  Initialize a supported UART peripheral and its GPIO pins
 * @param  config: pointer to the UART configuration structure
 * @retval BSP_UART_OK: UART initialized successfully
 * @retval BSP_UART_ERR_INVALID_ARGUMENT: configuration pointer or values invalid
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t bsp_uart_init(const bsp_uart_config_t *config);

/**
 * @brief  Enable receive interrupts for a previously initialized UART
 * @param  instance: UART instance to configure
 * @retval BSP_UART_OK: receive interrupt enabled successfully
 * @retval BSP_UART_ERR_INVALID_ARGUMENT: receive path was not enabled at initialization
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t bsp_uart_enable_rx_interrupt(bsp_uart_instance_t instance);

/**
 * @brief  Read one byte from the UART receive ring buffer without blocking
 * @param  instance: UART instance to read from
 * @param  byte: destination for the received byte
 * @retval BSP_UART_OK: one byte was returned
 * @retval BSP_UART_ERR_INVALID_ARGUMENT: byte pointer invalid
 * @retval BSP_UART_ERR_NO_DATA: no received byte available
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t bsp_uart_read_byte(bsp_uart_instance_t instance, uint8_t *byte);

/**
 * @brief  Transmit one byte through a UART instance
 * @param  instance: UART instance to use
 * @param  byte: data byte to send
 * @retval BSP_UART_OK: byte transmitted successfully
 * @retval BSP_UART_ERR_TIMEOUT: transmitter did not become ready in time
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t bsp_uart_write_byte(bsp_uart_instance_t instance, uint8_t byte);

/**
 * @brief  Transmit a null-terminated string through a UART instance
 * @param  instance: UART instance to use
 * @param  message: string to send
 * @retval BSP_UART_OK: string transmitted successfully
 * @retval BSP_UART_ERR_INVALID_ARGUMENT: message pointer invalid
 * @retval BSP_UART_ERR_TIMEOUT: transmitter did not become ready in time
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t bsp_uart_write_string(bsp_uart_instance_t instance, const char *message);

/**
 * @brief  Query whether a UART receive buffer overflow has been recorded
 * @param  instance: UART instance to query
 * @retval true: at least one received byte was dropped
 * @retval false: no overflow recorded or instance unsupported
 */
bool bsp_uart_rx_overflowed(bsp_uart_instance_t instance);

/**
 * @brief  Query whether a UART receive buffer currently has unread data
 * @param  instance: UART instance to query
 * @retval true: at least one unread byte is available
 * @retval false: no unread byte available or instance unsupported
 */
bool bsp_uart_has_rx_data(bsp_uart_instance_t instance);

/**
 * @brief  Clear the receive overflow flag for a UART instance
 * @param  instance: UART instance to update
 * @retval None
 */
void bsp_uart_clear_rx_overflow(bsp_uart_instance_t instance);

#endif
