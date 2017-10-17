#ifndef SCHEDULERX_PLATFORM
#define SCHEDULERX_PLATFORM

/*
    Target platform 
*/

/* If your device can be found in the following macro list, please directly use it. */
    /* #define SCHRX_STM32F10X */       /* STMicroelectronics STM32F10x Series */
    /* #define SCHRX_STM32F4XX */       /* STMicroelectronics STM32F4xx Series */

/* If your device cannot be found in the macro list above, please use following macro */
    /* #define SCHRX_ARM_M3 */          /* Cortex-M3 */
    /* #define SCHRX_ARM_M4 */          /* Cortex-M4 */



#ifdef SCHRX_STM32F10X
    #include "stm32f10x.h"

    #define SCHRX_ARM
    #define SCHRX_ARM_M3
#elif defined(SCHRX_STM32F4XX)
    #include "stm32f4xx.h"

    #define SCHRX_ARM
    #define SCHRX_ARM_M4
#else
    #if defined(SCHRX_ARM_M3) || defined(SCHRX_ARM_M4)
        #define SCHRX_ARM
    #endif
#endif


#ifndef SCHRX_ARM
    #error "SchedulerX: Unknown processor."
#else
    
    #if defined(SCHRX_ARM_M3)
        #include "core_cm3.h"
    #elif defined(SCHRX_ARM_M4)
        #include "core_cm4.h"
    #else
        #error "SchedulerX : Unknown arm processor."
    #endif

    #include "cortex.h"
#endif


void        schrx_context_switch_irq(SchrX_Context *_save, SchrX_Context *_new, SchrX_IRQContext *_irq_context);
void        schrx_switch(void);
void        schrx_schedule_routine(void* _scheduler, SchrX_IRQContext *_irq_context);
void        schrx_core_launch(void *_scheduler);
void        schrx_context_init(SchrX_Context *_context, void* _entry, void* _stack);
void*       schrx_get_running_scheduler(void);
uint32_t    schrx_is_user_context(void);
void        schrx_irq_critical_exit(void);
void        schrx_irq_critical_enter(void);
uint64_t    schrx_get_system_tick(void);
void        schrx_reset_system_tick(void);


void        schrx_exclusive_add_16(volatile int16_t* _address, int16_t _value);
uint32_t    schrx_exclusive_swap_32(volatile uint32_t* _address, uint32_t _swap);
uint8_t     schrx_exclusive_swap_8(volatile uint8_t* _address, uint8_t _swap);
uint32_t    schrx_cas_32(volatile uint32_t* _address, uint32_t _compare, uint32_t _exchange);
uint8_t     schrx_cas_8(volatile uint8_t* _address, uint8_t _compare, uint8_t _exchange);

#endif


