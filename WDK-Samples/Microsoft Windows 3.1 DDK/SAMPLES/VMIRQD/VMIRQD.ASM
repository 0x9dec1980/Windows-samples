PAGE 58,132
;******************************************************************************
TITLE VMIRQD - Virtual <Monitor I/O Traffic> Device
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1992
;
;   Title:      VMIRQD.ASM - Virtual <Monitor IRQ traffic> Device
;
;   Version:    3.10
;
;   Date:       10-Mar-1992
;
;   Author:     Neil Sandlin
;
;   Description:
;       This VxD demonstrates calling the VPICD_Virtualize_IRQ service.
;       Although most VxDs that use this service are designed to only
;       arbitrate the handling of the interrupt, this VxD will write
;       to a secondary monitor to display the activity on the IRQ.
;
;       THE SYSTEM WILL ONLY ALLOW ONE IRQ HANDLER PER IRQ. What this
;       means is that you can't virtualize an IRQ that is already 
;       virtualized in the system.
;
;       This VxD only demonstrates IRQ virtualization, NOT I/O port 
;       trapping. See the VMIOD VxD sample for more information about 
;       working with I/O port trapping.
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
        INCLUDE VPICD.Inc
        INCLUDE VMIRQD.Inc
        .LIST


;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VMIRQD, 3, 0ah, VMIRQD_Control, VMIRQD_Dev_ID ,,,


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_LOCKED_DATA_SEG

VMIRQD_IRQ_Handle dd      ?
VMIRQD_IRQ       db      ?


FirstMonoLine   equ     2
LastMonoLine    equ     23
CurrentLine     db      ?

vstring db      "Virtual interrupt reflected",0
rstring db      "IRET executed",0
mstring db      "Mask change",0
istring db      "Physical Interrupt",0
estring db      "EOI issued"

BlankPointer db "     ",0
NewLine      db "--->                                                                            ",0

VxD_LOCKED_DATA_ENDS




;******************************************************************************
;                       I N I T I A L I Z A T I O N   
;******************************************************************************

VxD_IDATA_SEG

VMIRQD_Ini_String  db  'VMIRQD_IRQ',0


VMIRQD_IRQ_Desc VPICD_IRQ_Descriptor <?,VPICD_Opt_Read_Hw_IRR,   \
                                    OFFSET32 VMIRQD_Hw_INT,      \
                                    OFFSET32 VMIRQD_Virt_INT,    \
                                    OFFSET32 VMIRQD_EOI,         \
                                    OFFSET32 VMIRQD_Mask_Change, \
                                    OFFSET32 VMIRQD_IRET>

VxD_IDATA_ENDS

VxD_ICODE_SEG


;******************************************************************************
;
;   VMIRQD_Device_Init
;
;   DESCRIPTION:
;       This routine will virtualize the IRQ specified in the SYSTEM.INI
;       string VMIRQD_IRQ=x.
;
;==============================================================================

BeginProc VMIRQD_Device_Init

        call    Mon_Init                        ; init secondary monitor
        jc      short diexit

        mov     eax, -1                         ; default IRQ 
        mov     edi, OFFSET32 VMIRQD_Ini_String
        xor     esi, esi                        ; [386enh] section
        VMMCall Get_Profile_Decimal_Int         ; get defined IRQ 
        mov     BYTE PTR [VMIRQD_IRQ], al       ; save it
        cmp     eax, -1                         ; anything specified?
        jz      SHORT No_Irq

        mov     edi, OFFSET32 VMIRQD_IRQ_Desc   ; point to vpicd struct
        mov     [edi.VID_IRQ_Number], ax        ; save it here
        VxDCall VPICD_Virtualize_IRQ            ; set up callbacks


IFDEF DEBUG
        jc      short bad_irq

        mov     al, [VMIRQD_IRQ]
        Trace_Out "VMIRQD: Installed on IRQ #AL"
        jmp     short diexit
