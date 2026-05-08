#ifndef STM32F767_REGISTERS_H
#define STM32F767_REGISTERS_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    volatile uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    volatile uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} stm32_rcc_registers_t;

typedef struct
{
    volatile uint32_t ACR;
} stm32_flash_registers_t;

typedef struct
{
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} stm32_gpio_registers_t;

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t BRR;
    volatile uint32_t GTPR;
    volatile uint32_t RTOR;
    volatile uint32_t RQR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} stm32_usart_registers_t;

#define RCC_BASE_ADDRESS 0x40023800UL
#define FLASH_BASE_ADDRESS 0x40023C00UL
#define GPIOA_BASE_ADDRESS 0x40020000UL
#define GPIO_PORT_STRIDE 0x00000400UL
#define USART2_BASE_ADDRESS 0x40004400UL
#define USART6_BASE_ADDRESS 0x40011400UL
#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

#define RCC ((stm32_rcc_registers_t *)RCC_BASE_ADDRESS)
#define FLASH ((stm32_flash_registers_t *)FLASH_BASE_ADDRESS)
#define USART2 ((stm32_usart_registers_t *)USART2_BASE_ADDRESS)
#define USART6 ((stm32_usart_registers_t *)USART6_BASE_ADDRESS)

#define SCB_CPACR_CP10_CP11_FULL_ACCESS (0xFUL << 20)

#define RCC_CR_HSION (1UL << 0)
#define RCC_CR_HSIRDY (1UL << 1)
#define RCC_CR_HSEON (1UL << 16)
#define RCC_CR_HSERDY (1UL << 17)
#define RCC_CR_HSEBYP (1UL << 18)
#define RCC_CR_CSSON (1UL << 19)
#define RCC_CR_PLLON (1UL << 24)

#define RCC_CFGR_SW_MASK (0x3UL << 0)
#define RCC_AHB1ENR_GPIO_PORT_ENABLE(port_index) (1UL << (port_index))
#define RCC_APB1ENR_USART2EN (1UL << 17)
#define RCC_APB2ENR_USART6EN (1UL << 5)

#define USART_ISR_TXE (1UL << 7)

#define USART_CR1_UE (1UL << 0)
#define USART_CR1_TE (1UL << 3)
#define USART_CR1_RE (1UL << 2)

static inline stm32_gpio_registers_t *stm32_gpio_port(uint32_t port_index)
{
    return (stm32_gpio_registers_t *)(GPIOA_BASE_ADDRESS + (GPIO_PORT_STRIDE * port_index));
}

#endif