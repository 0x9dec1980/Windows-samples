        page    58,132
        title   vpostd

;   (C) Copyright MICROSOFT Corp., 1991
;
;** vpostd.asm
;*
;*  DESCRIPTION:
;*      This is a sample VxD demonstrating how you can PostMessage()
;*      from a VxD.  It requires a .DLL that you Simulate_Far_Call to
;*      that will do the PostMessage() at the right time and priv.
;*
;*  HISTORY:
;*      2/27/91     cjp     wrote it
;*
;** cjp *


        .386p                   ; VxD's are for real processors!



;*****************************  INCLUDES  **********************************

.xlist
        include vmm.inc         ; this be required... unless your crazy.
        include debug.inc       ; optional; enabled if DEBUG defined
        include shell.inc       ; for dlg boxes and the like
        include vpicd.inc       ; if you are playing with interrupts
        include vdd.inc         ; for funky display control/status

        include vpostd.inc      ; this is us
.list



;********************  VIRTUAL DEVICE DECLARATION  *************************

Declare_Virtual_Device VPOSTD, VPOSTD_VERMAJ, VPOSTD_VERMIN, VPOSTD_Control,\
                        VPOSTD_DEV_ID,, VPOSTD_API_Proc, VPOSTD_API_Proc



;***********************  LOCAL LOCKED DATA  *******************************

VxD_LOCKED_DATA_SEG
        align   4

VPOSTD_CB_Offset        dd      0

VxD_LOCKED_DATA_ENDS



;**********************  INITIALIZATION DATA SEG  **************************

VxD_IDATA_SEG

        ;;  no discardable initialization data used in this VxD

VxD_IDATA_ENDS



;************************  NORMAL VXD DATA SEG  ****************************

VxD_DATA_SEG

;;  this info is registered by the POSTHOST.DLL

PostHost_hWnd           dw      0       ; as passed by POSTHOST.DLL
PostHost_Proc_Off32     dd      0       ; POSTHOST.DLL's post message addr
PostHost_Proc_Seg       dw      0


;;  API jump table

VPOSTD_API_Table label DWORD
        dd      offset32        VPOSTD_Get_Version
        dd      offset32        VPOSTD_Register_Callback
        dd      offset32        VPOSTD_Call_PostHost

VPOSTD_Max_API = ($ - VPOSTD_API_Table) / 4 - 1

VxD_DATA_ENDS




;***************  INITIALIZATION CODE SEG: PROTECT MODE  *******************

VxD_ICODE_SEG