bad_irq:
        mov     al, [VMIRQD_IRQ]
        Trace_Out "VMIRQD: Error installing on IRQ #AL"
        jmp     short diexit
ENDIF


No_Irq:
IFDEF DEBUG
        Trace_Out "VMIRQD: No VMIRQD_IRQ= specified in SYSTEM.INI"
ENDIF

diexit:
        clc
        ret
EndProc VMIRQD_Device_Init


VxD_ICODE_ENDS



;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VMIRQD_Control
;
;
;==============================================================================

BeginProc VMIRQD_Control

        Control_Dispatch Device_Init, VMIRQD_Device_Init
        clc
        ret

EndProc VMIRQD_Control


;******************************************************************************
;
;   VMIRQD_Virt_Int
;
;   DESCRIPTION:
;       This routine is called by VPICD when a virtual interrupt is
;       reflected.
;
;==============================================================================

BeginProc VMIRQD_Virt_Int, High_Freq

        push    esi
        mov     esi, OFFSET32 vstring
        call    Mon_Out_String
        pop     esi

        clc
        ret

EndProc VMIRQD_Virt_Int

;******************************************************************************
;
;   VMIRQD_IRET
;
;   DESCRIPTION:
;       This routine is called by VPICD when the virtual machine IRETs
;       at the end of an interrupt.
;
;==============================================================================

BeginProc VMIRQD_IRET, High_Freq

        push    esi
        mov     esi, OFFSET32 rstring
        call    Mon_Out_String
        pop     esi

        clc
        ret

EndProc VMIRQD_IRET


;******************************************************************************
;
;   VMIRQD_Mask_Change
;
;   DESCRIPTION:
;       This routine is called by VPICD when the virtual machine changes
;       the state of the interrupt enabled flag in the mask register.
;
;==============================================================================

BeginProc VMIRQD_Mask_Change, High_Freq

        push    esi
        mov     esi, OFFSET32 mstring
        call    Mon_Out_String
        pop     esi

        clc
        ret

EndProc VMIRQD_Mask_Change


;******************************************************************************
;          H A R D W A R E   I N T E R R U P T   R O U T I N E S
;******************************************************************************

;******************************************************************************
;
;   VMIRQD_Hw_Int
;
;   DESCRIPTION:
;       This routine is called at the time of the actual hardware interrupt.
;
;
;==============================================================================

BeginProc VMIRQD_Hw_Int, High_Freq

        push    esi
        mov     esi, OFFSET32 istring
        call    Mon_Out_String
        pop     esi

        VxDjmp  VPICD_Set_Int_Request

EndProc VMIRQD_Hw_Int


;******************************************************************************
;
;   VMIRQD_EOI
;
;   DESCRIPTION:
;       This routine is called by VPICD when an EOI is performed.
;
;
;==============================================================================

BeginProc VMIRQD_EOI, High_Freq

        push    esi
        mov     esi, OFFSET32 estring
        call    Mon_Out_String
        pop     esi

        VxDCall VPICD_Phys_EOI
        VxDjmp  VPICD_Clear_Int_Request

EndProc VMIRQD_EOI

VxD_LOCKED_CODE_ENDS


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

        Mono_Out_At 0, 0, " VMIRQD - IRQ monitor "
        Mono_Out_At 1, 0, " --------------------------------------------------------------------------- "

        mov     byte ptr [CurrentLine], 2

        clc
        ret

mon_init_error:
        Trace_Out "No mono display: VMIRQD"
        stc
        ret


EndProc Mon_Init


VxD_ICODE_ENDS


VxD_LOCKED_CODE_SEG


;******************************************************************************
;
;   Mon_Out_String
;
;   DESCRIPTION:
;
;   ENTRY:
;       ESI - offset of string
;
;
;==============================================================================

BeginProc Mon_Out_String

        pushad

        push    esi                             ;save output string
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

        pop     esi                             ;write new string
        VMMCall Out_Mono_String

        popad
        ret

EndProc Mon_Out_String

VxD_LOCKED_CODE_ENDS

        END

