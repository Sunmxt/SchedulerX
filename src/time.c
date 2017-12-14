// #include "time.h"
// #include "semaphore.h"

#include "SchedulerX.h"

SchrXStatus SchrX_CreateTimeTriggerManager(SchrX_TimeTriggerManager *_manager)
{
    _manager -> queue = 0;
    _manager -> append = 0;
    return SCHRX_OK;
}

//*
void schrx_time_trigger_all(for_list_node *_list)
{
    SchrX_TimeTrigger *cur;
    for_list_node *next;

    while(_list)
    {
        cur = S_LIST_TO_DATA(_list, SchrX_TimeTrigger, node);
        next = cur -> node.next;
        cur -> trigger_process(cur);
        _list = next;
    }
}

//*
SchrXStatus SchrX_DestroyTimeTriggerManager(SchrX_TimeTriggerManager *_manager)
{
    for_list_node *it;

    it = schrx_exclusive_pointer_swap(&_manager -> append, 0);
    schrx_time_trigger_all(it);

    it = schrx_exclusive_pointer_swap(&_manager -> queue, 0);
    schrx_time_trigger_all(it);

    return SCHRX_OK;
}

SchrXStatus schrx_register_time_trigger(SchrX_TimeTriggerManager *_manager, SchrX_TimeTrigger *_trigger, uint64_t _tick_point)
{
    for_list_node *chg;
    uint64_t current_tick;
    
    current_tick = SchrX_GetSystemTick();

    _trigger -> tick_point = _tick_point;
    if(current_tick >= _tick_point) // No need to append trigger.
    {
        _trigger -> trigger_process(_trigger);
        return SCHRX_OK;
    }

    // append
    chg = _manager -> append;
    do
    {
        _trigger -> node.next = chg;
        chg = schrx_pointer_cas(&_manager -> append, _trigger -> node.next, &_trigger -> node);
    }while(chg != _trigger -> node.next);

    return SCHRX_OK;
}

for_list_node* schrx_time_trigger_merge(for_list_node* _first, for_list_node* _second)
{
    SchrX_TimeTrigger *first, *second, *tmp;

    first = S_LIST_TO_DATA(_first, SchrX_TimeTrigger, node);
    second = S_LIST_TO_DATA(_second, SchrX_TimeTrigger, node);

    if(first -> tick_point > second -> tick_point)
    {
    	tmp = second; second = first; first = tmp;
    	_first = _second;
    }

    for(;;)
    {
        while(first -> node.next)
        {
            tmp = S_LIST_TO_DATA(first -> node.next, SchrX_TimeTrigger, node);
            if(tmp -> tick_point > second -> tick_point)
                break;

            first = tmp;
        }
        if(!first -> node.next)
            break;

        first -> node.next = &second -> node;
        first = second;
        second = tmp;
    }
    
    first -> node.next = &second -> node;

    return _first;
}

for_list_node* schrx_time_trigger_sort(for_list_node* _node)
{
    for_list_node *mid, *it;

    if(!_node -> next)
        return _node;

    it = mid = _node;
    while(it -> next)
    {
        it = it -> next;
        if(!it -> next) 
            break;
        it = it -> next;
        mid = mid -> next;
    }
    
    // Dissolve
    it = mid -> next;
    mid -> next = 0;
    _node = schrx_time_trigger_sort(_node);
    it = schrx_time_trigger_sort(it);

    // Merge
    return schrx_time_trigger_merge(_node ,it);
}

void schrx_time_trigger_list_update(SchrX_TimeTriggerManager *_manager)
{
    for_list_node *new, *it;

    it = _manager -> queue;
    new = schrx_exclusive_pointer_swap(&_manager -> append, 0);

    if(new)
    {
    	new = schrx_time_trigger_sort(new);
    	if(it)
    		it = schrx_time_trigger_merge(it, new);
    	else
    		it = new;
    }

    _manager -> queue = it;
}

uint32_t schrx_time_trigger(SchrX_TimeTriggerManager *_manager, uint64_t _tick_point)
{
    for_list_node *next;
    SchrX_TimeTrigger *trigger;
    uint32_t count;
    
    schrx_time_trigger_list_update(_manager);
    if(!_manager -> queue)
        return 0;

    count = 0;
    next = _manager -> queue;
    while(next)
    {   
        trigger = S_LIST_TO_DATA(next, SchrX_TimeTrigger, node);
        if(trigger -> tick_point > _tick_point)
            break;

        count++;
        next = trigger -> node.next;
        trigger -> node.next = 0;
        trigger -> trigger_process(trigger);
    }

    // Update new head
    _manager -> queue = next;

    return count;
}

void schrx_tick_routine(void *_schrx, uint64_t _tick)
{ schrx_time_trigger(&((SchedulerX*)_schrx) -> timer.manager, _tick); }
/* --------------------------------- */

