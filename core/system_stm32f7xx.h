/**
 ******************************************************************************
 * @file    system_stm32f7xx.h
 * @brief   Minimal system clock initialization interface for STM32F767
 ******************************************************************************
 */

#ifndef SYSTEM_STM32F7XX_H
#define SYSTEM_STM32F7XX_H

#include <stdint.h>

extern uint32_t SystemCoreClock;

/**
 * @brief  Initialize the MCU system clock and core runtime prerequisites
 * @retval None
 */
void SystemInit(void);

/**
 * @brief  Refresh the cached SystemCoreClock value from the active clock setup
 * @retval None
 */
void SystemCoreClockUpdate(void);

#endif
