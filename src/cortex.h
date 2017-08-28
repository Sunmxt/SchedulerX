#ifndef SCHEDULERX_ARM_HEADER
#define SCHEDULERX_ARM_HEADER


typedef struct _SchedulerX_ARM_Context
{
    uint32_t R[13];
    uint32_t SP;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
}SchrX_ARMContext;

typedef struct SchedulerX_Cortex_M3_IRQ_Store_Registers 
{
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
}SchrX_IRQStoredRegs;

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

