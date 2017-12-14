#include "SchedulerX.h"

void schrx_schedule_suspend(SchedulerX *_scheduler)
{
    schrx_exclusive_add_16(&_scheduler -> suspend_count, 1);
}

void schrx_schedule_resume(SchedulerX *_scheduler)
{
    schrx_exclusive_add_16(&_scheduler -> suspend_count, -1);
    if(_scheduler -> flags & SCHRX_PEND_SWITCH)
    {
        _scheduler -> flags &= ~SCHRX_PEND_SWITCH;
        schrx_switch();
    }
}

SchrX_Thread* schrx_pop_front_thread(SchedulerX *_scheduler, uint8_t _priority)
{
    SchrX_Thread *thread;
    for_list_node *head, *tail;
 
    head = 0;
    tail = 0;
    if( _scheduler -> loop.real_time.head.next )
    {
        head = &_scheduler -> loop.real_time.head;
        tail = &_scheduler -> loop.real_time.tail;
    }
    else if( _scheduler -> loop.high.head.next )
    {
        head = &_scheduler -> loop.high.head;
        tail = &_scheduler -> loop.high.tail;
    }
    else if( _scheduler -> loop.medium.head.next )
    {
        head = &_scheduler -> loop.medium.head;
        tail = &_scheduler -> loop.medium.tail;
    }
    else if( _scheduler -> loop.low.head.next )
    {
        head = &_scheduler -> loop.low.head;
        tail = &_scheduler -> loop.low.tail;
    }
    
    if(head)
    {
        thread = S_LIST_TO_DATA(head -> next, SchrX_Thread, wait_node);
        if( !_scheduler -> exec || thread -> priority >= _priority )
        {
            if(head -> next == tail -> next)
                head -> next = tail -> next = 0;
            else
            {
                ((bi_list_node*)head -> next) -> prev = 0;
                head -> next = (for_list_node*)((bi_list_node*)head -> next) -> next;
                thread -> wait_node.next = 0;
            }
            return thread;
        }
    }
    
    thread = _scheduler -> exec;

    return thread;
}


void schrx_push_back_thread(SchedulerX *_scheduler, SchrX_Thread *_thread)
{
    for_list_node *head, *tail;
    
    if( _thread -> state & SCHRX_BLOCKED_MASK )
    {
        _thread -> wait_node.prev = 0;
        _thread -> wait_node.next = (bi_list_node*)_scheduler -> loop.blocked.next;
        _scheduler -> loop.blocked.next = (for_list_node*)&_thread -> wait_node;
    }
    else
    {
        switch( _thread -> priority )
        {
        case ___SCHRX_PIROR_REALRIME:
            head  = &_scheduler -> loop.real_time.head;
            tail  = &_scheduler -> loop.real_time.tail;
            break;

        case ___SCHRX_PIROR_HIGH:
            head = &_scheduler -> loop.high.head;
            tail = &_scheduler -> loop.high.tail;
            break;

        case ___SCHRX_PIROR_MEDIUM:
            head = &_scheduler -> loop.medium.head;
            tail = &_scheduler -> loop.medium.tail;
            break;

        case ___SCHRX_PIROR_LOW:
            head = &_scheduler -> loop.low.head;
            tail = &_scheduler -> loop.low.tail;
            break;
        }

        _thread -> wait_node.prev = (bi_list_node*)tail -> next ;
        _thread -> wait_node.next = 0;
        if(tail -> next)
            ((bi_list_node*)tail -> next) -> next = &_thread -> wait_node;
        tail -> next = (for_list_node*)&_thread -> wait_node;
        if(!head -> next)
            head -> next = tail -> next;
    }
}

SchrXStatus schrx_extract_thread(SchedulerX *_scheduler, SchrX_Thread *_thread)
{
    for_list_node *head, *tail;

    head = tail = 0;
    switch( _thread -> priority )
    {
    case ___SCHRX_PIROR_REALRIME:
        head = &_scheduler -> loop.real_time.head;
        tail = &_scheduler -> loop.real_time.tail;
        break;

    case ___SCHRX_PIROR_HIGH:
        head = &_scheduler -> loop.high.head;
        tail = &_scheduler -> loop.high.tail;
        break;

    case ___SCHRX_PIROR_MEDIUM:
        head = &_scheduler -> loop.medium.head;
        tail = &_scheduler -> loop.medium.tail;
        break;

    case ___SCHRX_PIROR_LOW:
        head = &_scheduler -> loop.low.head;
        tail = &_scheduler -> loop.low.tail;
        break;
    }

    if( !head )
        return SCHRX_INVAILED_PARAMETERS;
    
    if( _thread -> wait_node.prev )
        head -> next = (for_list_node*)_thread -> wait_node.next;
    else
        _thread -> wait_node.prev -> next = _thread -> wait_node.next;

    if( _thread -> wait_node.next )
        tail -> next = (for_list_node*)_thread -> wait_node.prev;
    else
        _thread -> wait_node.next -> prev = _thread -> wait_node.prev;
    _thread -> wait_node.next = _thread -> wait_node.prev = 0;

    return SCHRX_OK;
}

