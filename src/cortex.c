#include "platform.h"

extern void *schrx_idle_stack;
extern void (*schrx_systick_handler_origin)(void);
extern void *schrx_schr_instance;
extern void SchrX_UserSysTickHandler(void);

uint64_t system_tick = 0;

void* schrx_get_running_scheduler(void)
{ return schrx_schr_instance; }

void schrx_switch(void)
{
    SysTick -> VAL = 0;
    SCB -> ICSR = SCB_ICSR_PENDSVSET_Msk;
}

uint64_t schrx_get_system_tick(void)
{ return system_tick; }

void schrx_reset_system_tick(void)
{ system_tick = 0; }

void schrx_systick_handler(void)
{
    system_tick++;
    SchrX_UserSysTickHandler();
    schrx_switch();
}


void schrx_context_switch_irq(SchrX_Context *_save , SchrX_Context *_new, SchrX_IRQContext *_irq_context)
{
    #define IRQ_STORE_REGS_SIZE     32
    #define APSR_MASK               ((uint32_t)0xF8000000)

    if( _save ) //Not a IDLE Task
    {
        /* store context */
        _save -> R[0] = _irq_context -> stored -> R0 ;
        _save -> R[1] = _irq_context -> stored -> R1;
        _save -> R[2] = _irq_context -> stored -> R2;
        _save -> R[3] = _irq_context -> stored -> R3;
        _save -> R[4] = _irq_context -> stored_extra -> R4;
        _save -> R[5] = _irq_context -> stored_extra -> R5;
        _save -> R[6] = _irq_context -> stored_extra -> R6;
        _save -> R[7] = _irq_context -> stored_extra -> R7;
        _save -> R[8] = _irq_context -> stored_extra -> R8;
        _save -> R[9] = _irq_context -> stored_extra -> R9;
        _save -> R[10] = _irq_context -> stored_extra -> R10;
        _save -> R[11] = _irq_context -> stored_extra -> R11;
        _save -> R[12] = _irq_context -> stored -> R12;
        _save -> LR = _irq_context -> stored -> LR;
        _save -> PC = _irq_context -> stored -> PC;
        //_save -> xPSR = _irq_context -> stored -> xPSR & APSR_MASK;
        _save -> xPSR = _irq_context -> stored -> xPSR;
        _save -> SP = _irq_context -> stored_extra -> SP + IRQ_STORE_REGS_SIZE;
    }
    /* restore target thread context */
    if( _new )
    {
        _irq_context -> stored_extra -> SP = _new -> SP - IRQ_STORE_REGS_SIZE;
        _irq_context -> stored_extra -> R4 = _new -> R[4];
        _irq_context -> stored_extra -> R5 = _new -> R[5];
        _irq_context -> stored_extra -> R6 = _new -> R[6];
        _irq_context -> stored_extra -> R7 = _new -> R[7];
        _irq_context -> stored_extra -> R8 = _new -> R[8];
        _irq_context -> stored_extra -> R9 = _new -> R[9];
        _irq_context -> stored_extra -> R10 = _new -> R[10];
        _irq_context -> stored_extra -> R11 = _new -> R[11];
        
        _irq_context -> stored = (SchrX_IRQStoredRegs*) _irq_context -> stored_extra -> SP ;
        _irq_context -> stored -> R0 = _new -> R[0];
        _irq_context -> stored -> R1 = _new -> R[1];
        _irq_context -> stored -> R2 = _new -> R[2];
        _irq_context -> stored -> R3 = _new -> R[3];
        _irq_context -> stored -> R12 = _new -> R[12];
        _irq_context -> stored -> LR = _new -> LR;
        _irq_context -> stored -> PC = _new -> PC;
        _irq_context -> stored -> xPSR = _new -> xPSR;
        //_irq_context -> stored -> xPSR = (_new -> xPSR & APSR_MASK) | (_irq_context -> stored -> xPSR &(~APSR_MASK));
    }
    else /* IDLE */
    {
        _irq_context -> stored_extra -> SP = (uint32_t) schrx_idle_stack;
        _irq_context -> stored -> PC = (uint32_t)&schrx_do_idle;
    }
    #undef IRQ_STORE_REGS_SIZE

}

void schrx_scheduler_attach()
{
    union
    {
        uint32_t irq_vector;
        uint32_t time_slice;
    }v1;


    /* Hook systick handler */
    v1.irq_vector = SCB -> VTOR;
    if( v1.irq_vector & (1UL << 29) )
    {
        schrx_systick_handler_origin = (void (*)())((uint32_t*)v1.irq_vector)[SysTick_IRQn + 16];
        ((uint32_t*)v1.irq_vector)[SysTick_IRQn + 16] = (uint32_t)&schrx_systick_handler;
        ((uint32_t*)v1.irq_vector)[PendSV_IRQn + 16] = (uint32_t)&schrx_pendsv_handler;
    }
    /* 

                While vector table is in FLASH Memory, scheduler can't engage 
            by hooking SysTick_Handler() and PendSV_Handler().

                If official startup file is used, compile SchedulerXIRQHook.s 
            and use SchrX_UserSystickHandler() for systick handler instead.
                
                If you set vector table by yourself, make sure that 
            schrx_systick_handler() is systick handler.
    
        */

    //configure system tick
    v1.time_slice = SysTick -> CALIB;
    v1.time_slice &= SysTick_CALIB_TENMS_Msk;
    if(!v1.time_slice) // no calibation
        SysTick -> LOAD = 0x00005000;
    else
        SysTick -> LOAD = v1.time_slice; // 1ms
    
    //NVIC_SetPriority(SysTick_IRQn, 0);
    NVIC_SetPriority(PendSV_IRQn, 0xFFFFFFFF);
    
    SysTick -> VAL = SysTick_VAL_CURRENT_Msk;
    SysTick -> CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void schrx_context_init(SchrX_Context *_context, void* _entry, void* _stack)
{
    unsigned char i;

    for(i = 0 ; i < 12 ; i++)
        _context -> R[i] = 0;
    _context -> SP = (uint32_t)_stack;
    _context -> LR = 0;
    _context -> PC = (uint32_t)_entry;
    _context -> xPSR = 0x01000000;
}
