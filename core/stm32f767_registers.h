/**
 ******************************************************************************
 * @file    stm32f767_registers.h
 * @brief   Minimal STM32F767 register definitions used by the current firmware
 ******************************************************************************
 */

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
    volatile uint32_t RESERVED3[11];
    volatile uint32_t CSR;
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

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t TIMINGR;
    volatile uint32_t TIMEOUTR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t PECR;
    volatile uint32_t RXDR;
    volatile uint32_t TXDR;
} stm32_i2c_registers_t;

#define RCC_BASE_ADDRESS 0x40023800UL
#define FLASH_BASE_ADDRESS 0x40023C00UL
#define GPIOA_BASE_ADDRESS 0x40020000UL
#define GPIO_PORT_STRIDE 0x00000400UL
#define I2C1_BASE_ADDRESS 0x40005400UL
#define USART2_BASE_ADDRESS 0x40004400UL
#define USART3_BASE_ADDRESS 0x40004800UL
#define USART6_BASE_ADDRESS 0x40011400UL
#define NVIC_ISER_BASE_ADDRESS 0xE000E100UL
#define USART6_IRQ_NUMBER 71U
#define SYSTICK_CTRL (*(volatile uint32_t *)0xE000E010UL)
#define SYSTICK_LOAD (*(volatile uint32_t *)0xE000E014UL)
#define SYSTICK_VAL (*(volatile uint32_t *)0xE000E018UL)
#define SCB_AIRCR (*(volatile uint32_t *)0xE000ED0CUL)
#define SCB_CFSR (*(volatile uint32_t *)0xE000ED28UL)
#define SCB_HFSR (*(volatile uint32_t *)0xE000ED2CUL)
#define SCB_MMFAR (*(volatile uint32_t *)0xE000ED34UL)
#define SCB_BFAR (*(volatile uint32_t *)0xE000ED38UL)
#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

#define RCC ((stm32_rcc_registers_t *)RCC_BASE_ADDRESS)
#define FLASH ((stm32_flash_registers_t *)FLASH_BASE_ADDRESS)
#define I2C1 ((stm32_i2c_registers_t *)I2C1_BASE_ADDRESS)
#define USART2 ((stm32_usart_registers_t *)USART2_BASE_ADDRESS)
#define USART3 ((stm32_usart_registers_t *)USART3_BASE_ADDRESS)
#define USART6 ((stm32_usart_registers_t *)USART6_BASE_ADDRESS)

#define SCB_CPACR_CP10_CP11_FULL_ACCESS (0xFUL << 20)
#define SYSTICK_CTRL_ENABLE (1UL << 0)
#define SYSTICK_CTRL_TICKINT (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE (1UL << 2)
#define SYSTICK_LOAD_RELOAD_MASK 0x00FFFFFFUL
#define SCB_AIRCR_PRIGROUP_MASK (0x7UL << 8)
#define SCB_AIRCR_VECTKEY_WRITE (0x5FAUL << 16)
#define SCB_AIRCR_SYSRESETREQ (1UL << 2)

#define RCC_CR_HSION (1UL << 0)
#define RCC_CR_HSIRDY (1UL << 1)
#define RCC_CR_HSEON (1UL << 16)
#define RCC_CR_HSERDY (1UL << 17)
#define RCC_CR_HSEBYP (1UL << 18)
#define RCC_CR_CSSON (1UL << 19)
#define RCC_CR_PLLON (1UL << 24)

#define RCC_CFGR_SW_MASK (0x3UL << 0)
#define RCC_AHB1ENR_GPIO_PORT_ENABLE(port_index) (1UL << (port_index))
#define RCC_APB1ENR_I2C1EN (1UL << 21)
#define RCC_APB1ENR_USART2EN (1UL << 17)
#define RCC_APB1ENR_USART3EN (1UL << 18)
#define RCC_APB2ENR_USART6EN (1UL << 5)

#define RCC_CSR_RMVF (1UL << 24)
#define RCC_CSR_BORRSTF (1UL << 25)
#define RCC_CSR_PINRSTF (1UL << 26)
#define RCC_CSR_PORRSTF (1UL << 27)
#define RCC_CSR_SFTRSTF (1UL << 28)
#define RCC_CSR_IWDGRSTF (1UL << 29)
#define RCC_CSR_WWDGRSTF (1UL << 30)
#define RCC_CSR_LPWRRSTF (1UL << 31)

#define I2C_CR1_PE (1UL << 0)

#define I2C_CR2_SADD_MASK 0x3FFUL
#define I2C_CR2_RD_WRN (1UL << 10)
#define I2C_CR2_START (1UL << 13)
#define I2C_CR2_STOP (1UL << 14)
#define I2C_CR2_NBYTES_SHIFT 16U
#define I2C_CR2_NBYTES_MASK (0xFFUL << I2C_CR2_NBYTES_SHIFT)
#define I2C_CR2_AUTOEND (1UL << 25)

#define I2C_ISR_TXIS (1UL << 1)
#define I2C_ISR_RXNE (1UL << 2)
#define I2C_ISR_NACKF (1UL << 4)
#define I2C_ISR_STOPF (1UL << 5)
#define I2C_ISR_TC (1UL << 6)
#define I2C_ISR_BERR (1UL << 8)
#define I2C_ISR_ARLO (1UL << 9)
#define I2C_ISR_OVR (1UL << 10)
#define I2C_ISR_TIMEOUT (1UL << 12)
#define I2C_ISR_BUSY (1UL << 15)

#define I2C_ICR_ADDRCF (1UL << 3)
#define I2C_ICR_NACKCF (1UL << 4)
#define I2C_ICR_STOPCF (1UL << 5)
#define I2C_ICR_BERRCF (1UL << 8)
#define I2C_ICR_ARLOCF (1UL << 9)
#define I2C_ICR_OVRCF (1UL << 10)
#define I2C_ICR_TIMOUTCF (1UL << 12)

#define USART_ISR_RXNE (1UL << 5)
#define USART_ISR_TXE (1UL << 7)

#define USART_CR1_UE (1UL << 0)
#define USART_CR1_TE (1UL << 3)
#define USART_CR1_RE (1UL << 2)
#define USART_CR1_RXNEIE (1UL << 5)

/**
 * @brief  Resolve a GPIO port register block from its zero-based port index
 * @param  port_index: zero-based GPIO port index
 * @retval stm32_gpio_registers_t*  Pointer to the selected GPIO register block
 */
static inline stm32_gpio_registers_t *stm32_gpio_port(uint32_t port_index)
{
    return (stm32_gpio_registers_t *)(uintptr_t)(GPIOA_BASE_ADDRESS + (GPIO_PORT_STRIDE * port_index));
}

/**
 * @brief  Enable an interrupt line in the NVIC set-enable register array
 * @param  irq_number: IRQ number to enable
 * @retval None
 */
static inline void stm32_nvic_enable_irq(uint32_t irq_number)
{
    volatile uint32_t *iser = (volatile uint32_t *)(NVIC_ISER_BASE_ADDRESS + ((irq_number / 32U) * sizeof(uint32_t)));

    *iser = (1UL << (irq_number % 32U));
}

#endif
