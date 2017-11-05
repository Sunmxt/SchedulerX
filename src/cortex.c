#include "platform.h"
#include "SchedulerX.h"

void schrx_scheduler_attach(void);

/* -------------------------------------------- */
/*  Setting                                     */
/* -------------------------------------------- */

#define SCHRX_IDLE_STACK_SIZE   400

/* -------------------------------------------- */
/*  Basic runtime variables                     */
/* -------------------------------------------- */

#ifndef __GNUC__
    /* Stack for IDLE Task */
    uint8_t schrx_idle_stack[SCHRX_IDLE_STACK_SIZE];

    /* Original systick handler (exists when Systick Handler is hooked) */
    void (*schrx_systick_handler_origin)(void);

    /* Systick callback for user */
    // extern void SchrX_UserSysTickHandler(void);

    #ifdef SCHRX_CORTEX_FPU_ENABLE
        void schrx_invoke_no_fpu(void);
        void schrx_invoke_fpu(void);
        void (*schrx_schedule_invoke)(void) = 0;
    #endif

#else
    
    /* Stack for IDLE Task */
    extern void *schrx_idle_stack;

    /* Original systick handler (exists when Systick Handler is hooked) */
    extern void (*schrx_systick_handler_origin)(void);

    /* Systick callback for user */
    extern void SchrX_UserSysTickHandler(void);

    #ifdef SCHRX_CORTEX_FPU_ENABLE
        extern void schrx_invoke_no_fpu(void);
        extern void schrx_invoke_fpu(void);
        void (*schrx_schedule_invoke)(void) = 0;
    #endif
    
#endif

    /* Single scheduler instance */
    void *schrx_schr_instance;

    /* System tick counter */
    uint64_t system_tick = 0;
/* -------------------------------------------- */
/*  Port                                        */
/* -------------------------------------------- */

