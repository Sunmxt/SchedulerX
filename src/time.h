#ifndef SCHEDULERX_TIME_HEADER
#define SCHEDULERX_TIME_HEADER

//#include "SchedulerX.h"


/* --------------------------------- */
/*  Basic                            */
/* --------------------------------- */
inline uint64_t __attribute__((always_inline)) SchrX_GetSystemTick(void)
{ return schrx_get_system_tick(); }

void SchrX_Delay(uint32_t _period_ms);

/* --------------------------------- */
/*  Time trigger                     */
/* --------------------------------- */

typedef struct SchedulerX_Time_Trigger {
    void            (*trigger_process)(struct SchedulerX_Time_Trigger *_trigger);
    uint64_t        tick_point;
    void*           params;
    for_list_node   node;
}SchrX_TimeTrigger;

typedef struct SchedulerX_Time_Trigger_Manager {
    for_list_node *queue;
    for_list_node *append;
}SchrX_TimeTriggerManager;


SchrXStatus     schrx_register_time_trigger(SchrX_TimeTriggerManager *_manager, SchrX_TimeTrigger *_trigger, uint64_t _tick_point);
uint32_t        schrx_time_trigger(SchrX_TimeTriggerManager *_manager, uint64_t _tick_point);
SchrXStatus     SchrX_CreateTimeTriggerManager(SchrX_TimeTriggerManager *_manager);
SchrXStatus     SchrX_DestroyTimeTriggerManager(SchrX_TimeTriggerManager *_manager);
SchrXStatus     SchrX_RegisterTimeTrigger(SchrX_TimeTrigger *_trigger, uint64_t _tick_point);


/* --------------------------------- */
/*  Timer                            */
/* --------------------------------- */

typedef struct SchedulerX_Timer {
    const char  *name;
    uint64_t    period;
    uint8_t     flags;
    void        (*timer_process)(struct SchedulerX_Timer *_timer);
    #define     SCHRX_TIMER_STOPPED_POS     0
    #define     SCHRX_TIMER_STOPPED         (1u << SCHRX_TIMER_STOPPED_POS)
    #define     SCHRX_TIMER_REGISTERED_POS  1
    #define     SCHRX_TIMER_REGISTERED      (1u << SCHRX_TIMER_REGISTERED_POS)

    #define     SCHRX_TIMER_ATTRIBUTE_POS   2
    #define     SCHRX_TIMER_ATTRIBUTE_MASK  (1u << SCHRX_TIMER_ATTRIBUTE_POS)
    #define     SCHRX_TIMER_ONESHOT_POS     (SCHRX_TIMER_ATTRIBUTE_POS + 0)
    #define     SCHRX_TIMER_ONESHOT         (1u << SCHRX_TIMER_ONESHOT_POS)

    SchrX_TimeTrigger trigger;
    void* params;
}SchrX_Timer;

SchrXStatus SchrX_TimerServe(void);
SchrXStatus SchrX_TimerRegister(SchrX_Timer *_timer);
// SchrXStatus SchrX_TimerStop(SchrX_Timer *_timer);
SchrXStatus SchrX_TimerCreate(SchrX_Timer *_timer, const char *_name, uint64_t _period_ms, uint8_t _flags);


#endif

