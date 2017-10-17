/*

    SchedulerX

    A tiny scheduler for Embedded Processors.

*/

#ifndef SCHEDULERX
#define SCHEDULERX


#include "klist.h"
#include "platform.h"

typedef uint32_t size_t;
typedef unsigned char Byte;

/* Commom */
typedef int SchrXStatus;
#define SCHRX_OK                            0       /* No Error */
#define SCHRX_SYSTICK_IRQ_READONLY          1       /* IRQ Vector is readonly */
#define SCHRX_SCHEDULER_EXIST               -1      /* Scheduler is already exist */
#define SCHRX_INVAILED_PARAMETERS           -2      /* Invailed Parameters */
#define SCHRX_ACCESS_DENIED                 -3      /* Access Denied */
#define SCHRX_INVAILED_CONTEXT              -4      /* Invailed thread context */

typedef uint32_t SchrXExitCode;


/* Thread Control Block */
typedef struct _SchedulerX_Thread
{
    SchrX_Context context;

    void* stack;
    size_t stack_size;

    struct _SchedulerX_Instance *scheduler;
    SchrXExitCode exit_code;
    
    volatile uint8_t op;
    #define SCHRX_OP_PREPARE            0x01
    #define SCHRX_OP_ACTIVE_JUDGE       0x02

    uint8_t state;
    #define SCHRX_RUNNING_MASK          0x01
    #define SCHRX_BLOCKED_MASK          0x02
    #define SCHRX_TERMINATED_MASK       0x04
    #define SCHRX_TERMINATING_MASK      0x08

    volatile uint32_t active_count;
    #define SCHRX_UNBLOCK_COUNT_POS     16
    #define SCHRX_RESUME_COUNT_POS      0
    #define SCHRX_UNBLOCK_COUNT_MASK    0xFFFF0000
    #define SCHRX_RESUME_COUNT_MASK     0x0000FFFF
    #define SCHRX_PASSIVE_FLAG          0x80008000

    uint8_t priority;
    #define SCHRX_PRIORITY_REALTIME     4
    #define SCHRX_PRIORITY_HIGH         3
    #define SCHRX_PRIORITY_MEDIUM       2
    #define SCHRX_PRIORITY_LOW          1

    for_list_node node;
    for_list_node op_node;
    bi_list_node wait_node;
}SchrX_Thread;

/* Thread Process Entry */
typedef SchrXExitCode (*SchrX_ThreadEntry)(void* _params);


/* Scheduler */
typedef struct _SchedulerX_Instance 
{
    uint8_t flags;
    #define SCHRX_PEND_SWITCH   0x01
    volatile int16_t suspend_count;

    for_list_node op_list;
    for_list_node threads;

    SchrX_Thread *exec;
    
    struct {
        struct {
            for_list_node head;
            for_list_node tail;
        } real_time;

        struct {
            for_list_node head;
            for_list_node tail;
        } high;

        struct {
            for_list_node head;
            for_list_node tail;
        } medium;

        struct {
            for_list_node head;
            for_list_node tail;
        } low;

        for_list_node blocked;
    }loop;

}SchedulerX;

/* Create a scheduler */
SchrXStatus SchrX_Create(SchedulerX *_scheduler);

/* Stop current scheduler and exit */
SchrXStatus SchrX_Exit(void);

/* 
    Run scheduler
*/
SchrXStatus SchrX_Run(SchedulerX *_scheduler);




/*
    Create a thread
*/
SchrXStatus SchrX_CreateThread(SchedulerX *_scheduler ,SchrX_Thread *_thread
            , SchrX_ThreadEntry _entry, void* _user_param, void* _stack, size_t _stack_size, Byte _control);

    #define SCHRX_CREATE_SUSPEND                0x01
    #define SCHRX_CREATE_PRIORITY_POS           1
    #define SCHRX_CREATE_PRIORITY_MASK          (3UL << SCHRX_CREATE_PRIORITY_POS)
    #define SCHRX_CREATE_LOW_PRIORITT           (0x00 << SCHRX_CREATE_PRIORITY_POS)
    #define SCHRX_CREATE_MEDIUM_PRIORITY        (0x01 << SCHRX_CREATE_PRIORITY_POS)
    #define SCHRX_CREATE_HIGH_PRIORITY          (0x02 << SCHRX_CREATE_PRIORITY_POS)
    #define SCHRX_CREATE_REAL_TIME_PRIOTITY     (0x03 << SCHRX_CREATE_PRIORITY_POS)

/*
    Suspend/Resume a thread
*/
SchrXStatus SchrX_SuspendThread(SchrX_Thread *_thread);
SchrXStatus SchrX_ResumeThread(SchrX_Thread *_thread);

/*
    Block functions

    Do not use for suspend/resume. use SchrX_SuspendThread and SchrX_ResumeThread instead.
*/

SchrXStatus SchrX_BlockThread(SchrX_Thread *_thread);
SchrXStatus SchrX_UnblockThread(SchrX_Thread *_thread);


/*
    terminate a thread

*/
SchrXStatus SchrX_TerminateThread(SchedulerX *_scheduler, SchrX_Thread *_thread, SchrXExitCode _exit_code);

/*
    Get the running thread.
*/
SchrX_Thread* SchrX_GetCurrentThread(void);

/*
    [- Private -]
    suspend/resume scheduler
*/
void schrx_schedule_suspend(SchedulerX *_scheduler);
void schrx_schedule_resume(SchedulerX *_scheduler);

#endif
