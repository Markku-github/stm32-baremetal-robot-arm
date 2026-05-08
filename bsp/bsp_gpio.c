#include "bsp_gpio.h"

#include "stm32f767_registers.h"

static bool bsp_gpio_is_valid_port(bsp_gpio_port_t port)
{
    return port <= BSP_GPIO_PORT_K;
}

static bool bsp_gpio_is_valid_pin(uint8_t pin)
{
    return pin < 16U;
}

static uint32_t bsp_gpio_two_bit_shift(uint8_t pin)
{
    return (uint32_t)pin * 2U;
}

static uint32_t bsp_gpio_alternate_function_shift(uint8_t pin)
{
    return ((uint32_t)pin & 0x7UL) * 4U;
}

bsp_gpio_status_t bsp_gpio_init_output(const bsp_gpio_output_config_t *config)
{
    uint32_t shift;
    stm32_gpio_registers_t *gpio;

    if ((config == 0) || !bsp_gpio_is_valid_port(config->port) || !bsp_gpio_is_valid_pin(config->pin))
    {
        return BSP_GPIO_ERR_INVALID_ARGUMENT;
    }

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIO_PORT_ENABLE((uint32_t)config->port);
    gpio = stm32_gpio_port((uint32_t)config->port);
    shift = bsp_gpio_two_bit_shift(config->pin);

    gpio->MODER &= ~(0x3UL << shift);
    gpio->MODER |= (0x1UL << shift);

    gpio->OTYPER &= ~(0x1UL << config->pin);
    gpio->OTYPER |= ((uint32_t)config->output_type << config->pin);

    gpio->OSPEEDR &= ~(0x3UL << shift);
    gpio->OSPEEDR |= ((uint32_t)config->speed << shift);

    gpio->PUPDR &= ~(0x3UL << shift);
    gpio->PUPDR |= ((uint32_t)config->pull << shift);

    return BSP_GPIO_OK;
}

bsp_gpio_status_t bsp_gpio_init_alternate_function(const bsp_gpio_alternate_function_config_t *config)
{
    uint32_t shift;
    uint32_t af_index;
    uint32_t af_shift;
    stm32_gpio_registers_t *gpio;

    if ((config == 0) || !bsp_gpio_is_valid_port(config->port) || !bsp_gpio_is_valid_pin(config->pin) || (config->alternate_function > 15U))
    {
        return BSP_GPIO_ERR_INVALID_ARGUMENT;
    }

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIO_PORT_ENABLE((uint32_t)config->port);
    gpio = stm32_gpio_port((uint32_t)config->port);
    shift = bsp_gpio_two_bit_shift(config->pin);
    af_index = (uint32_t)config->pin / 8U;
    af_shift = bsp_gpio_alternate_function_shift(config->pin);

    gpio->MODER &= ~(0x3UL << shift);
    gpio->MODER |= (0x2UL << shift);

    gpio->OTYPER &= ~(0x1UL << config->pin);
    gpio->OTYPER |= ((uint32_t)config->output_type << config->pin);

    gpio->OSPEEDR &= ~(0x3UL << shift);
    gpio->OSPEEDR |= ((uint32_t)config->speed << shift);

    gpio->PUPDR &= ~(0x3UL << shift);
    gpio->PUPDR |= ((uint32_t)config->pull << shift);

    gpio->AFR[af_index] &= ~(0xFUL << af_shift);
    gpio->AFR[af_index] |= ((uint32_t)config->alternate_function << af_shift);

    return BSP_GPIO_OK;
}

bsp_gpio_status_t bsp_gpio_write_pin(bsp_gpio_port_t port, uint8_t pin, bool level)
{
    stm32_gpio_registers_t *gpio;

    if (!bsp_gpio_is_valid_port(port) || !bsp_gpio_is_valid_pin(pin))
    {
        return BSP_GPIO_ERR_INVALID_ARGUMENT;
    }

    gpio = stm32_gpio_port((uint32_t)port);
    gpio->BSRR = level ? (0x1UL << pin) : (0x1UL << (pin + 16U));

    return BSP_GPIO_OK;
}

bsp_gpio_status_t bsp_gpio_toggle_pin(bsp_gpio_port_t port, uint8_t pin)
{
    stm32_gpio_registers_t *gpio;
    bool next_level;

    if (!bsp_gpio_is_valid_port(port) || !bsp_gpio_is_valid_pin(pin))
    {
        return BSP_GPIO_ERR_INVALID_ARGUMENT;
    }

    gpio = stm32_gpio_port((uint32_t)port);
    next_level = (gpio->ODR & (0x1UL << pin)) == 0U;

    return bsp_gpio_write_pin(port, pin, next_level);
}