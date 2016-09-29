PAGE 58,132
;******************************************************************************
TITLE VITD - Virtual Interval Timer
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1991
;
;   Title:      VITD.ASM - Virtual Interval Timer
;
;   Version:    3.00
;
;   Date:       10-Jan-1991 
;
;   Author:     Neil Sandlin
;
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV                 DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;
;==============================================================================

        .386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

        .XLIST
        INCLUDE VMM.Inc
        INCLUDE Debug.Inc
        INCLUDE VTD.Inc
        INCLUDE ITIMER.Inc
        .LIST

;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VITD, 3, 0, VITD_Control, Undefined_Device_ID ,,,


;******************************************************************************
;                         I N I T   D A T A
;******************************************************************************


VXD_IDATA_SEG

Begin_VxD_IO_Table VITD_IO_Table
        VxD_IO  ITIMER_Count, VITD_Count
        VxD_IO  ITIMER_State, VITD_State
End_VxD_IO_Table VITD_IO_Table


VXD_IDATA_ENDS


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************


VITD_CB_DATA STRUC

Timeout_Handle  dd      ?
Timeout_Start   dd      ?
Initial_Count   dd      ?
Current_Count   dd      ?
Access_byte     db      ?
Timer_State     db      ?

VITD_CB_DATA ENDS


VxD_LOCKED_DATA_SEG

VITD_CB_Offset  dd   0
VITD_VMM_Min    dd   ?

VxD_LOCKED_DATA_ENDS




;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   VITD_Sys_Crit_Init
;
;   DESCRIPTION:
;
;       This routine allocates space in the VM control block area for
;       VM specific variables.
;   
;   ENTRY:
;
;       EBX = Current VM Handle
;
;
;==============================================================================

BeginProc VITD_Sys_Crit_Init

        push    ebx
        VMMCall _Allocate_Device_CB_Area, <<SIZE VITD_CB_DATA>, 0>
        test    eax, eax
        jnz     SHORT VITD_CB_OK
IFDEF DEBUG
        Debug_Out "VITD: Allocate_Device_CB_Area failed"
ENDIF
        VMMCall Fatal_Memory_Error

VITD_CB_OK:
        mov     [VITD_CB_Offset], eax
        pop     ebx
        
        clc
        ret


EndProc VITD_Sys_Crit_Init


;******************************************************************************
;
;   VITD_Device_Init
;
;   DESCRIPTION:
;
;       This routine installs the I/O handlers for the virtual timer
;       hardware. 
;
;   ENTRY:
;
;       EBX = Current VM Handle
;
;==============================================================================

BeginProc VITD_Device_Init

        VxDCall VTD_Get_Version
        VxDCall VTD_Get_Interrupt_Period

IFDEF DEBUG
        Trace_Out "VITD installed"
        Trace_Out "Current interrupt rate=#EAX msecs. Min=#EBX, Max=#ECX"
ENDIF

        mov     VITD_VMM_Min, ebx            ; save minimum interval
        mov     edi, OFFSET32 VITD_IO_Table
        VMMCall Install_Mult_IO_Handlers
        jnc     SHORT DI_ok1
        Trace_Out "VITD ERROR: installed only #ECX I/O handlers"
DI_ok1:

        clc
        ret

EndProc VITD_Device_Init

VxD_ICODE_ENDS


VxD_CODE_SEG

;******************************************************************************
;
;   VITD_Create_VM
;
;   DESCRIPTION:
;
;       This routine initializes the virtual timer registers.
;
;   ENTRY:
;
;       EBX = Current VM Handle
;
;==============================================================================

BeginProc VITD_Create_VM

        mov     edi, ebx                        ; vm block
        add     edi, [VITD_CB_Offset]        ; point to our area
        mov     [edi.Access_Byte], 0            ; initialize
        mov     [edi.Timer_State], ITIMER_STOPPED       ; initial state

        clc
        ret

EndProc VITD_Create_VM


