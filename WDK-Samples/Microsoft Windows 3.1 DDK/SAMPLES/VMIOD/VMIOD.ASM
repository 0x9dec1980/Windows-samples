PAGE 58,132
;******************************************************************************
TITLE VMIOD - Virtual <Monitor I/O Traffic> Device
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1992
;
;   Title:      VMIOD.ASM - Virtual <Monitor I/O Traffic> Device
;
;   Version:    3.10
;
;   Date:       10-Mar-1992
;
;   Author:     Neil Sandlin
;
;   Description:
;       This VxD demonstrates calling the Install_IO_Handler service.
;       Although most VxDs that use this service are designed to virtualize
;       the I/O port, this VxD will only write to a secondary monitor to 
;       display the activity on the port.
;
;       THE SYSTEM WILL ONLY ALLOW ONE IO HANDLER PER PORT. What this
;       means is that you can't install an I/O handler on a port that
;       is already trapped in the system.
;
;       This VxD only demonstrates I/O port trapping, NOT IRQ handling.
;       See the VMIRQD VxD sample for more information about working
;       with IRQs.
;
;       NOTE: This VxD uses the Debug Monitor support that was added
;       in Windows 3.1. You must have a secondary monitor in your system
;       to see the effects of this VxD. To enable this support, put 
;       DebugMono=TRUE in your system.ini.
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
        INCLUDE VMIOD.Inc
        .LIST


;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VMIOD, 3, 0ah, VMIOD_Control, VMIOD_Dev_ID ,,,


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_LOCKED_DATA_SEG

VMIOD_Port      dd      0
instring        db      "  in  #DX, #AL",0
outstring       db      "  out #AL, #DX",0

FirstMonoLine   equ     2
LastMonoLine    equ     23
CurrentLine     db      ?

BlankPointer db "     ",0
NewLine      db "--->                                                                            ",0

VxD_LOCKED_DATA_ENDS




;******************************************************************************
;                       I N I T I A L I Z A T I O N   
;******************************************************************************

VxD_IDATA_SEG

VMIOD_Ini_String    db  'VMIOD_Port',0

VxD_IDATA_ENDS

VxD_ICODE_SEG


;******************************************************************************
;
;   VMIOD_Device_Init
;
;   DESCRIPTION:
;       This routine installs our I/O port handler.
;
;
;==============================================================================

BeginProc VMIOD_Device_Init

        call    Mon_Init                        ; init secondary monitor
        jc      short diexit

        xor     eax, eax                        ; zero default port
        mov     edi, OFFSET32 VMIOD_Ini_String
        xor     esi, esi                        ; [386enh] section
        VMMCall Get_Profile_Hex_Int             ; get defined port
        mov     [VMIOD_Port], eax               ; save it
        or      eax,eax                         ; anything specified?
        jz      SHORT No_Port           
;
; hook the port to watch
;
        mov     edx, eax                        ; port number
        mov     esi, OFFSET32 VMIOD_Trap        ; address of our handler
        VMMCall Install_IO_Handler

IFDEF DEBUG
        jc      short bad_port

        mov     eax, [VMIOD_Port]
        Trace_Out "VMIOD: Installed on port #AX"
        jmp     short diexit
bad_port:
        mov     eax, [VMIOD_Port]
        Trace_Out "VMIOD: Error installing on port #AX"
        jmp     short diexit
ENDIF

No_Port:
IFDEF DEBUG
        Trace_Out "VMIOD: No VMIOD_Port= specified in SYSTEM.INI"
ENDIF

diexit:

        clc
        ret
EndProc VMIOD_Device_Init

VxD_ICODE_ENDS


;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VMIOD_Control
;
;
;==============================================================================

BeginProc VMIOD_Control

        Control_Dispatch Device_Init, VMIOD_Device_Init
        clc
        ret

EndProc VMIOD_Control

VxD_LOCKED_CODE_ENDS



VxD_CODE_SEG


;******************************************************************************
;
;   VMIOD_Trap
;
;   DESCRIPTION:
;       This is the actual I/O handler for the port. There is no
;       virtualization done, it just monitors what is read and written
;       to the port.
;
;==============================================================================

BeginProc VMIOD_Trap

        Dispatch_Byte_IO Fall_Through, <SHORT VMIOD_Out_Handler>

        in      al,dx                       ; perform real I/O

        push    esi
        mov     esi, OFFSET32 instring
        call    Mon_Out_String              ; display
        pop     esi
        ret

VMIOD_Out_Handler:

        push    esi
        mov     esi, OFFSET32 outstring
        call    Mon_Out_String              ; display
        pop     esi

        out     dx,al                       ; perform real I/O

        ret

EndProc VMIOD_Trap


VxD_CODE_ENDS



;******************************************************************************
;******************************************************************************
;
;               Monitor Display Routines
;
;******************************************************************************

VxD_ICODE_SEG
;******************************************************************************
;
;   Mon_Init
;
;
;==============================================================================

BeginProc Mon_Init


        VMMCall Clear_Mono_Screen
        jz      short mon_init_error

        Mono_Out_At 0, 0, " VMIOD - I/O monitor "
        Mono_Out_At 1, 0, " --------------------------------------------------------------------------- "

        mov     byte ptr [CurrentLine], 2

        clc
        ret

mon_init_error:
        stc
        ret

EndProc Mon_Init


VxD_ICODE_ENDS


VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   Mon_Out_String
;
;
;   ENTRY:
;       ESI - offset of string
;
;
;==============================================================================

BeginProc Mon_Out_String

        pushad

        push    esi                             ;save output string
        push    eax
        push    edx

        movzx   edx, [CurrentLine]
        shl     edx, 8                          ;column zero
        VMMCall Set_Mono_Cur_Pos
        mov     esi, OFFSET32 BlankPointer
        VMMCall Out_Mono_String
 
        movzx   edx, [CurrentLine]              ;update line pointer
        inc     edx
        cmp     edx, LastMonoLine               ;too far?
        jb      short moc_ok
        mov     edx, FirstMonoLine
moc_ok:
        mov     [CurrentLine], dl        
        shl     edx, 8
        VMMCall Set_Mono_Cur_Pos
        
        mov     esi, OFFSET32 NewLine           ;set up new line
        VMMCall Out_Mono_String

        mov     dh, [CurrentLine]
        mov     dl, 5                           ;column 5
        VMMCall Set_Mono_Cur_Pos

        pop     edx
        pop     eax
        pop     esi                             ;write new string
        VMMCall Out_Mono_String

        popad
        ret

EndProc Mon_Out_String

VxD_LOCKED_CODE_ENDS

        END