#ifdef __GNUC__

    void __attribute((naked)) schrx_do_idle(void)
    {
        while(1);
    }

    void __attribute((weak)) SchrX_UserSysTickHandler(void)
    {
        if(schrx_systick_handler_origin)
            schrx_systick_handler_origin();
    }

    void schrx_exclusive_add_16(volatile int16_t* _address, int16_t _value)
    {
        asm volatile(
            "__schrx_ea16_begin:\n"
            "\t ldrexh  r2, [%0]\n"
            "\t add     r2, %1\n"
            "\t strexh  r3, r2, [%0]\n"
            "\t cbz     r3, __schrx_ea16_ok\n"
            "\t b       __schrx_ea16_begin\n"
            "__schrx_ea16_ok:\n"
            : 
            : "r"(_address), "r"(_value)
            : "r2", "r3"
        );
    }

    uint32_t schrx_cas_32(volatile uint32_t* _address, uint32_t _compare, uint32_t _exchange)
    {
        register uint32_t ret;
        asm volatile(
            "__schrx_c32_begin:\n"
            "\t ldrex   r3, [%2]\n"
            "\t eor     %1, r3\n"
            "\t cbnz    %1, __schrx_c32_ret\n"
            "\t strex   %1, %3, [%2]\n"
            "\t cbz     %1, __schrx_c32_ret\n"
            "\t mov     %1, r3\n"
            "\t b       __schrx_c32_begin\n"
            "__schrx_c32_ret:\n"
            "\t mov     %0, r3\n"
            : "=r"(ret), "+r"(_compare)
            : "r"(_address), "r"(_exchange)
            : "r3"
        );
        return ret;
    }

    uint8_t schrx_cas_8(volatile uint8_t* _address, uint8_t _compare, uint8_t _exchange)
    {
        register uint8_t ret;
        asm volatile(
            "__schrx_c8_begin:\n"
            "\t ldrexb  r3, [%0]\n"
            "\t eor     %1, r3\n"
            "\t cbnz    %1, __schrx_c8_ret\n"
            "\t strex   %1, %3, [%0]\n"
            "\t cbz     %1, __schrx_c8_ret\n"
            "\t mov     %1, r3\n"
            "\t b       __schrx_c8_begin\n"
            "\t mov     %1, r3\n"
            "__schrx_c8_ret:\n"
            "\t mov     %0, r3\n"
            : "=r"(ret), "+r"(_compare)
            : "r"(_address), "r"(_exchange)
            : "r3"
        );
        return ret;
    }

    uint8_t schrx_exclusive_swap_8(volatile uint8_t* _address, uint8_t _swap)
    {
        register uint8_t ret;
        asm volatile(
            "__schrx_es8_begin:\n"
            "\t ldrexb  r2, [%1]\n"
            "\t strexb  r3, %2, [%1]\n"
            "\t cbz     r3, __schrx_es8_end\n"
            "\t b       __schrx_es8_begin\n"
            "__schrx_es8_end:\n"
            "\t mov 	%0, r2\n"
            : "=r"(ret)
            : "r"(_address), "r"(_swap)
            : "r3" ,"r2"
        );
        return ret;
    }

    uint32_t schrx_exclusive_swap_32(volatile uint32_t* _address, uint32_t _swap)
    {
        register uint32_t ret;
        asm volatile(
            "__schrx_es32_begin:\n"
            "\t ldrex   r2, [%1]\n"
            "\t strex   r3, %2, [%1]\n"
            "\t cbz     r3, __schrx_es32_end\n"
            "\t b       __schrx_es32_begin\n"
            "__schrx_es32_end:\n"
            "\t mov		%0, r2\n"
            : "=r"(ret)
            : "r"(_address), "r"(_swap)
            : "r3", "r2"
        );
        return ret;
    }

    /*
        save context with FP Expansion
    */
    void __attribute((naked)) schrx_invoke_fpu(void)
    {
        asm volatile(
            "\t vpush   {s16-s31}\n"    // Save FPU Register
            "\t mrs     r1, psp\n"      // read user stack
            "\t push    {r1, r4-r11}\n"
            "\t mov     r2, sp\n"
            "\t push    {r1-r2}\n"      // irq context structure
        );

        asm volatile(
            "\t mov     r1, sp\n"       // call schedule precedure
            "\t ldr     r0, %0\n"
            "\t blx     schrx_schedule_routine\n"
            :
            : "m"(schrx_schr_instance)
            : "r0", "r1"
        );

        asm volatile(
            "\t pop     {r0-r2}\n"
            "\t msr     psp, r2\n"
            "\t pop     {r4-r11}\n"
            "\t vpop    {s16-s31}\n"
            "\t pop     {pc}\n"
        );
    }

    /*
        save context with no FP Expansion
    */

    void __attribute((naked, target("thumb"))) schrx_invoke_no_fpu(void)
    {
        asm volatile(
            "\t mrs     r1, psp\n"          // read user stack
            "\t push    {r1, r4-r11}\n"
            "\t mov     r2, sp\n"
            "\t push    {r1-r2}\n"          // irq context
        );

        asm volatile( 
            "\t mov     r1, sp\n"           // call schedule procedure
            "\t ldr     r0, %0\n"
            "\t blx     schrx_schedule_routine\n"
            :
            : "m"(schrx_schr_instance)
            : "r0", "r1"
        );

        asm volatile(
            "\t pop     {r0-r2}\n"
            "\t msr     psp, r2\n"
            "\t pop     {r4-r11}\n"
            
            "\t pop 	{pc}\n"
        );
    }

    /*
        pendsv handler
    */
    void __attribute__((naked, target("thumb"))) schrx_pendsv_handler(void)
    {
        asm volatile("\t push {lr}\n");
        if(system_tick == 16006)
            system_tick = 0;
        asm volatile(
            "\t cbz     %0, __pendsv_ret\n"
            "\t mov     r1, lr\n"
            "\t and     r1, 0x04\n"             // check stack type
            "\t cbz     r1, __pendsv_ret\n"     // in irq routine, do not switch
            :
            : "r"(schrx_schr_instance)
            : "r1"
        );

        #ifndef SCHRX_CORTEX_FPU_ENABLE
            asm volatile("\t bx  schrx_invoke_no_fpu\n");
        #else
            asm volatile(
                "\t bx  %0\n"
                :
                : "r"(schrx_schedule_invoke)
                :
            );
        #endif

        asm volatile(
            "__pendsv_ret:\n"
            "\t pop     {pc}\n"
        );
    }

    /*
        launch precedure
    */
    void __attribute__((target("thumb"))) schrx_core_launch(void *_scheduler)
    { 
        schrx_schr_instance = _scheduler;
        schrx_schedule_suspend(_scheduler);
        schrx_scheduler_attach();

        asm volatile(
            "\t msr     psp, %0\n"
            "\t mrs     r1, CONTROL\n"      // switch to user stack
            "\t orr     r1, #0x2\n"
            "\t msr     CONTROL, r1\n"
            :
            : "r"(schrx_idle_stack + SCHRX_IDLE_STACK_SIZE - 1)
            : "r1"
        );

        schrx_schedule_resume(_scheduler);
        schrx_switch();
        schrx_do_idle();
    }

    uint32_t schrx_is_user_context(void)
    {
        uint32_t ctl;
        
        asm volatile(
            "\t mrs	r0, CONTROL\n"
            "\t str r0, %0\n"
        : "=m"(ctl) ::"r0");
        
        return ctl & 0x02;
    }

    void schrx_irq_critical_enter(void)
    { asm volatile("\t cpsid i\n":::); }

    void schrx_irq_critical_exit(void)
    { asm volatile("\t cpsie i\n":::); }

#endif

