#ifndef SCHEDULERX_ARM_HEADER
#define SCHEDULERX_ARM_HEADER

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    #define SCHRX_CORTEX_FPU_ENABLE
#endif

typedef struct _SchedulerX_ARM_Context
{
    #ifdef SCHRX_CORTEX_FPU_ENABLE
        uint32_t Reserved[2];
        uint32_t FPSCR;
        uint32_t S[32];
    #endif
    uint32_t R[13];
    uint32_t SP;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
}SchrX_ARMContext;

typedef struct SchedulerX_Cortex_IRQ_Store_Registers 
{
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
    #ifdef SCHRX_CORTEX_FPU_ENABLE
        uint32_t S[16];
        uint32_t FPSCR;
        uint32_t Reserved[2];
    #else
        uint32_t Reserved[1];
    #endif
}SchrX_IRQStoredRegs;
#define CORTEX_IRQ_STORE_REGS_SIZE sizeof(SchrX_IRQStoredRegs)
#ifdef SCHRX_CORTEX_FPU_ENABLE
    #define CORTEX_IRQ_STORE_REGS_SIZE_NO_FPU \
        (sizeof(SchrX_IRQStoredRegs) - sizeof(uint32_t)*16 - 4 - 8)
#endif

typedef struct SchedulerX_Extra_IRQ_Store_Register
{
    uint32_t SP;
    uint32_t R4;
    uint32_t R5;
    uint32_t R6;
    uint32_t R7;
    uint32_t R8;
    uint32_t R9;
    uint32_t R10;
    uint32_t R11;
    #ifdef SCHRX_CORTEX_FPU_ENABLE
        uint32_t S_16[16]; //FPU Registers S16-S30
    #endif
    uint32_t irq_exc_ret;
}SchrX_IRQStoredRegsExtra;

typedef struct SchedulerX_IRQ_Context
{
    SchrX_IRQStoredRegs *stored;
    SchrX_IRQStoredRegsExtra *stored_extra;
}SchrX_IRQContext;

typedef SchrX_ARMContext SchrX_Context;

#define schrx_context_param0(_context_ptr, param)   ((_context_ptr) -> R[0] = param)
#define schrx_context_param1(_context_ptr, param)   ((_context_ptr) -> R[1] = param)
#define schrx_context_param2(_context_ptr, param)   ((_context_ptr) -> R[2] = param)
#define schrx_context_param3(_context_ptr, param)   ((_context_ptr) -> R[3] = param)

void schrx_do_idle(void);
void schrx_systick_handler(void);
void schrx_pendsv_handler(void);


#endif

