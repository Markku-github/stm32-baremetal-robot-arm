#ifndef STM32F767_REGISTERS_H
#define STM32F767_REGISTERS_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
} stm32_rcc_registers_t;

typedef struct
{
    volatile uint32_t ACR;
} stm32_flash_registers_t;

#define RCC_BASE_ADDRESS 0x40023800UL
#define FLASH_BASE_ADDRESS 0x40023C00UL
#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

#define RCC ((stm32_rcc_registers_t *)RCC_BASE_ADDRESS)
#define FLASH ((stm32_flash_registers_t *)FLASH_BASE_ADDRESS)

#define SCB_CPACR_CP10_CP11_FULL_ACCESS (0xFUL << 20)

#define RCC_CR_HSION (1UL << 0)
#define RCC_CR_HSIRDY (1UL << 1)
#define RCC_CR_HSEON (1UL << 16)
#define RCC_CR_HSERDY (1UL << 17)
#define RCC_CR_HSEBYP (1UL << 18)
#define RCC_CR_CSSON (1UL << 19)
#define RCC_CR_PLLON (1UL << 24)

#define RCC_CFGR_SW_MASK (0x3UL << 0)

#endif