/**
 ******************************************************************************
 * @file    board_nucleo_f767zi.h
 * @brief   Nucleo-F767ZI board-level wrappers for LED and debug UART access
 ******************************************************************************
 */

#ifndef BOARD_NUCLEO_F767ZI_H
#define BOARD_NUCLEO_F767ZI_H

#include <stdbool.h>

#include "bsp_gpio.h"
#include "bsp_i2c.h"
#include "bsp_uart.h"

/**
 * @brief  Initialize the basic board resources used during V0 bring-up
 * @retval BSP_GPIO_OK: board resources initialized successfully
 * @retval BSP_GPIO_ERR_INVALID_ARGUMENT: an underlying GPIO configuration failed
 */
bsp_gpio_status_t board_nucleo_f767zi_init(void);

/**
 * @brief  Set the state of the onboard LD1 debug LED
 * @param  enabled: true to turn the LED on, false to turn it off
 * @retval None
 */
void board_nucleo_f767zi_set_debug_led(bool enabled);

/**
 * @brief  Toggle the current state of the onboard LD1 debug LED
 * @retval None
 */
void board_nucleo_f767zi_toggle_debug_led(void);

/**
 * @brief  Initialize the board debug UART on USART6
 * @retval BSP_UART_OK: UART initialized successfully
 * @retval BSP_UART_ERR_INVALID_ARGUMENT: GPIO or UART configuration invalid
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t board_nucleo_f767zi_init_debug_uart(void);

/**
 * @brief  Initialize the board I2C bus used for PCA9685 communication tests
 * @retval BSP_I2C_OK: I2C bus initialized successfully
 * @retval BSP_I2C_ERR_INVALID_ARGUMENT: GPIO or timing configuration invalid
 * @retval BSP_I2C_ERR_UNSUPPORTED_INSTANCE: requested I2C instance unsupported
 */
bsp_i2c_status_t board_nucleo_f767zi_init_pca9685_i2c(void);

/**
 * @brief  Enable the USART6 receive interrupt for the board debug UART
 * @retval BSP_UART_OK: receive interrupt enabled successfully
 * @retval BSP_UART_ERR_INVALID_ARGUMENT: receive path was not enabled at initialization
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t board_nucleo_f767zi_enable_debug_uart_rx_interrupt(void);

/**
 * @brief  Read one received byte from the board debug UART ring buffer
 * @param  byte: destination for the received byte
 * @retval BSP_UART_OK: one byte was returned
 * @retval BSP_UART_ERR_INVALID_ARGUMENT: byte pointer invalid
 * @retval BSP_UART_ERR_NO_DATA: no received data currently available
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t board_nucleo_f767zi_read_debug_byte(uint8_t *byte);

/**
 * @brief  Transmit one byte through the board debug UART
 * @param  byte: data byte to send
 * @retval BSP_UART_OK: byte transmitted successfully
 * @retval BSP_UART_ERR_TIMEOUT: transmitter did not become ready in time
 * @retval BSP_UART_ERR_UNSUPPORTED_INSTANCE: requested UART instance unsupported
 */
bsp_uart_status_t board_nucleo_f767zi_write_debug_byte(uint8_t byte);

/**
 * @brief  Transmit a null-terminated string through the board debug UART
 * @param  message: string to send
 * @retval None
 */
void board_nucleo_f767zi_write_debug_string(const char *message);

/**
 * @brief  Query whether the board debug UART receive buffer overflowed
 * @retval true: at least one received byte was dropped
 * @retval false: no overflow has been recorded since the last clear
 */
bool board_nucleo_f767zi_debug_uart_overflowed(void);

/**
 * @brief  Clear the board debug UART receive overflow flag
 * @retval None
 */
void board_nucleo_f767zi_clear_debug_uart_overflow(void);

#endif
