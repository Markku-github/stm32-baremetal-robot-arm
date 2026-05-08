#include <stdint.h>

#include "system_stm32f7xx.h"

int main(void);

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void);
void Default_Handler(void);

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

__attribute__((used, section(".isr_vector")))
const uintptr_t vector_table[] = {
    (uintptr_t)&_estack,
    (uintptr_t)Reset_Handler,
    (uintptr_t)NMI_Handler,
    (uintptr_t)HardFault_Handler,
    (uintptr_t)MemManage_Handler,
    (uintptr_t)BusFault_Handler,
    (uintptr_t)UsageFault_Handler,
    0,
    0,
    0,
    0,
    (uintptr_t)SVC_Handler,
    (uintptr_t)DebugMon_Handler,
    0,
    (uintptr_t)PendSV_Handler,
    (uintptr_t)SysTick_Handler,
};

static void initialize_data(void)
{
    uint32_t *source = &_sidata;
    uint32_t *destination = &_sdata;

    while (destination < &_edata)
    {
        *destination = *source;
        destination++;
        source++;
    }
}

static void initialize_bss(void)
{
    uint32_t *destination = &_sbss;

    while (destination < &_ebss)
    {
        *destination = 0U;
        destination++;
    }
}

void Reset_Handler(void)
{
    initialize_data();
    initialize_bss();
    SystemInit();

    (void)main();

    for (;;)
    {
    }
}

void Default_Handler(void)
{
    for (;;)
    {
    }
}