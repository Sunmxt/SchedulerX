#include "semaphore.h"

uint32_t schrx_sem_wake(SchrX_Semaphore *_sem, uint32_t _count)
{
    union
    {
        schrx_sem_wait_token *it;
        bi_list_node *b_it;
    }v1;

    /* optimize */
    v1.b_it = (bi_list_node*) _sem -> waits.next;
    while(v1.b_it -> next)
    {
        v1.b_it -> next -> prev = v1.b_it;
        v1.b_it = v1.b_it -> next;
    }
    
    if(_count) /* wake the thread queue */
    {
        do
        {
            v1.it = S_LIST_TO_DATA(v1.b_it, schrx_sem_wait_token, node);
            SchrX_UnblockThread(v1.it -> thread);
            v1.b_it = v1.it -> node.prev;
            if( !v1.b_it )
            {
                _sem -> waits.next = 0;
                break;
            }
            else
                v1.b_it -> next = 0;
        }while(--_count);
    }
    
    return _count;
}

void schrx_sem_wake_and_unlock(SchrX_Semaphore *_sem)
{
    uint32_t used, old, new, chg;

    used = 0;
    for(old = _sem -> resource ;; old = chg)
    {
        new = (old & SCHRX_SEM_RESOURCE) >> SCHRX_SEM_RESOURCE_POS;
        if( new > used )
            /* Resource found, wake sleepers. */
            /*
                arithmetic simplifiction of : 
                used += (new - used) - schrx_sem_wake(_sem, new - used);
            */
            used = new - schrx_sem_wake(_sem, new - used);

        /* calculate remaining resource count, and unlock the list. */
        new = (new - used) << SCHRX_SEM_RESOURCE_POS;

        chg = schrx_cas_32((volatile uint32_t*)&_sem -> resource, old, new);
        if(chg == old)
            break;
    }
}

SchrXStatus SchrX_SemaphoreCreate(SchrX_Semaphore *_sem)
{
    _sem -> resource = 0;
    _sem -> waits.next = 0;

    return SCHRX_OK;
}

SchrXStatus SchrX_SemaphoreDestroy(SchrX_Semaphore *_sem)
{
    _sem -> resource = 0;
    schrx_sem_wake(_sem, (uint32_t)(-1));
    
    return SCHRX_OK;
}

SchrXStatus SchrX_SemaphorePost(SchrX_Semaphore *_sem, uint32_t _resource)
{
    union
    {
        struct {uint32_t old, new, chg;};
        SchedulerX *schr;
    }v1;

    v1.schr = schrx_get_running_scheduler();
    if(!v1.schr)
        return SCHRX_INVAILED_CONTEXT;

    schrx_schedule_suspend(v1.schr);

    for(v1.old = _sem -> resource ;; v1.old = v1.chg)
    {
        /* add my resource */
        v1.new = (v1.old & SCHRX_SEM_RESOURCE) + (_resource << SCHRX_SEM_RESOURCE_POS);
        if(_sem -> waits.next)
            /* some threads are waiting. */
            v1.new |= SCHRX_SEM_WAIT_LIST_LOCK;

        v1.chg = schrx_cas_32((volatile uint32_t*)&_sem -> resource, v1.old, v1.new);
        if(v1.chg == v1.old)
            break;
    }

    if( (v1.old ^ v1.new) & SCHRX_SEM_WAIT_LIST_LOCK ) /* waiting list is locked by me */
        schrx_sem_wake_and_unlock(_sem); /* wake sleepers and unlock the list */

    v1.schr = schrx_get_running_scheduler();
    schrx_schedule_resume(v1.schr);

    return SCHRX_OK;
}

SchrXStatus SchrX_SemaphoreWait(SchrX_Semaphore *_sem)
{
    union
    {
        schrx_sem_wait_token token;
        struct {uint32_t old, new, chg, used;};
    }v1;
    
    v1.token.thread = SchrX_GetCurrentThread();
    if(!v1.token.thread)
        return SCHRX_INVAILED_CONTEXT;

    schrx_schedule_suspend(v1.token.thread -> scheduler);
    /*
        Disable scheduling.

        Only a wait thread can enter critial section of the semaphore in a time to acquire resources.
        However, some post-operations (from interrupts) may break in. So synchronization is needed. 

        To achieve faster irq response, IRQ mask is not used for critical section.
    */

    for(v1.old = _sem -> resource ;; v1.old = v1.chg)
    {
        if(v1.old & SCHRX_SEM_RESOURCE) 
        /* If resources exist, try to acquire. */
            v1.new = v1.old - SCHRX_SEM_RESOURCE_UNIT;
        else
        /* otherwise, need to join waiting list. acquire waiting list lock. */
            v1.new = v1.old | SCHRX_SEM_WAIT_LIST_LOCK; 
        
        v1.chg = schrx_cas_32((volatile uint32_t*)&_sem -> resource, v1.old, v1.new);
        if(v1.chg == v1.old) /* succeed */
            break;
        /* If resource state is changed, CAS will failed. Then retry. */
    }

    v1.token.thread = SchrX_GetCurrentThread();
    if( (v1.old ^ v1.new) & SCHRX_SEM_WAIT_LIST_LOCK ) /* waiting list lock by me .*/
    {
        /* join the list */
        v1.token.node.prev = 0;
        v1.token.node.next = (bi_list_node*)_sem -> waits.next;
        _sem -> waits.next = (for_list_node*)&v1.token.node;

        schrx_sem_wake_and_unlock(_sem);    /* unlock waiting list */
        schrx_schedule_resume(v1.token.thread -> scheduler);
        SchrX_BlockThread(v1.token.thread);  /* sleep */
    }
    else
        schrx_schedule_resume(v1.token.thread -> scheduler);

    return SCHRX_OK;
}
