    EXPORT  SysTick_Handler
    EXPORT  PendSV_Handler

    IMPORT  schrx_systick_handler
    IMPORT  schrx_pendsv_handler

    AREA    |.text|, CODE, READONLY
    PRESERVE8
    THUMB

SysTick_Handler 		PROC
        B   schrx_systick_handler
    ENDP

PendSV_Handler          PROC
        B   schrx_pendsv_handler
    ENDP
    END