SchrXStatus SchrX_RegisterTimeTrigger(SchrX_TimeTrigger *_trigger, uint64_t _tick_point)
{
    SchedulerX *schr;

    if(!_trigger)
        return SCHRX_INVAILED_PARAMETERS;

    schr = schrx_get_running_scheduler();
    if(!schr)
        return SCHRX_INVAILED_CONTEXT;

    return schrx_register_time_trigger(&schr ->timer.manager, _trigger, _tick_point);
}

/* ---------------------------------- */

void schrx_delay_trigger_process(SchrX_TimeTrigger *_trigger)
{ SchrX_SemaphorePost((SchrX_Semaphore*)_trigger -> params, 1); }

void SchrX_Delay(uint32_t _period_ms)
{
    SchrX_Semaphore sem_wake;
    SchrX_TimeTrigger trigger;

    trigger.node.next = 0;
    trigger.params = &sem_wake;
    trigger.trigger_process = schrx_delay_trigger_process;

    SchrX_SemaphoreCreate(&sem_wake, 0);
    SchrX_RegisterTimeTrigger(&trigger, SchrX_GetSystemTick() + _period_ms);

    SchrX_SemaphoreWait(&sem_wake);
    SchrX_SemaphoreDestroy(&sem_wake);
}

/* ---------------------------------- */
/*  Timer                             */
/* ---------------------------------- */


void schrx_timer_trigger_process(SchrX_TimeTrigger *_trigger)
{
    SchrX_Timer *old, *chg, *timer;
    SchedulerX *schr;

    schr = schrx_get_running_scheduler();
    if(!schr)
        return;

    // push to wake list
    timer = (SchrX_Timer*)_trigger -> params;
    chg = schr -> timer.triggered;
    do
    {
    	old = chg;
    	if(old)
    		_trigger -> node.next = &old -> trigger.node;
    	else
    		_trigger -> node.next = 0;
        chg = (SchrX_Timer*)schrx_pointer_cas(&schr -> timer.triggered, old, timer);
    }while(chg != old);

    SchrX_SemaphorePost(&schr -> timer.sem_block, 1); // Wake server threads.
}

SchrXStatus SchrX_TimerCreate(SchrX_Timer *_timer, const char *_name, uint64_t _period_ms, uint8_t _flags)
{
    if(!_timer || !_period_ms)
        return SCHRX_INVAILED_PARAMETERS;

    _timer -> name = _name;
    _timer -> period = _period_ms;
    _timer -> flags = _flags & SCHRX_TIMER_ATTRIBUTE_MASK;
    _timer -> trigger.trigger_process = schrx_timer_trigger_process;
    _timer -> trigger.tick_point = 0;
    _timer -> trigger.params = _timer;

    return SCHRX_OK;
}

SchrXStatus SchrX_TimerRegister(SchrX_Timer *_timer)
{
    if(!_timer)
        return SCHRX_INVAILED_PARAMETERS;

    if(_timer -> flags & SCHRX_TIMER_REGISTERED)
        return SCHRX_OK;

    schrx_exclusive_or8(&_timer -> flags, SCHRX_TIMER_REGISTERED);
    schrx_exclusive_and8(&_timer -> flags, SCHRX_TIMER_STOPPED);
    return SchrX_RegisterTimeTrigger(&_timer -> trigger, SchrX_GetSystemTick() + _timer -> period);
}

SchrXStatus SchrX_TimerServe(void)
{
    SchrXStatus status;
    SchedulerX *schr;
    SchrX_Timer *timer;

    schr = schrx_get_running_scheduler();
    if(!schr)
        return SCHRX_INVAILED_CONTEXT;

    // Wait for a timer.
    status = SchrX_SemaphoreWait(&schr -> timer.sem_block);
    if(SCHRX_OK != status)
        return status;

    status = SchrX_SemaphoreWait(&schr -> timer.lock);
    if(SCHRX_OK != status)
        return status;
    timer = schr -> timer.triggered;
    if(timer -> trigger.node.next)
    	schr -> timer.triggered = S_LIST_TO_DATA(timer -> trigger.node.next, SchrX_Timer, trigger.node);
    SchrX_SemaphorePost(&schr -> timer.lock, 1);

    if(timer -> timer_process)
        timer -> timer_process(timer);

    if(timer -> flags & (SCHRX_TIMER_ONESHOT | SCHRX_TIMER_STOPPED))
    {
        schrx_exclusive_or8(&timer -> flags, SCHRX_TIMER_STOPPED);
        schrx_exclusive_and8(&timer -> flags, SCHRX_TIMER_REGISTERED);
        return SCHRX_OK;
    }

    return SchrX_RegisterTimeTrigger(&timer -> trigger, timer -> trigger.tick_point + timer -> period);
}
