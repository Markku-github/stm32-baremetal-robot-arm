#include "system_stm32f7xx.h"

uint32_t SystemCoreClock = 16000000U;

void SystemInit(void)
{
    SystemCoreClock = 16000000U;
}