;******************************************************************************
;
;   VITD_Destroy_VM
;
;   DESCRIPTION:
;
;       This routine has to cancel potential events on a destroyed VM.
;
;   ENTRY:
;
;       EBX = Current VM Handle
;
;==============================================================================

BeginProc VITD_Destroy_VM

        mov     edi, ebx                    ; vm block
        add     edi, [VITD_CB_Offset]       ; point to our area
        call    VITD_Cancel          ; cancel pending event, if any

vdm_exit:
        clc
        ret

EndProc VITD_Destroy_VM

VxD_CODE_ENDS


;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VITD_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation.
;
;   ENTRY:
;
;       EBX = Current VM Handle
;
;==============================================================================

BeginProc VITD_Control

        Control_Dispatch Sys_Critical_Init, VITD_Sys_Crit_Init
        Control_Dispatch Device_Init, VITD_Device_Init
        Control_Dispatch Destroy_VM, VITD_Destroy_VM
        Control_Dispatch Create_VM, VITD_Create_VM
        clc
        ret

EndProc VITD_Control

VxD_LOCKED_CODE_ENDS



VxD_CODE_SEG

;******************************************************************************
;
;   VITD_Count
;
;   DESCRIPTION:
;
;       This routine handles byte I/O to the virtual count register.
;       The basic algorithm is as follows: 
;       
;       if I/O is a read  ( IN al,countport )
;          get access byte
;          if Timer is active and reading first byte
;               decrement count by elapsed time
;          return Current_Count[access_byte]
;
;       else if I/O is a write ( OUT countport,al )
;          get access byte
;          if Timer NOT active
;               save Count[access_byte]
;
;   ENTRY:
;
;       EBX = Current VM Handle
;       ECX = Type of I/O
;       EDX = Port number
;       EBP-> Client register structure
;
;******************************************************************************
   
   
BeginProc VITD_Count

        mov     edi, ebx                        ; vm block
        add     edi, [VITD_CB_Offset]        ; point to our area

        Dispatch_Byte_IO Fall_Through, <SHORT vitc_Out>

;---------------------------------------------------------------------
;  Handle Read Count Port
;---------------------------------------------------------------------
        call    Update_Access_Byte              ; load ECX with index

        or      ecx, ecx                        ; first byte?
        jnz     SHORT vitc_no_adjust
        test    [edi.Timer_State], ITIMER_RUNNING       ; timer pending?
        jz      SHORT vitc_no_adjust            ; no, don't update

        VMMCall Get_VM_Exec_Time                ; get current time
        sub     eax, [edi.Timeout_Start]        ; now have elapsed time
        mov     ebx, [edi.Initial_Count]        ; get original count
        sub     ebx, eax                        ; now have new count
        mov     [edi.Current_Count], ebx        ; save it for output

vitc_no_adjust:
        mov     eax, [edi.Current_Count]        ; get entire count
        shl     ecx, 3                          ; multipy index by 8
        shr     eax, cl                         ; now have byte in al
        ret

;---------------------------------------------------------------------
;  Handle Write Count Port
;---------------------------------------------------------------------
vitc_Out:
        call    Update_Access_Byte                      ; load ECX with index
        test    [edi.Timer_State], ITIMER_RUNNING       ; timer pending?
        jnz     SHORT vitc_ro                           ; yes, count is R/O
        mov     BYTE PTR [edi.Initial_Count+ecx], al    ; save count
        mov     BYTE PTR [edi.Current_Count+ecx], al    ; save count
vitc_ro:
        ret


EndProc VITD_Count


;******************************************************************************
;
;   Update_Access_Byte
;
;   DESCRIPTION:
;
;       When a VM does I/O to the virtual count register, it has to
;       do it a byte at a time. This routine updates the byte index 
;       into the 4 byte register, so that each byte I/O accesses the
;       next byte. The register is programmed starting from the lowest
;       significant byte to the highest.
;       
;
;   ENTRY:
;
;
;******************************************************************************
   
