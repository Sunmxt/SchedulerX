#include "platform.h"

void __attribute((naked)) SysTick_Handler(void)
{
    asm volatile("b schrx_systick_handler");
}

void __attribute((naked)) PendSV_Handler(void)
{
    asm volatile("b schrx_pendsv_handler");
}