/* -------------------------------------------- */
/*  Code                                        */
/* -------------------------------------------- */
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
    #define FPU_ENABLED     (schrx_schedule_invoke == schrx_invoke_fpu)
    #define CORTEX_REALIGNMENT  (1u << 9)

	uint32_t ctx_size;

    #ifdef SCHRX_CORTEX_FPU_ENABLE
    uint8_t i;
    
        if(FPU_ENABLED)
            ctx_size = CORTEX_IRQ_STORE_REGS_SIZE;
        else
            ctx_size = CORTEX_IRQ_STORE_REGS_SIZE_NO_FPU;
    #else
        ctx_size = CORTEX_IRQ_STORE_REGS_SIZE;
    #endif
    
    if( _save ) //Not a IDLE Task
    {
        /* store context */
        _save -> R[0] = _irq_context -> stored -> R0;
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
        _save -> xPSR = _irq_context -> stored -> xPSR;
        _save -> SP = _irq_context -> stored_extra -> SP + ctx_size;

        #ifdef SCHRX_CORTEX_FPU_ENABLE
            if(FPU_ENABLED)
            {
                for(i = 0 ; i < 16; i++)
                {
                    // Save FPU S0-S15
                    _save -> S[i] = _irq_context -> stored -> S[i]; 
                    // Save FPU S16-S30
                    _save -> S[i + 16] = _irq_context -> stored_extra -> S_16[i]; 
                }
                // Save FPSCR
                _save -> FPSCR = _irq_context -> stored -> FPSCR;

                _save -> Reserved[0] = _irq_context -> stored -> Reserved[0];
                if(_save -> xPSR & CORTEX_REALIGNMENT) // Stack is realigned to 8-Byte.
                    _save -> Reserved[1] = _irq_context -> stored -> Reserved[1];
                else
                    _save -> SP -= 4;
            }
            else
                if(!(_save -> xPSR & CORTEX_REALIGNMENT))
                    _save -> SP -= 4;
        #else
            if(!(_save -> xPSR & CORTEX_REALIGNMENT))
                _save -> SP -= 4;
        #endif
    }

    /* restore target thread context */
    if( _new )
    {
        _irq_context -> stored_extra -> R4 = _new -> R[4];
        _irq_context -> stored_extra -> R5 = _new -> R[5];
        _irq_context -> stored_extra -> R6 = _new -> R[6];
        _irq_context -> stored_extra -> R7 = _new -> R[7];
        _irq_context -> stored_extra -> R8 = _new -> R[8];
        _irq_context -> stored_extra -> R9 = _new -> R[9];
        _irq_context -> stored_extra -> R10 = _new -> R[10];
        _irq_context -> stored_extra -> R11 = _new -> R[11];
        _irq_context -> stored_extra -> SP = _new -> SP - ctx_size;
        if(!(_new -> xPSR & CORTEX_REALIGNMENT)) //SP hadn't ever been realigned to 8-Byte.
            _irq_context -> stored_extra -> SP += 4;
        _irq_context -> stored = (SchrX_IRQStoredRegs*) _irq_context -> stored_extra -> SP ;
        _irq_context -> stored -> R0 = _new -> R[0];
        _irq_context -> stored -> R1 = _new -> R[1];
        _irq_context -> stored -> R2 = _new -> R[2];
        _irq_context -> stored -> R3 = _new -> R[3];
        _irq_context -> stored -> R12 = _new -> R[12];
        _irq_context -> stored -> LR = _new -> LR;
        _irq_context -> stored -> PC = _new -> PC;
        _irq_context -> stored -> xPSR = _new -> xPSR;

        #ifdef SCHRX_CORTEX_FPU_ENABLE
            if(FPU_ENABLED)
            {
                for(i = 0 ; i < 16 ; i++)
                {
                    // Restore FPU S0-S15
                    _irq_context -> stored -> S[i] = _new -> S[i];
                    _irq_context -> stored_extra -> S_16[i] = _new -> S[i + 16];
                }
                _irq_context -> stored -> FPSCR = _new -> FPSCR;
            }
        #endif
    }
    else /* IDLE */
    {
        _irq_context -> stored_extra -> SP = (uint32_t) schrx_idle_stack - ctx_size + 4;
        /*
            IDLE Stored register restore.
            IDLE Task is dummy task. it don't care the values of Rx registers.
        */
        _irq_context -> stored = (SchrX_IRQStoredRegs*) _irq_context -> stored_extra -> SP;
        _irq_context -> stored -> LR = 0;
        _irq_context -> stored -> xPSR = 0x01000000;
        
        _irq_context -> stored -> PC = (uint32_t)&schrx_do_idle;
    }

    #undef CORTEX_REALIGNMENT
    #undef FPU_ENABLED
}

#ifdef SCHRX_CORTEX_FPU_ENABLE

    void schrx_cortex_reload_invoke_precedure()
    {
        // Check whether FPU is ENABLED.
        if(SCB -> CPACR & ((3UL << 10*2)|(3UL << 11*2)) )
            schrx_schedule_invoke = schrx_invoke_fpu;
        else
            schrx_schedule_invoke = schrx_invoke_no_fpu;
    }

#endif

void schrx_scheduler_attach(void)
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

    #ifdef SCHRX_CORTEX_FPU_ENABLE
        schrx_cortex_reload_invoke_precedure();
    #endif

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
    
    _context -> SP = (uint32_t)_stack & (~0x7u); //Align to 8-byte border.
    _context -> LR = 0;
    _context -> PC = (uint32_t)_entry;
    _context -> xPSR = 0x01000000;

    #ifdef SCHRX_CORTEX_FPU_ENABLE
        for(i = 0 ; i < 32 ; i++)
            _context -> S[i] = 0;
    #endif
}