BeginProc Update_Access_Byte
        movzx   ecx, [edi.Access_Byte]          ; get byte to retrieve
        cmp     ecx, 3                          ; will increment wrap?
        jb      SHORT vitc_go1                  ; no
        mov     [edi.Access_Byte], 0            ; initialize
        jmp     SHORT vitc_go2                  ; no
vitc_go1:
        inc     [edi.Access_Byte]               ; for next time
vitc_go2:
        ret

EndProc Update_Access_Byte



;******************************************************************************
;
;   VITD_State
;
;   DESCRIPTION:
;
;       This routine handles byte I/O to the virtual state register.
;       The basic algorithm is as follows: 
;       
;       if I/O is a read  ( IN al,stateport )
;          return State
;
;       else if I/O is a write ( OUT stateport,al )
;          if Timer_Start requested
;               if Timer NOT running
;                   save new state
;                   if Enhanced requested
;                       Start enhanced VMM state
;                   Set up VM Timeout
;                   save handle and start time
;
;          else /* Timer Stop requested */
;               if Timer running
;                   Cancel previous timer request
;               save new state
;
;
;   ENTRY:
;
;       EBX = Current VM Handle
;       ECX = Type of I/O
;       EDX = Port number
;       EBP-> Client register structure
;
;******************************************************************************
   
   
BeginProc VITD_State

        mov     edi, ebx                        ; vm block
        add     edi, [VITD_CB_Offset]        ; point to our area


        Dispatch_Byte_IO Fall_Through, <SHORT vits_Out>
;---------------------------------------------------------------------
;  Handle Read State port
;---------------------------------------------------------------------
        movzx   eax, [edi.Timer_State]        ; get state
        ret

vits_Out:
;---------------------------------------------------------------------
;  Handle Write State port
;---------------------------------------------------------------------
        test    eax, ITIMER_RUNNING     ; Start timer request?
        jz      SHORT vits_stop_timer   ; no
        test    [edi.Timer_State], ITIMER_RUNNING   ; timer pending?
        jnz     SHORT vits_exit         ; yes, state is R/O

        mov     [edi.Timer_State], al   ; save state

        test    eax, ITIMER_ENHANCED    ; going for accuracy?
        jz      SHORT vits_noenh        ; no
        mov     eax, VITD_VMM_Min    ; get minimum
        VxDCall VTD_Begin_Min_Int_Period  ; ok, here we go

vits_noenh:
        mov     eax, [edi.Initial_Count]        ; get msecs to wait
        mov     esi, OFFSET32 VITD_Timeout_Proc
        xor     edx, edx                        ; ref data
        VMMCall Set_VM_Time_Out         ; start timer running        
        or      esi, esi                ; was timeout not scheduled?
        jz      SHORT vits_abort        ; hmmm...
        mov     [edi.Timeout_Handle], esi ; save handle
        
        VMMCall Get_VM_Exec_Time        ; remember the start
        mov     [edi.Timeout_Start], eax  ; for later

IFDEF DEBUG
        VxDCall VTD_Get_Interrupt_Period
        Trace_Out "VITD Start: VM=#EBX, rate=#EAX msecs"
ENDIF

vits_exit:
        ret

vits_abort:
        mov     [edi.Timer_State], ITIMER_ERROR  ; woops...
        ret

vits_stop_timer:
        test    [edi.Timer_State], ITIMER_RUNNING   ; timer pending?
        jz      SHORT vits_svst         ; no event to cancel
        call    VITD_Cancel             ; cancel pending event, if any
vits_svst:
        mov     [edi.Timer_State], al   ; save new state
        ret


EndProc VITD_State



