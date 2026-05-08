#include "bsp_uart.h"

#include "stm32f767_registers.h"

#define BSP_UART_POLL_TIMEOUT_CYCLES 1000000U
#define BSP_UART_RX_BUFFER_CAPACITY 128U

typedef struct
{
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile bool overflowed;
    bool rx_enabled;
    uint8_t data[BSP_UART_RX_BUFFER_CAPACITY];
} bsp_uart_rx_buffer_t;

static bsp_uart_rx_buffer_t bsp_uart_rx_buffers[BSP_UART_INSTANCE_USART6 + 1U];

static void bsp_uart_startup_delay(void)
{
    /* Give the UART a short post-enable settle time observed on hardware bring-up. */
    volatile uint32_t cycles = 10000U;

    while (cycles > 0U)
    {
        cycles--;
    }
}

static bool bsp_uart_is_supported_instance(bsp_uart_instance_t instance)
{
    return (instance == BSP_UART_INSTANCE_USART2) || (instance == BSP_UART_INSTANCE_USART6);
}

static bsp_uart_rx_buffer_t *bsp_uart_rx_buffer(bsp_uart_instance_t instance)
{
    if (!bsp_uart_is_supported_instance(instance))
    {
        return 0;
    }

    return &bsp_uart_rx_buffers[(uint32_t)instance];
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

static void bsp_uart_reset_rx_buffer(bsp_uart_instance_t instance, bool rx_enabled)
{
    bsp_uart_rx_buffer_t *rx_buffer = bsp_uart_rx_buffer(instance);

    if (rx_buffer == 0)
    {
        return;
    }

    rx_buffer->head = 0U;
    rx_buffer->tail = 0U;
    rx_buffer->overflowed = false;
    rx_buffer->rx_enabled = rx_enabled;
}

static void bsp_uart_store_received_byte(bsp_uart_instance_t instance, uint8_t byte)
{
    bsp_uart_rx_buffer_t *rx_buffer = bsp_uart_rx_buffer(instance);
    uint16_t next_head;

    if ((rx_buffer == 0) || !rx_buffer->rx_enabled)
    {
        return;
    }

    next_head = (uint16_t)((rx_buffer->head + 1U) % BSP_UART_RX_BUFFER_CAPACITY);
    if (next_head == rx_buffer->tail)
    {
        rx_buffer->overflowed = true;
        return;
    }

    rx_buffer->data[rx_buffer->head] = byte;
    rx_buffer->head = next_head;
}

static void bsp_uart_handle_rx_interrupt(bsp_uart_instance_t instance)
{
    stm32_usart_registers_t *usart = bsp_uart_registers(instance);

    if (usart == 0)
    {
        return;
    }

    while ((usart->ISR & USART_ISR_RXNE) != 0U)
    {
        bsp_uart_store_received_byte(instance, (uint8_t)usart->RDR);
    }
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

    bsp_uart_reset_rx_buffer(config->instance, config->enable_rx);

    usart->CR1 = 0U;
    usart->CR2 = 0U;
    usart->CR3 = 0U;
    usart->BRR = bsp_uart_compute_brr(config->peripheral_clock_hz, config->baud_rate);
    usart->CR1 = USART_CR1_TE | USART_CR1_UE;

    if (config->enable_rx)
    {
        usart->CR1 |= USART_CR1_RE;
    }

    bsp_uart_startup_delay();

    return BSP_UART_OK;
}

bsp_uart_status_t bsp_uart_enable_rx_interrupt(bsp_uart_instance_t instance)
{
    stm32_usart_registers_t *usart = bsp_uart_registers(instance);
    bsp_uart_rx_buffer_t *rx_buffer = bsp_uart_rx_buffer(instance);

    if ((usart == 0) || (rx_buffer == 0))
    {
        return BSP_UART_ERR_UNSUPPORTED_INSTANCE;
    }

    if (!rx_buffer->rx_enabled)
    {
        return BSP_UART_ERR_INVALID_ARGUMENT;
    }

    if (instance != BSP_UART_INSTANCE_USART6)
    {
        return BSP_UART_ERR_UNSUPPORTED_INSTANCE;
    }

    stm32_nvic_enable_irq(USART6_IRQ_NUMBER);
    usart->CR1 |= USART_CR1_RXNEIE;

    return BSP_UART_OK;
}

bsp_uart_status_t bsp_uart_read_byte(bsp_uart_instance_t instance, uint8_t *byte)
{
    bsp_uart_rx_buffer_t *rx_buffer = bsp_uart_rx_buffer(instance);

    if (byte == 0)
    {
        return BSP_UART_ERR_INVALID_ARGUMENT;
    }

    if (rx_buffer == 0)
    {
        return BSP_UART_ERR_UNSUPPORTED_INSTANCE;
    }

    if (rx_buffer->tail == rx_buffer->head)
    {
        return BSP_UART_ERR_NO_DATA;
    }

    *byte = rx_buffer->data[rx_buffer->tail];
    rx_buffer->tail = (uint16_t)((rx_buffer->tail + 1U) % BSP_UART_RX_BUFFER_CAPACITY);

    return BSP_UART_OK;
}

bsp_uart_status_t bsp_uart_write_byte(bsp_uart_instance_t instance, uint8_t byte)
{
    stm32_usart_registers_t *usart = bsp_uart_registers(instance);
    uint32_t timeout = BSP_UART_POLL_TIMEOUT_CYCLES;

    if (usart == 0)
    {
        return BSP_UART_ERR_UNSUPPORTED_INSTANCE;
    }

    while ((usart->ISR & USART_ISR_TXE) == 0U)
    {
        if (timeout == 0U)
        {
            return BSP_UART_ERR_TIMEOUT;
        }

        timeout--;
    }

    usart->TDR = byte;

    return BSP_UART_OK;
}

bool bsp_uart_rx_overflowed(bsp_uart_instance_t instance)
{
    bsp_uart_rx_buffer_t *rx_buffer = bsp_uart_rx_buffer(instance);

    if (rx_buffer == 0)
    {
        return false;
    }

    return rx_buffer->overflowed;
}

void bsp_uart_clear_rx_overflow(bsp_uart_instance_t instance)
{
    bsp_uart_rx_buffer_t *rx_buffer = bsp_uart_rx_buffer(instance);

    if (rx_buffer == 0)
    {
        return;
    }

    rx_buffer->overflowed = false;
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

void USART6_IRQHandler(void)
{
    bsp_uart_handle_rx_interrupt(BSP_UART_INSTANCE_USART6);
}
