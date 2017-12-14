/*
    
    Default handlers for official startup (startup_xxxxx.S / system_xxxxx.c).

*/

#include "platform.h"

extern void SystemCoreClockUpdate(void);
extern uint32_t SystemCoreClock; // Clock setting

uint64_t schrx_cortex_calc_tick_count_1s(void)
{
    SystemCoreClockUpdate();
    return SystemCoreClock;
}

void __attribute((naked)) SysTick_Handler(void)
{
    asm volatile("b schrx_systick_handler");
}

void __attribute((naked)) PendSV_Handler(void)
{
    asm volatile("b schrx_pendsv_handler");
}

