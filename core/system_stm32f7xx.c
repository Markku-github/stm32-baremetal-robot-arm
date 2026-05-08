/**
 ******************************************************************************
 * @file    system_stm32f7xx.c
 * @brief   Minimal system clock initialization for the STM32F767 bring-up path
 ******************************************************************************
 */

#include "system_stm32f7xx.h"
#include "stm32f767_registers.h"

#define HSI_FREQUENCY_HZ 16000000U

uint32_t SystemCoreClock = HSI_FREQUENCY_HZ;

/**
 * @brief  Reset the clock tree to a known HSI-based safe state
 * @retval None
 */
static void system_clock_reset_to_hsi(void)
{
    SCB_CPACR |= SCB_CPACR_CP10_CP11_FULL_ACCESS;

    RCC->CR |= RCC_CR_HSION;

    while ((RCC->CR & RCC_CR_HSIRDY) == 0U)
    {
    }

    RCC->CFGR &= ~RCC_CFGR_SW_MASK;
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_HSEBYP | RCC_CR_CSSON | RCC_CR_PLLON);
    FLASH->ACR = 0U;
}

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = HSI_FREQUENCY_HZ;
}

void SystemInit(void)
{
    system_clock_reset_to_hsi();
    SystemCoreClockUpdate();
}