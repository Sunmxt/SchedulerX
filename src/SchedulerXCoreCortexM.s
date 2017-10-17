SCHRX_IDLE_STACK_SIZE   EQU     400
    EXPORT  schrx_switch                    [WEAK]
    EXPORT  SchrX_UserSysTickHandler        [WEAK]
    EXPORT  schrx_schedule_routine          [WEAK]
    EXPORT  schrx_schedule_suspend          [WEAK]
    EXPORT  schrx_schedule_resume           [WEAK]
    EXPORT  schrx_scheduler_attach          [WEAK]
	EXPORT	schrx_systick_handler           [WEAK]
    
    EXPORT  schrx_pendsv_handler
    EXPORT  schrx_systick_handler_origin
    EXPORT  schrx_exclusive_add_16
    EXPORT  schrx_exclusive_swap_32
    EXPORT  schrx_exclusive_swap_8
    EXPORT  schrx_cas_32
    EXPORT  schrx_cas_8
    EXPORT  schrx_is_user_context
    EXPORT  schrx_do_idle
    EXPORT  schrx_core_launch
    EXPORT  schrx_irq_critical_enter
    EXPORT  schrx_irq_critical_exit
    
    EXPORT  schrx_schr_instance             [WEAK]
    EXPORT  schrx_idle_stack        

    AREA    SCHRX_RUNTIME, DATA, READWRITE
schrx_schr_instance
schrx_systick_handler_origin    DCD     0
                                SPACE   SCHRX_IDLE_STACK_SIZE
schrx_idle_stack_begin

    AREA    SCHRX_INFO, DATA, READONLY
schrx_idle_stack        DCD     schrx_idle_stack_begin    


    AREA    |.text|, CODE, READONLY
    PRESERVE8
    THUMB

schrx_switch
schrx_schedule_routine
schrx_schedule_suspend
schrx_schedule_resume
schrx_scheduler_attach
schrx_systick_handler


schrx_do_idle   PROC
        B       .
    ENDP
                   
SchrX_UserSysTickHandler    PROC
        LDR     R0, =schrx_systick_handler_origin
        LDR     R0, [R0]
        CBZ     R0, __origin_invoke_finish
        PUSH    {LR}                    
        BLX     R0
        POP     {LR}

__origin_invoke_finish  
        BX      LR
    ENDP

;   
;       Exclusive add (16-bit)
;
;   void schrx_exclusive_add_16(uint16_t* _address, uint16_t _value);
;
schrx_exclusive_add_16  PROC

__schrx_ea16_begin
        LDREXH  R2, [R0]
        ADD     R2, R2, R1
        STREXH  R3, R2, [R0]
        CBZ     R3, __schrx_ea16_ok
        B       __schrx_ea16_begin

__schrx_ea16_ok
        BX      LR
    ENDP

;
;   32-bit Compare and exchange
;   
;   uint32_t schrx_cas_32(uint32_t *_destination, uin32_t _compare ,uint32_t _exchange)
;

schrx_cas_32    PROC

__schrx_c32_begin
        LDREX   R3, [R0]
        EOR     R1, R3
        CBNZ    R1, __schrx_c32_ret
        STREX   R1, R2, [R0]
        CBZ     R1, __schrx_c32_ret
        B       __schrx_c32_begin
__schrx_c32_ret
        MOV     R0, R3
        BX      LR
    ENDP

;
;   8-bit Compare and exchange
;
;   uint32_t schrx_cas_8(uint8_t *_destination, uint8_t _compate, uint8_t _exchange)
;
schrx_cas_8    PROC

__schrx_c8_begin
        LDREXB  R3, [R0]
        EOR     R1, R3
        CBNZ    R1, __schrx_c8_ret
        STREXB  R1, R2, [R0]
        CBZ     R1, __schrx_c8_ret
        B       __schrx_c8_begin
__schrx_c8_ret
        MOV     R0, R3
        BX      LR
    ENDP

;
;       32-bit Exclusive Swap 
;
;   uint32_t schrx_exclusive_swap_32(uint32_t* _destination, uint32_t _swap);
;
schrx_exclusive_swap_32    PROC

__schrx_es32_begin
        LDREX   R2, [R0]
        STREX  R3, R1, [R0]
        CBZ     R3, __schrx_es32_end
        B       __schrx_es32_begin
__schrx_es32_end
        MOV     R0, R2
        BX      LR

    ENDP

;
;       8-bit Exclusive Swap
;
;   uint32_t schrx_exclusive_swap_8(uint8_t* _destination, uint8_t _swap);
;
schrx_exclusive_swap_8      PROC

