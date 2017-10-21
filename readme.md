# SchedulerX
***A tiny RTOS scheduler for embedded processors.***


---
##### Supported devices:
- ARM Cortex-M3 MCUs (e.g. STMicroelectronics STM32F1 Series)
- ARM Cortex-M4 MCUs (e.g: STMicroelectronics STM32F4 Series)
- Other Cortex-M3/M4 MCUs.

And more devices will be supported later.

---
## How to use?

- **Build SchedulerX with your project.**
 1. ***Select target platform***
  *Define platfrom macro before **#include "SchedulerX.h"** *
  ```
  //Select target (one of them)
  #define SCHRX_STM32F10X    // for Microelectronics STM32f10X series
                                // stm32f10x.h is needed
  #define SCHRX_STM32F4XX    // for Microelectronics STM32F4XX series
                                // stm32f4xx.h is needed
  #define SCHRX_CORE_M3      // other Cortex-M3 MCUs
                                // core_m3.h is needed
  #define SCHRX_CORE_M4      // other Cortex-M4 MCUs
                                // core_m4.h is needed
  ```
 2. ***Compile:***
 ``` 
 cortex.c SchedulerX.c semaphore.c SchedulerXCoreCortexM.s
 ```
 
 3. ***Interrupt Setup***
   *For STM32 MCUs using official startup files, compile:*
   ```
   SchedulerXCoreCortexM.s
   ```
   *For others whose IRQ Vector is readonly, make sure:*
   ```
   schrx_pendsv_handler();
   schrx_systick_handler();
   ```
    is **PendSV** and **SysTick** handler.

- **Use APIs**
  **Thread APIs** and **Scheduler APIs** is defined in **SchedulerX.h**
  **Semaphore APIs** is defined in **Semaphore.h**
  
---

## API Quick Reference

- [Scheduler](#_scheduler)
- [Thread](#_thread)
- [Semaphore](#_semaphore)
 
 
**<span id = "_scheduler">Scheduler APIs:</span>**
- Create
 ```
      /*
          Create new scheduler instance.
      @params:
          _scheduler : pointer to SchedulerX structure.
      */
      SchrXStatus SchrX_Create(SchedulerX *_scheduler);
 ```
 
- Run
 ```
      /*
          Start scheduler.
      @params:
          _scheduler : pointer to SchedulerX structure.
     */
      SchrXStatus SchrX_Run(SchedulerX *_scheduler);
 ```
 
**<span id = "_thread">Thread APIs</span>:**
- Create
 ```
 /*
     Create a new thread.
 @params:
     _scheduler    : Pointer to scheduler instance.
     _thread       : Pointer to thread instance.
     _entry        : Thread's entry point.
     _user_params  : User params.
     _stack        : Pointer to stack memory.
     _stack_size   : Stack size.
     _control      : Control flags (can be combination of following values)
             SCHRX_CREATE_SUSPEND            Create and susupend the thread.
                    
         Priority (one of them):
             SCHRX_CREATE_LOW_PRIORITY       Low priority
             SCHRX_CREATE_MEDIUM_PRIORITY    Medium priority
             SCHRX_CREATE_HIGH_PRIORTITY     High priority
             SCHRX_CREATE_REAL_TIME_PRIORITY Real time priority
 */
 SchrXStatus SchrX_CreateThread(SchedulerX *_scheduler ,SchrX_Thread *_thread , SchrX_ThreadEntry _entry, void* _user_param, void* _stack, size_t _stack_size, Byte _control);
 ```
- Suspend/Resume
 ```
        /*
        	Suspend a thread.
        */
        SchrXStatus SchrX_SuspendThread(SchrX_Thread *_thread);
        /*
            Resume a thread.
        */
        SchrXStatus SchrX_ResumeThread(SchrX_Thread *_thread);
 ```

**<span id = "_semaphore">Semaphore APIs:</span>**
- Create
 ```
 SchrXStatus SchrX_SemaphoreCreate(SchrX_Semaphore *_sem);
 ```

- Post
 ```
 /*
     Post
 @params:
     _sem         : pointer to semaphore.
     _resource    : resource count.
 */
 SchrXStatus SchrX_SemaphorePost(SchrX_Semaphore *_sem, uint32_t _resource);
 ```
- Wait
 ```
 SchrXStatus SchrX_SemaphoreWait(SchrX_Semaphore *_sem);
 ```
- Destroy
 ```
 SchrXStatus SchrX_SemaphoreDestroy(SchrX_Semaphore *_sem);
 ```

---
Just a beginning. Have fun :)