void schrx_scheduler_modify(SchedulerX *_scheduler)
{
    for_list_node *op_node, *op_next;
    uint8_t op;
    SchrX_Thread *thread;

    op_node = (for_list_node*) schrx_exclusive_swap_32((volatile uint32_t*)&_scheduler -> op_list.next, 0);

    while(op_node)
    {
        op_next = op_node -> next;
        thread = (SchrX_Thread*) S_LIST_TO_DATA(op_node, SchrX_Thread, op_node);
        
        op = schrx_exclusive_swap_8((volatile uint8_t*)&thread -> op, 0);
        if(op & SCHRX_OP_PREPARE)
        {
            thread -> node.next = _scheduler -> threads.next;
            _scheduler -> threads.next = (for_list_node*)&thread -> node;
            if( thread -> active_count & SCHRX_PASSIVE_FLAG )
                thread -> state |= SCHRX_BLOCKED_MASK;
            schrx_push_back_thread(_scheduler, thread);
        }
        else if(op & SCHRX_OP_ACTIVE_JUDGE)
        {
            if( (thread -> state & SCHRX_BLOCKED_MASK)
                && !(thread -> active_count & SCHRX_PASSIVE_FLAG) )
            {
                thread -> state &= ~SCHRX_BLOCKED_MASK;
                if( thread -> wait_node.next )
                {
                    thread -> wait_node.next -> prev = thread -> wait_node.prev;
                    thread -> wait_node.next = 0;
                }
                if( !thread -> wait_node.prev )
                    _scheduler -> loop.blocked.next = (for_list_node*)thread -> wait_node.next;
                else
                {
                    thread -> wait_node.prev -> next = thread -> wait_node.next;
                    thread -> wait_node.prev = 0;
                }
                schrx_push_back_thread(_scheduler, thread);
            }
            else if( !(thread -> state & SCHRX_BLOCKED_MASK) 
                && (thread -> active_count & SCHRX_PASSIVE_FLAG) )
            {
                if( !(thread -> state & SCHRX_RUNNING_MASK) )
                {
                    schrx_extract_thread(_scheduler, thread);
                    schrx_push_back_thread(_scheduler, thread);
                }
                thread -> state |= SCHRX_BLOCKED_MASK;
            }
        }
        op_node = op_next;
    }
}

void schrx_schedule_routine(void *_scheduler, SchrX_IRQContext *_irq_context)
{
	uint8_t target_prior;
    SchedulerX *schr;
    SchrX_Thread *target;
    SchrX_Context *ctx_cur, *ctx_target;
    
    schr = (SchedulerX*)_scheduler;
    if( schr -> suspend_count )
    {
        schr -> flags |= SCHRX_PEND_SWITCH;
        return;
    }

    schrx_scheduler_modify(schr);

    ctx_cur = ctx_target = 0;
    if(schr -> exec)
    {
    	if(schr -> exec -> state & SCHRX_TERMINATED_MASK)
    	{
    		schr -> exec = 0;
    		target_prior = ___SCHRX_PIROR_LOW;
    	}
    	if(schr -> exec -> state & SCHRX_BLOCKED_MASK )
            target_prior = ___SCHRX_PIROR_LOW;
    	else
    		target_prior = schr -> exec -> priority;
    }
    else
    	target_prior = ___SCHRX_PIROR_LOW;

    target = schrx_pop_front_thread(schr, target_prior);
    if(target)
    {
        target -> state |= SCHRX_RUNNING_MASK;
        ctx_target = &target -> context;
    }
    if(schr -> exec)
    {
        schr -> exec -> state &= ~SCHRX_RUNNING_MASK;
        schrx_push_back_thread(_scheduler, schr -> exec);
        ctx_cur = &schr -> exec -> context;
    }
    
    schr -> exec = target;
    schrx_context_switch_irq(ctx_cur, ctx_target, _irq_context);
}

