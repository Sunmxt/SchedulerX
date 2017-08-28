#ifndef SCHEDULERX_SEMAPHORE
#define SCHEDULERX_SEMAPHORE

#include "SchedulerX.h"

typedef struct _SchedulerX_Semaphore_Wait_Token
{
    bi_list_node node;
    SchrX_Thread *thread;
}schrx_sem_wait_token;

typedef struct _SchedulerX_Semaphore
{
    uint32_t resource;
    #define SCHRX_SEM_RESOURCE_POS          1
    #define SCHRX_SEM_WAIT_LIST_LOCK        0x00000001
    #define SCHRX_SEM_RESOURCE_UNIT         2
    #define SCHRX_SEM_RESOURCE              0xFFFFFFFE

    for_list_node waits;
}SchrX_Semaphore;

SchrXStatus SchrX_SemaphoreCreate(SchrX_Semaphore *_sem);
SchrXStatus SchrX_SemaphorePost(SchrX_Semaphore *_sem, uint32_t _resource);
SchrXStatus SchrX_SemaphoreWait(SchrX_Semaphore *_sem);
SchrXStatus SchrX_SemaphoreDestroy(SchrX_Semaphore *_sem);

#endif