__schrx_es8_begin
        LDREXB  R2, [R0]
        STREXB  R3, R1, [R0]
        CBZ     R3, __schrx_es8_end
        B       __schrx_es8_begin
__schrx_es8_end
        MOV     R0, R2
        BX      LR
    ENDP


;
;   Scheduler routine entry
;

    ;
    ;   For processor with VFP
    ;
        IF {TARGET_FPU_VFP}
            EXPORT  schrx_schedule_invoke           [WEAK]
            EXPORT  schrx_invoke_fpu
            EXPORT  schrx_invoke_no_fpu
                
schrx_schedule_invoke                               ; dummy tag

        ;
        ;   save context with FP Expension
        ;
schrx_invoke_fpu        PROC
                VPUSH   {S16-S31}                       ; Save FPU Registers
                MRS     R1, PSP                         ; Read user stack
                PUSH    {R1, R4-R11}
                MOV     R2, SP
                PUSH    {R1-R2}                         ; irq context structure
                
                MOV     R1, SP                          ; call shedule procedure
                BL      schrx_schedule_routine
                
                POP     {R0-R2}
                MSR     PSP, R2
                
                POP     {R4-R11}
                VPOP    {S16-S31}

                POP     {PC}
            ENDP
            
        ENDIF

    ;
    ;   Save context with no FP Expansion
    ;
schrx_invoke_no_fpu     PROC

        MRS     R1, PSP                         ; Read user stack
        PUSH    {R1, R4-R11}
        MOV     R2, SP
        PUSH    {R1-R2}                         ; irq context
        
        MOV     R1, SP                          ; call schedule procedure
        BL      schrx_schedule_routine
        
        POP     {R0-R2}
        MSR     PSP, R2
        
        POP     {R4-R11}

        POP     {PC}
    ENDP

    ;
    ;   PendSV Handler
    ;
    ;   void schrx_pendsv_handler(void);
    ;
schrx_pendsv_handler    PROC
IRQ_EXC_STACK_MASK      EQU     0x4

        PUSH    {LR}
        LDR     R0, =schrx_schr_instance
        LDR     R0, [R0]
        CBZ     R0, __pendsv_ret    

        MOV     R1, LR
        AND     R1, #IRQ_EXC_STACK_MASK         ; Check stack type
        CBZ     R1, __pendsv_ret                ; in irq routine, do not switch

        IF {TARGET_FPU_VFP}
            LDR     R1, =schrx_schedule_invoke
            LDR     R1, [R1]
            BX      R1
        ELSE
            B       schrx_invoke_no_fpu
        ENDIF

__pendsv_ret
        POP     {PC}
    ENDP


;
;   launch procedure
;
;   void schrx_core_launch(struct SchedulerX *_scheduler);
;
schrx_core_launch           PROC
CTRL_USER_STACK_MASK        EQU     2

        PUSH    {LR}                        ; Save exit point

        LDR     R1, =schrx_schr_instance
        STR     R0, [R1]
        PUSH    {R0}
        BL      schrx_schedule_suspend
        BL      schrx_scheduler_attach
        POP     {R0}
        
        LDR     R1, =schrx_idle_stack
        LDR     R1, [R1]
        MSR     PSP, R1                     ; switch to user stack
        MRS     R1, CONTROL
        ORR     R1, #CTRL_USER_STACK_MASK
        MSR     CONTROL, R1

        BL      schrx_schedule_resume
        BL      schrx_switch
        B       schrx_do_idle
    ENDP

;
;   check whether the executing-code is running currently in user context.
;
;   uint32_t schrx_is_user_context(void);
;
;@return value:
;   0  -   be not in user context
;   1  -   be in user context
;

schrx_is_user_context   PROC
    
        MRS     R0, CONTROL
        AND     R0, #0x02
        CBZ     R0, __schrx_iuc_ret
        MRS     R0, IPSR
        CBZ     R0, __schrx_iuc_true
        MOV     R0, #0
        BX      LR
__schrx_iuc_true
        MOV     R0, #0x01
__schrx_iuc_ret
        BX      LR
    ENDP

;
;   enter irq critical
;
;   void schrx_irq_critical_enter(void);
;
schrx_irq_critical_enter    PROC
        CPSID   I
        BX      LR
    ENDP

;
;   leave irq critical
;
;   void schrx_irq_critical_exit(void);
;
schrx_irq_critical_exit     PROC
        CPSIE   I
        BX      LR
    ENDP

;
;
;   return to exit point
;
schrx_run_exit_procedure    PROC
                            POP     {LR}                        ; return to SchrX_Run() 
                            BX      LR                          ; stop scheduling
                            ENDP
    END

