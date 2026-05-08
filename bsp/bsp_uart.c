#include "bsp_uart.h"

#include "stm32f767_registers.h"

static bool bsp_uart_is_supported_instance(bsp_uart_instance_t instance)
{
    return (instance == BSP_UART_INSTANCE_USART2) || (instance == BSP_UART_INSTANCE_USART6);
}

static stm32_usart_registers_t *bsp_uart_registers(bsp_uart_instance_t instance)
{
    if (instance == BSP_UART_INSTANCE_USART2)
    {
        return USART2;
    }

    if (instance == BSP_UART_INSTANCE_USART6)
    {
        return USART6;
    }

    return 0;
}

static void bsp_uart_enable_clock(bsp_uart_instance_t instance)
{
    if (instance == BSP_UART_INSTANCE_USART2)
    {
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    }
    else if (instance == BSP_UART_INSTANCE_USART6)
    {
        RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
    }
}

static uint32_t bsp_uart_compute_brr(uint32_t peripheral_clock_hz, uint32_t baud_rate)
{
    return (peripheral_clock_hz + (baud_rate / 2U)) / baud_rate;
}

bsp_uart_status_t bsp_uart_init(const bsp_uart_config_t *config)
{
    stm32_usart_registers_t *usart;
    bsp_gpio_status_t gpio_status;

    if ((config == 0) || !bsp_uart_is_supported_instance(config->instance) || (config->baud_rate == 0U) || (config->peripheral_clock_hz == 0U))
    {
        return BSP_UART_ERR_INVALID_ARGUMENT;
    }

    gpio_status = bsp_gpio_init_alternate_function(&config->tx_pin);
    if (gpio_status != BSP_GPIO_OK)
    {
        return BSP_UART_ERR_INVALID_ARGUMENT;
    }

    if (config->enable_rx)
    {
        gpio_status = bsp_gpio_init_alternate_function(&config->rx_pin);
        if (gpio_status != BSP_GPIO_OK)
        {
            return BSP_UART_ERR_INVALID_ARGUMENT;
        }
    }

    bsp_uart_enable_clock(config->instance);
    usart = bsp_uart_registers(config->instance);
    if (usart == 0)
    {
        return BSP_UART_ERR_UNSUPPORTED_INSTANCE;
    }

    usart->CR1 = 0U;
    usart->CR2 = 0U;
    usart->CR3 = 0U;
    usart->BRR = bsp_uart_compute_brr(config->peripheral_clock_hz, config->baud_rate);
    usart->CR1 = USART_CR1_TE | USART_CR1_UE;

    if (config->enable_rx)
    {
        usart->CR1 |= USART_CR1_RE;
    }

    return BSP_UART_OK;
}

bsp_uart_status_t bsp_uart_write_byte(bsp_uart_instance_t instance, uint8_t byte)
{
    stm32_usart_registers_t *usart = bsp_uart_registers(instance);

    if (usart == 0)
    {
        return BSP_UART_ERR_UNSUPPORTED_INSTANCE;
    }

    while ((usart->SR & USART_SR_TXE) == 0U)
    {
    }

    usart->DR = byte;

    while ((usart->SR & USART_SR_TC) == 0U)
    {
    }

    return BSP_UART_OK;
}

bsp_uart_status_t bsp_uart_write_string(bsp_uart_instance_t instance, const char *message)
{
    if (message == 0)
    {
        return BSP_UART_ERR_INVALID_ARGUMENT;
    }

    while (*message != '\0')
    {
        bsp_uart_status_t status = bsp_uart_write_byte(instance, (uint8_t)(*message));
        if (status != BSP_UART_OK)
        {
            return status;
        }

        message++;
    }

    return BSP_UART_OK;
}