;******************************************************************************
;
;   VITD_Cancel
;
;   DESCRIPTION:
;
;       This routine will cancel a previous Timeout request made, if any.
;       It is called if the VM stops the timer manually, or if the VM
;       is destroyed.
;
;   ENTRY:
;
;       EBX = Current VM Handle
;       EDI-> VITD CB area
;
;******************************************************************************
   
   
BeginProc VITD_Cancel

        test    [edi.Timer_State], ITIMER_RUNNING       ; timer pending?
        jz      SHORT vcnl_exit                         ; no
        mov     esi, [edi.Timeout_Handle]               ; get handle
        VMMCall Cancel_Time_Out                         ; cancel it
        test    [edi.Timer_State], ITIMER_ENHANCED      ; going for accuracy?
        jz      SHORT vcnl_exit                         ; no
        mov     eax, VITD_VMM_Min                    ; get minimum
        VxDCall VTD_End_Min_Int_Period                  ; cancel min int
        
vcnl_exit:

IFDEF DEBUG
        VxDCall VTD_Get_Interrupt_Period
        Trace_Out "VITD Cancel: VM=#EBX, rate=#EAX msecs"
ENDIF
        ret


EndProc VITD_Cancel


;******************************************************************************
;
;   VITD_Timeout_Proc
;
;   DESCRIPTION:
;
;       This routine is called by VMM when the requested time has
;       elapsed in the virtual machine. The routine cancels the enhanced
;       interval period, if it was in effect, updates the count, and
;       queues an event to call VITD_Reflect_Int. Most likely, the
;       Reflect_Int routine will be called immediately, because we
;       are already in the VM we want. But, to be safe, the routine
;       will not be called until the VM has enabled for interrupts
;       and is not in a critical section.
;
;   ENTRY:
;
;       EBX = current VM (VM time-out was scheduled for)
;       ECX = Number of EXTRA milleseconds that have elapsed
;       EDX = Reference Data (0)
;       EBP-> Client register structure
;
;******************************************************************************
   
   
BeginProc VITD_Timeout_Proc


        mov     edi, ebx                        ; vm block
        add     edi, [VITD_CB_Offset]        ; point to our area

        test    [edi.Timer_State], ITIMER_ENHANCED      ; going for accuracy?
        jz      SHORT vtmo_noenh                        ; no
        mov     eax, VITD_VMM_Min                    ; get minimum
        VxDCall VTD_End_Min_Int_Period                  ; cancel min int

vtmo_noenh:
        xor     eax, eax                        ; clear
        sub     eax, ecx                        ; now has overrun count
        mov     [edi.Current_Count], eax        ; save here

        test    [edi.Timer_State], ITIMER_NO_RUPT       ; test to send rupt? 
        mov     [edi.Timer_State], ITIMER_STOPPED       ; and clear state
        jnz     SHORT vtmo_norupt                       ; no rupt needed

        xor     eax, eax                        ; no boost
        mov     ecx, PEF_Wait_For_STI or PEF_Wait_Not_Crit  ; better wait
        mov     edx, edi                        ; pointer to CB area
        mov     esi, OFFSET32 VITD_Reflect_Int
        VMMCall Call_Priority_VM_Event          ; do it

vtmo_norupt:
IFDEF DEBUG
        VxDCall VTD_Get_Interrupt_Period
        Trace_Out "VITD Stop: VM=#EBX, rate=#EAX msecs"
ENDIF
        ret

EndProc VITD_Timeout_Proc



;******************************************************************************
;
;   VITD_Reflect_Int
;
;   DESCRIPTION:
;
;       This routine is called to simulate an interval timer interrupt
;       into the current VM.
;
;   ENTRY:
;
;       EBX = current VM (VM time-out was scheduled for)
;       EDX-> VITD CB area 
;       EBP-> Client register structure
;
;******************************************************************************
   
   
BeginProc VITD_Reflect_Int


        mov     edi, edx                        ; our CB area

        VMMCall Begin_Nest_Exec                 ; ready to call VM
        mov     eax, ITIMER_VECTOR              ; get interrupt vector
        VMMCall Exec_Int                        ; reflect
        VMMCall End_Nest_Exec                   ; that's it
        
        ret

EndProc VITD_Reflect_Int



VxD_CODE_ENDS



        END