;** VPOSTD_Sys_Crit_Init
;*
;*  DESCRIPTION:
;*      Phase 1.  When this function is called, interrupts are not yet
;*      enabled.  Therefore, this phase should accomplish the following
;*      tasks as quickly as possible:
;*
;*              o  Initialization of critical functions necessary
;*                 when interrupts are enabled.
;*
;*              o  Claiming a particular range of V86 pages if
;*                 necessary (such as the video memory for the VDD).
;*
;*              o  Initialization of data needed by the services
;*                 provided by this VxD (FAST stuff).
;*                 
;*              o  During this phase, the System VM Simulate_Int
;*                 and Exec_Int commands must NOT be used (can't).
;*
;*  ENTRY:
;*      EBX     :       VM Handle.
;*
;*  EXIT:
;*      Carry clear if successful.  Set if fail--this will keep this
;*      VxD from loading.
;*
;*  NOTES:
;*
;** cjp *

BeginProc VPOSTD_Sys_Crit_Init

        ;;  This isn't a very important VxD, so we don't do much here.

        clc                             ; no error
        ret

EndProc VPOSTD_Sys_Crit_Init



;** VPOSTD_Device_Init
;*
;*  DESCRIPTION:
;*      Phase 2.  This is where the bulk of your initialization should
;*      be done.  Interrupts are enabled now and the Simulate_Int and 
;*      Exec_Int services are allowed.  During this phase, you can:
;*
;*              o  Allocate your Control Block area and misc. memory.
;*
;*              o  Hook interrupts and I/O ports.
;*
;*              o  Specify instance data.
;*
;*      The System VM's Control Block should be set up with the initial
;*      state of the VxD.  Since the System VM has already been created
;*      calls such as Simulate_Int or Exec_Int are allowed.
;*
;*  ENTRY:
;*      EBX     :       VM Handle.
;*
;*  EXIT:
;*      Carry clear if successful.  Set if fail--this will keep this
;*      VxD from loading.
;*
;*  NOTES:
;*
;** cjp *

BeginProc VPOSTD_Device_Init

        VMMcall _Allocate_Device_CB_Area, <<SIZE VPOSTD_CB_DATA>, 0>
        test    eax, eax
        jnz     short VPOSTD_CB_OK


        ;;  uh-oh... trouble...

        Debug_Out "VPOSTD_Device_Init: _Allocate_Device_CB_Area failed!"
        VMMcall Fatal_Memory_Error


VPOSTD_CB_OK:
        mov     [VPOSTD_CB_Offset], eax         ; gotta keep this around
        
        clc                                     ; no error
        ret

EndProc VPOSTD_Device_Init

VxD_ICODE_ENDS



;**************************  LOCKED CODE SEG  ******************************


VxD_LOCKED_CODE_SEG

;** VPOSTD_Control
;*
;*  DESCRIPTION:
;*
;*  ENTRY:
;*
;*  EXIT:
;*
;*  NOTES:
;*
;** cjp *

BeginProc VPOSTD_Control

        Control_Dispatch Sys_Critical_Init,     VPOSTD_Sys_Crit_Init
        Control_Dispatch Device_Init,           VPOSTD_Device_Init

        clc
        ret

EndProc VPOSTD_Control

VxD_LOCKED_CODE_ENDS



;*************************  PROTECT MODE CODE SEG  *************************

VxD_CODE_SEG

BeginDoc
;** VPOSTD_API_Proc
;*
;*  DESCRIPTION:
;*      This is the exported API procedure that is callable from VM's. 
;*      An application needs only to use INT 2Fh, AX=1684h, BX=device ID
;*      and a call back address is returned.  Then, when the address is
;*      called, eventually it ends up here.
;*
;*
;*  ENTRY:
;*      EBX             :       VM Handle.
;*      EBP             :       Client register structure.
;*      Client_CS:IP    :       Instruction following API call.
;*      Client_AX       :       Function number.
;*
;*  EXIT:
;*      Client:
;*              Carry clear     :  success
;*              Carry set       :  hosed
;*
;*  NOTES:
;*
;** cjp *
EndDoc

BeginProc VPOSTD_API_Proc

        ;; a short word from our sponsor

        movzx   eax, [ebp.Client_AX]            ; function #

        Trace_Out "VPOSTD_API_Proc: VM #EBX Function #EAX"

        cmp     ax, VPOSTD_Max_API              ; a valid service?
        ja      short VPOSTD_API_Failed
        jmp     VPOSTD_API_Table[ eax * 4 ]


VPOSTD_API_Success:

        Trace_Out "VPOSTD_API_Proc: Success!  VM #EBX"

        and     [ebp.Client_EFlags], NOT CF_Mask
        jmp     short VPOSTD_Exit


VPOSTD_API_Failed :

        Trace_Out "VPOSTD_API_Proc: Failed!  VM #EBX"

        mov     [ebp.Client_AX], 0              ; FALSE return value
        or      [ebp.Client_EFlags], CF_Mask

VPOSTD_Exit:
        ret

EndProc VPOSTD_API_Proc


BeginDoc
;** VPOSTD_Get_Version
;*
;*  DESCRIPTION:
;*      This service gets the version number of this VxD.  The major
;*      version number is loaded into AH; the minor version number is
;*      stuffed into AL.
;*
;*  ENTRY:
;*      EBX             :       VM Handle.
;*      EBP             :       Client register structure.
;*      Client_CS:IP    :       Instruction following API call.
;*
;*  EXIT:
;*      Client:
;*              EAX             :  Major and minor version number.
;*              Carry clear     :  Success.
;*
;*  NOTES:
;*
;** cjp *
EndDoc

BeginProc VPOSTD_Get_Version

        mov     [ebp.Client_AX], (VPOSTD_VERMAJ shl 8) + VPOSTD_VERMIN
        jmp     VPOSTD_API_Success

EndProc VPOSTD_Get_Version


BeginDoc
;** VPOSTD_Register_Callback
;*
;*  DESCRIPTION:
;*      This service sets the callback address and window handle for
;*      the POSTHOST.DLL.  Calling this procedure with ES:DI set to
;*      NULL will de-register the callback, thus voiding the POSTHOST.DLL
;*      from getting called.  This should be done when the POSTHOST is
;*      exiting or no longer requires posts.
;*
;*  ENTRY:
;*      EBX             :       VM Handle.
;*      EBP             :       Client register structure.
;*      Client_CS:IP    :       Instruction following API call.
;*      Client_BX       :       Window handle to post to.
;*      Client_ES:DI    :       PostHost proc address.
;*
;*  EXIT:
;*      Client:
;*              Carry clear     :  Success; always does.
;*
;*  NOTES:
;*
;** cjp *
EndDoc

BeginProc VPOSTD_Register_Callback

        ;;  just stuff ES:DI into our data seg--if null, de-register

        movzx   eax, [ebp.Client_DI]            ; grab offset
        mov     [PostHost_Proc_Off32], eax      ; stuff it
        mov     ax, [ebp.Client_ES]             ; grab seg
        mov     [PostHost_Proc_Seg], ax         ; stuff it

        mov     ax, [ebp.Client_BX]             ; grab window handle
        mov     [PostHost_hWnd], ax             ; stuff it

        mov     [ebp.Client_AX], 1              ; successful (TRUE) return
        jmp     short VPOSTD_API_Success

EndProc VPOSTD_Register_Callback


BeginDoc
;** VPOSTD_Call_PostHost
;*
;*  DESCRIPTION:
;*      This service is used to call the POSTHOST.DLL to post a message
;*      to the hWnd that was registered with VPOSTD_Register_Callback.
;*      If the callback address is NULL, then this service will fail--
;*      no sanity checks are made on the hWnd.
;*
;*  ENTRY:
;*      EBX             :       VM Handle.
;*      EBP             :       Client register structure.
;*      Client_CS:IP    :       Instruction following API call.
;*      Client_CX:DX    :       DWORD data to be passed to PostHost.
;*
;*  EXIT:
;*      Client:
;*              Carry clear     :  Success.
;*              Carry set       :  Hosed.  Callback address is NULL.
;*
;*  NOTES:
;*
;** cjp *
EndDoc

BeginProc VPOSTD_Call_PostHost

        Trace_Out "VPOSTD_Call_PostHost: Enter  VM #EBX"

        movzx   eax, [PostHost_Proc_Seg]        ; is callback addr NULL?
        or      eax, [PostHost_Proc_Off32]
        jz      VPOSTD_API_Failed               ; no go--fail!

        mov     esi, ebx                        ; vm block of caller
        add     esi, [VPOSTD_CB_Offset]         ; point to that VM's area

        mov     ax, [ebp.Client_CX]
        mov     [esi.VPOSTD_CB_Client_CX], ax   ; save the CX info
        mov     ax, [ebp.Client_DX]
        mov     [esi.VPOSTD_CB_Client_DX], ax   ; save the DX info

        VMMcall Test_Sys_VM_Handle              ; PostHost is in sys VM
        jnz     short VPOSTD_Schedule_Call      ; we gotta be in sys VM...

        VMMcall Get_Crit_Section_Status         ; need to wait for !crit?
        jc      short VPOSTD_Schedule_Call      ; bummer...


        ;;  we know current VM is sys VM and critical section unowned
        ;;  so do the callback when ints are enabled--which they may be...

        Trace_Out "VPOSTD_Call_PostHost: IN SYS VM!!!  VM #EBX"

        mov     edx, ebx                        ; ref. data == sys VM handle
        mov     esi, offset32 VPOSTD_Call_PostHost_Now
        VMMcall Call_When_VM_Ints_Enabled

        jmp     short VPOSTD_Call_PostHost_Exit


VPOSTD_Schedule_Call:

        ;;  This Trace_Out is really not the greatest thing to be doing
        ;;  here... we could be in an ISR and we're just wasting time!
        ;;  But it is nice for watching the flow of this VxD...

        Trace_Out "VPOSTD_Call_PostHost: SCHEDULE SYS VM!  VM #EBX"


        ;;  Ok.  We need to get the sys VM called sometime in the near
        ;;  future when everything stablizes.  This callback is not that
        ;;  important, so we will wait until there is no critical section
        ;;  and interrupts have been enabled.  We actually have to so we
        ;;  don't re-enter the PostHost... Call_Priority_VM_Event is your
        ;;  friend!

        mov     edx, ebx                        ; ref. data == curr VM handle
        VMMcall Get_Sys_VM_Handle               ; wanna go to sys VM
        mov     eax, Low_Pri_Device_Boost       ; low priority...
        mov     ecx, PEF_Wait_For_STI or PEF_Wait_Not_Crit
        mov     esi, offset32 VPOSTD_Call_PostHost_Now
        VMMcall Call_Priority_VM_Event


VPOSTD_Call_PostHost_Exit:

        Trace_Out "VPOSTD_Call_PostHost: Exit  VM #EBX"

        mov     [ebp.Client_AX], 1              ; successful (TRUE) return
        jmp     VPOSTD_API_Success              ; cool!

EndProc VPOSTD_Call_PostHost


BeginDoc
;** VPOSTD_Call_PostHost_Now
;*
;*  DESCRIPTION:
;*      This routine is a callback for calling the POSTHOST.DLL's callback!
;*      The .DLL's callback will receive the hWnd that was previously
;*      registered as the only argument.  This is really not necessary,
;*      but it demonstrates how to pass info to a callback on the stack.
;*
;*  ENTRY:
;*      EBX     :       *System* VM Handle.
;*      EDX     :       Reference data passed to this service (VM handle).
;*      EBP     :       Client register structure.
;*
;*  EXIT:
;*      No change to client regs.  We can trash EAX, EBX, ECX, EDX, ESI,
;*      EDI, and Flags.
;*
;*  NOTES:
;*
;** cjp *
EndDoc

BeginProc VPOSTD_Call_PostHost_Now

        Push_Client_State                       ; gotta save this!

        VMMcall Begin_Nest_Exec                 ; prepare to call VM


        ;;  The callback in the .DLL is prototyped as follows:
        ;;  void FAR PASCAL phCallBack( HWND hWnd, WORD wVMID, DWORD lParam )

        mov     ax, [PostHost_hWnd]             ; push hWnd for PostHost
        VMMcall Simulate_Push
        mov     eax, [edx.CB_VMID]              ; push VM ID of caller
        VMMcall Simulate_Push


        ;;  we need to grab the CX:DX that the user passed from the
        ;;  correct VM--handle was passed in edx

        mov     esi, edx                        ; vm block of caller
        add     esi, [VPOSTD_CB_Offset]         ; point to that VM's area


        ;;  dealing with 16 bit PM app so Simulate_Push will do 16 bits

        mov     ax, [esi.VPOSTD_CB_Client_CX]   ; get the CX info
        VMMcall Simulate_Push                   ; high word first
        mov     ax, [esi.VPOSTD_CB_Client_DX]   ; get the DX info
        VMMcall Simulate_Push                   ; low word second

        mov     cx, [PostHost_Proc_Seg]         ; CX:EDX -> PostHost...
        mov     edx, [PostHost_Proc_Off32]

        VMMcall Simulate_Far_Call               ; set stuff up for call
        VMMcall Resume_Exec                     ; finally! call it (immediate)

        VMMcall End_Nest_Exec                   ; whew! done with that

        Pop_Client_State                        ; restore client stuff now

        ret


EndProc VPOSTD_Call_PostHost_Now


VxD_CODE_ENDS


        end

;** EOF: vpostd.asm **