void schrx_thread_entry(SchrX_Thread *_thread, SchrX_ThreadEntry _entry, void* params)
{
    _thread -> exit_code = _entry(params);
    SchrX_TerminateThread((SchedulerX*)schrx_get_running_scheduler() , _thread, _thread -> exit_code);

    while(1);   //should never reach.
}


SchrXStatus SchrX_Create(SchedulerX *_scheduler)
{
    _scheduler -> threads.next = 0;
    _scheduler -> loop.real_time.head.next = 0;
    _scheduler -> loop.real_time.tail.next = 0;
    _scheduler -> loop.high.head.next = 0;
    _scheduler -> loop.high.tail.next = 0;
    _scheduler -> loop.medium.tail.next = 0;
    _scheduler -> loop.medium.head.next = 0;
    _scheduler -> loop.low.head.next = 0;
    _scheduler -> loop.low.tail.next = 0;
    _scheduler -> loop.blocked.next = 0;
    _scheduler -> op_list.next = 0;
    _scheduler -> exec = 0;
    _scheduler -> flags = 0;

    #ifdef SCHEDULERX_TIME
        SchrX_CreateTimeTriggerManager(&_scheduler -> timer.manager);
        SchrX_SemaphoreCreate(&_scheduler -> timer.lock, 0);
        SchrX_SemaphoreCreate(&_scheduler -> timer.sem_block, 0);
        _scheduler -> timer.lock.resource = SCHRX_SEM_RESOURCE_UNIT;
        _scheduler -> timer.triggered = 0;
    #endif
    
    return SCHRX_OK;
}

SchrXStatus SchrX_Run(SchedulerX *_scheduler)
{
    if(schrx_get_running_scheduler())
        return SCHRX_SCHEDULER_EXIST;
    
    schrx_core_launch( _scheduler );

    return SCHRX_OK;
}

SchrXStatus SchrX_CreateThread(SchedulerX *_scheduler ,SchrX_Thread *_thread, SchrX_ThreadEntry _entry, void* _user_param, void* _stack, size_t _stack_size, Byte _control)
{   
    if( _stack_size < 1 || !_entry || !_stack )
        return SCHRX_INVAILED_PARAMETERS;


    _thread -> stack = _stack;
    _thread -> stack_size = _stack_size;
    //_thread -> wait_count = 0;
    _thread -> exit_code = 0;
    //_thread -> suspend_count = 0;

    schrx_context_init(&_thread -> context, schrx_thread_entry, (char*)_stack + _stack_size);
    schrx_context_param0(&_thread -> context, (uint32_t)_thread);
    schrx_context_param1(&_thread -> context, (uint32_t)_entry);
    schrx_context_param2(&_thread -> context, (uint32_t)_user_param);

    _thread -> op = SCHRX_OP_PREPARE;
    _thread -> state = 0;
    if( _control & SCHRX_SUSPEND )
        _thread -> active_count = SCHRX_RESUME_COUNT_MASK;

    _thread -> priority = ((_control & SCHRX_CREATE_PRIORITY_MASK) >> SCHRX_CREATE_PRIORITY_POS) + 1;
    _thread -> scheduler = _scheduler;

    schrx_schedule_suspend(_scheduler);
    _thread -> op_node.next = (for_list_node*)schrx_exclusive_swap_32((volatile uint32_t*)&_scheduler -> op_list.next, (uint32_t)&_thread -> op_node);
    schrx_schedule_resume(_scheduler);

    if( _scheduler -> exec
        && _scheduler -> exec -> priority < _thread -> priority 
        && _scheduler == schrx_get_running_scheduler())
        schrx_switch();
        
    return SCHRX_OK;
}

void schrx_thread_active_judge(SchrX_Thread *_thread)
{
    uint8_t old, chg;

    for(old = _thread -> op ; !(old & SCHRX_OP_ACTIVE_JUDGE) ; old = chg)
    {
        chg = schrx_cas_8((volatile uint8_t*)&_thread -> op, old, old | SCHRX_OP_ACTIVE_JUDGE);
        if(chg == old)
            break;
    }

    if( !(old & SCHRX_OP_ACTIVE_JUDGE))
    {
        schrx_schedule_suspend(_thread -> scheduler);
        _thread -> op_node.next = (for_list_node*)schrx_exclusive_swap_32((volatile uint32_t*)&_thread -> scheduler -> op_list.next, (uint32_t)&_thread -> op_node);
        schrx_schedule_resume(_thread -> scheduler);
    }

}

