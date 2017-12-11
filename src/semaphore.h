#ifndef SCHEDULERX_SEMAPHORE_HEADER
#define SCHEDULERX_SEMAPHORE_HEADER

// #include "SchedulerX.h"

typedef struct _SchedulerX_Semaphore_Wait_Token
{
    bi_list_node node;
    SchrX_Thread *thread;
    uint8_t flags;
    #define SEM_TOKEN_SPIN_BIT  0x01
}schrx_sem_wait_token;

typedef struct _SchedulerX_Semaphore
{
    uint32_t resource;
    #define SCHRX_SEM_LOCK_BIT_POS          0
    #define SCHRX_SEM_DESTROYED_POS         1
    #define SCHRX_SEM_RESOURCE_POS          2

    #define SCHRX_SEM_WAIT_LIST_LOCK        (1u << SCHRX_SEM_LOCK_BIT_POS)
    #define SCHRX_SEM_DESTROYED             (1u << SCHRX_SEM_DESTROYED_POS)
    #define SCHRX_SEM_RESOURCE_UNIT         (1u << SCHRX_SEM_RESOURCE_POS)
    #define SCHRX_SEM_RESOURCE              ((-1) << SCHRX_SEM_RESOURCE_POS)

    for_list_node waits;
}SchrX_Semaphore;

SchrXStatus SchrX_SemaphoreCreate(SchrX_Semaphore *_sem, uint32_t _resource);
SchrXStatus SchrX_SemaphorePost(SchrX_Semaphore *_sem, uint32_t _resource);
SchrXStatus SchrX_SemaphoreWait(SchrX_Semaphore *_sem);
SchrXStatus SchrX_SemaphoreDestroy(SchrX_Semaphore *_sem);

#endif
