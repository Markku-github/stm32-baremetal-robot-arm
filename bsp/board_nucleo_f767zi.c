/**
 ******************************************************************************
 * @file    board_nucleo_f767zi.c
 * @brief   Nucleo-F767ZI board-level wrappers for LED, debug UART, and I2C access
 ******************************************************************************
 */

#include "board_nucleo_f767zi.h"

#include "system_stm32f7xx.h"

#define BOARD_NUCLEO_F767ZI_DEBUG_LED_PORT BSP_GPIO_PORT_B
#define BOARD_NUCLEO_F767ZI_DEBUG_LED_PIN 0U

#define BOARD_NUCLEO_F767ZI_DEBUG_UART_INSTANCE BSP_UART_INSTANCE_USART6
#define BOARD_NUCLEO_F767ZI_DEBUG_UART_TX_PORT BSP_GPIO_PORT_G
#define BOARD_NUCLEO_F767ZI_DEBUG_UART_TX_PIN 14U
#define BOARD_NUCLEO_F767ZI_DEBUG_UART_RX_PORT BSP_GPIO_PORT_G
#define BOARD_NUCLEO_F767ZI_DEBUG_UART_RX_PIN 9U
#define BOARD_NUCLEO_F767ZI_DEBUG_UART_AF 8U
#define BOARD_NUCLEO_F767ZI_DEBUG_UART_BAUD_RATE 115200U

#define BOARD_NUCLEO_F767ZI_PCA9685_I2C_INSTANCE BSP_I2C_INSTANCE_I2C1
#define BOARD_NUCLEO_F767ZI_PCA9685_I2C_SCL_PORT BSP_GPIO_PORT_B
#define BOARD_NUCLEO_F767ZI_PCA9685_I2C_SCL_PIN 8U
#define BOARD_NUCLEO_F767ZI_PCA9685_I2C_SDA_PORT BSP_GPIO_PORT_B
#define BOARD_NUCLEO_F767ZI_PCA9685_I2C_SDA_PIN 9U
#define BOARD_NUCLEO_F767ZI_PCA9685_I2C_AF 4U
#define BOARD_NUCLEO_F767ZI_PCA9685_I2C_TIMING_16MHZ_100KHZ 0x0020098EU

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

bsp_uart_status_t board_nucleo_f767zi_init_debug_uart(void)
{
    const bsp_uart_config_t config = {
        .instance = BOARD_NUCLEO_F767ZI_DEBUG_UART_INSTANCE,
        .baud_rate = BOARD_NUCLEO_F767ZI_DEBUG_UART_BAUD_RATE,
        .peripheral_clock_hz = SystemCoreClock,
        .tx_pin = {
            .port = BOARD_NUCLEO_F767ZI_DEBUG_UART_TX_PORT,
            .pin = BOARD_NUCLEO_F767ZI_DEBUG_UART_TX_PIN,
            .alternate_function = BOARD_NUCLEO_F767ZI_DEBUG_UART_AF,
            .output_type = BSP_GPIO_OUTPUT_PUSH_PULL,
            .pull = BSP_GPIO_PULL_UP,
            .speed = BSP_GPIO_SPEED_VERY_HIGH,
        },
        .rx_pin = {
            .port = BOARD_NUCLEO_F767ZI_DEBUG_UART_RX_PORT,
            .pin = BOARD_NUCLEO_F767ZI_DEBUG_UART_RX_PIN,
            .alternate_function = BOARD_NUCLEO_F767ZI_DEBUG_UART_AF,
            .output_type = BSP_GPIO_OUTPUT_PUSH_PULL,
            .pull = BSP_GPIO_PULL_UP,
            .speed = BSP_GPIO_SPEED_VERY_HIGH,
        },
        .enable_rx = true,
    };

    return bsp_uart_init(&config);
}

bsp_i2c_status_t board_nucleo_f767zi_init_pca9685_i2c(void)
{
    const bsp_i2c_config_t config = {
        .instance = BOARD_NUCLEO_F767ZI_PCA9685_I2C_INSTANCE,
        .timing = BOARD_NUCLEO_F767ZI_PCA9685_I2C_TIMING_16MHZ_100KHZ,
        .scl_pin = {
            .port = BOARD_NUCLEO_F767ZI_PCA9685_I2C_SCL_PORT,
            .pin = BOARD_NUCLEO_F767ZI_PCA9685_I2C_SCL_PIN,
            .alternate_function = BOARD_NUCLEO_F767ZI_PCA9685_I2C_AF,
            .output_type = BSP_GPIO_OUTPUT_OPEN_DRAIN,
            .pull = BSP_GPIO_PULL_UP,
            .speed = BSP_GPIO_SPEED_HIGH,
        },
        .sda_pin = {
            .port = BOARD_NUCLEO_F767ZI_PCA9685_I2C_SDA_PORT,
            .pin = BOARD_NUCLEO_F767ZI_PCA9685_I2C_SDA_PIN,
            .alternate_function = BOARD_NUCLEO_F767ZI_PCA9685_I2C_AF,
            .output_type = BSP_GPIO_OUTPUT_OPEN_DRAIN,
            .pull = BSP_GPIO_PULL_UP,
            .speed = BSP_GPIO_SPEED_HIGH,
        },
    };

    return bsp_i2c_init(&config);
}

bsp_uart_status_t board_nucleo_f767zi_enable_debug_uart_rx_interrupt(void)
{
    return bsp_uart_enable_rx_interrupt(BOARD_NUCLEO_F767ZI_DEBUG_UART_INSTANCE);
}

bsp_uart_status_t board_nucleo_f767zi_read_debug_byte(uint8_t *byte)
{
    return bsp_uart_read_byte(BOARD_NUCLEO_F767ZI_DEBUG_UART_INSTANCE, byte);
}

bsp_uart_status_t board_nucleo_f767zi_write_debug_byte(uint8_t byte)
{
    return bsp_uart_write_byte(BOARD_NUCLEO_F767ZI_DEBUG_UART_INSTANCE, byte);
}

void board_nucleo_f767zi_write_debug_string(const char *message)
{
    (void)bsp_uart_write_string(BOARD_NUCLEO_F767ZI_DEBUG_UART_INSTANCE, message);
}

bool board_nucleo_f767zi_debug_uart_overflowed(void)
{
    return bsp_uart_rx_overflowed(BOARD_NUCLEO_F767ZI_DEBUG_UART_INSTANCE);
}

void board_nucleo_f767zi_clear_debug_uart_overflow(void)
{
    bsp_uart_clear_rx_overflow(BOARD_NUCLEO_F767ZI_DEBUG_UART_INSTANCE);
}