SchrXStatus schrx_core_unblock_thread(SchrX_Thread *_thread, uint32_t _mask, uint32_t _pos)
{
    uint32_t old, chg, new;

    if( !_thread -> scheduler )
        return SCHRX_INVAILED_PARAMETERS;


    for(old = _thread -> active_count ;; old = chg)
    {
        new = ((old + (0x1 << _pos)) & _mask) | (old & (~_mask));
        chg = schrx_cas_32((uint32_t*)&_thread -> active_count, old, new);
        if( chg == old )
            break;
    }
    
    if( !(new & SCHRX_PASSIVE_FLAG) && (old & SCHRX_PASSIVE_FLAG))
    {
        schrx_thread_active_judge(_thread);
        if( ( !_thread -> scheduler -> exec || _thread -> scheduler -> exec -> priority < _thread -> priority
            )&& _thread -> scheduler == schrx_get_running_scheduler())
            schrx_switch(); 
    }

    return SCHRX_OK;
}


SchrXStatus schrx_core_block_thread(SchrX_Thread *_thread, uint32_t _mask, uint32_t _pos)
{
    uint32_t old, chg, _new;

    if( !_thread -> scheduler )
        return SCHRX_INVAILED_PARAMETERS;


    for(old = _thread -> active_count ;; old = chg)
    {
        _new = ((old - (0x1 << _pos)) & _mask) | (old & (~_mask));

        chg = schrx_cas_32((uint32_t*)&_thread -> active_count, old, _new);
        if( chg == old )
            break;
    }


    if( (_new & SCHRX_PASSIVE_FLAG) && !(old & SCHRX_PASSIVE_FLAG) )
    {
        schrx_thread_active_judge(_thread);
        if( _thread -> scheduler == schrx_get_running_scheduler() 
            && _thread -> scheduler -> exec == _thread)
            schrx_switch();
    }

    return SCHRX_OK;
}

SchrXStatus SchrX_SuspendThread(SchrX_Thread *_thread)
{ return schrx_core_block_thread(_thread, SCHRX_RESUME_COUNT_MASK, SCHRX_RESUME_COUNT_POS); }

SchrXStatus SchrX_ResumeThread(SchrX_Thread *_thread)
{ return schrx_core_unblock_thread(_thread, SCHRX_RESUME_COUNT_MASK, SCHRX_RESUME_COUNT_POS); }

SchrXStatus SchrX_BlockThread(SchrX_Thread *_thread)
{ return schrx_core_block_thread(_thread, SCHRX_UNBLOCK_COUNT_MASK, SCHRX_UNBLOCK_COUNT_POS); }

SchrXStatus SchrX_UnblockThread(SchrX_Thread *_thread)
{ return schrx_core_unblock_thread(_thread, SCHRX_UNBLOCK_COUNT_MASK, SCHRX_UNBLOCK_COUNT_POS); }


SchrXStatus SchrX_TerminateThread(SchedulerX *_scheduler, SchrX_Thread *_thread, SchrXExitCode _exit_code)
{
    union 
    {
        SchrXStatus status;
        for_list_node *node;
    }v1;
    
    int is_user_context;

    is_user_context = 0;
    if( schrx_is_user_context() )
        is_user_context = 1;
    else
        schrx_irq_critical_enter();
        
    schrx_schedule_suspend(_scheduler);
    
    v1.node = &_scheduler -> threads;              //remove thread;
    if(v1.node -> next == &_thread -> node)
        v1.node -> next = _thread -> node.next;
    else
    {
        while(v1.node)
        {
            if(v1.node -> next == &_thread -> node)
                break;
            v1.node = v1.node -> next;
        }
        if(!v1.node)
            return SCHRX_INVAILED_PARAMETERS;
        v1.node -> next = v1.node -> next -> next;
    }
    _thread -> node.next = 0;
    v1.status = SCHRX_OK;


    _thread -> exit_code = _exit_code;
    _thread -> state |= SCHRX_TERMINATED_MASK;
    if(_scheduler -> exec == _thread) //the thread is runing.
    {
        // kill the thread
        schrx_schedule_resume(_scheduler);
        if( _scheduler == schrx_get_running_scheduler() ) // scheduler is runing
        {
            if(!is_user_context)
                schrx_irq_critical_exit();

            schrx_switch();
            return v1.status;
        }
    }
    else
        v1.status = schrx_extract_thread(_scheduler, _thread);

    schrx_schedule_resume(_scheduler);
    if(!is_user_context)
        schrx_irq_critical_exit();

    return v1.status;
}

SchrX_Thread* SchrX_GetCurrentThread(void)
{
    SchedulerX *schr;
    
    schr = schrx_get_running_scheduler();
    if( !schrx_is_user_context()
        || !schr )
        return 0;

    return schr -> exec;
}
