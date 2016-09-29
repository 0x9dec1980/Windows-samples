PAGE 58,132
;******************************************************************************
TITLE VKXD - Virtual Keyboard Device
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1990-1991
;
;   Title:      VKXD.ASM - Virtual Keyboard Extension Device
;
;   Version:    3.00
;
;   Date:       8-Oct-1990 
;
;   Author:     Neil Sandlin
;
;   This is a tiny little VxD to let you hit a function key and get the
;   same effect as hitting "alt-enter"
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV                 DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   15-Apr-1991 bjm Added Cancel_Hot_Key_State in case of scanning normal
;                   keys. ALT-ENTER was a special case because it is also
;                   a hot key. Without Cancel_Hot_Key_State, normal keys
;                   would not be forced into a VM.
;
;==============================================================================

        .386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

        .XLIST
        INCLUDE VMM.Inc
        INCLUDE Debug.Inc
        INCLUDE VKD.Inc
        .LIST

Scan_F9 EQU     43h                     ; scan code for the "F9" key

;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VKXD, 3, 0, VKXD_Control, Undefined_Device_ID ,,,


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_LOCKED_DATA_SEG

                                        ; Scan codes
VKXD_Buffer     db      038h            ; alt (down)
                db      01ch            ; enter (down)
                db      09ch            ; enter (up)
                db      0b8h            ; alt (up)
VKXD_Buffer_Len equ     $-VKXD_Buffer

VxD_LOCKED_DATA_ENDS




;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   VKXD_Device_Init
;
;   DESCRIPTION:
;
;       This routine sets up the hot key call-back.
;
;
;==============================================================================

BeginProc VKXD_Device_Init
IFDEF DEBUG
    Trace_Out "VKXD installed"
ENDIF
        
    mov    ax, Scan_F9                     ; Hot key: scan code for F9
    mov    ebx,0                           ; shift state: don't care
   	mov    esi, OFFSET32 VKXD_HK_Handler   ; address of our handler
  	mov    cl, CallOnPress                 ; call when hot key is released
  	mov    edx, 1                          ; reference data - arbitrary
	VxDCall VKD_Define_Hot_Key              ; set up hot key

    clc
    ret
EndProc VKXD_Device_Init

VxD_ICODE_ENDS



;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VKXD_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation. 
;
;
;==============================================================================

BeginProc VKXD_Control

    Control_Dispatch Device_Init, VKXD_Device_Init
    clc
    ret

EndProc VKXD_Control

VxD_LOCKED_CODE_ENDS



VxD_CODE_SEG

;******************************************************************************
;
;   VKXD_HK_Handler
;
;   DESCRIPTION:
;
;       This routine is called when the hot key is pressed. Its only 
;       function is to call VKD_Force_Keys to simulate an alt-enter.
;
;   ENTRY:
;
;		    AL = scan code of key
;		    AH = 0, if key just pressed 	    (Hot_Key_Pressed)
;		       = 1, if key just released	    (Hot_Key_Released)
;		       = 2, if key is an auto-repeat press  (Hot_Key_Repeated)
;		       = 3, hot key state ended 	    (Hot_Key_Completed)
;		    EBX is hot key handle
;		    ECX = global shift state
;		    EDX is reference data
;		    EDI = elapsed time for delayed notification  (milliseconds)
;			    (normally 0, but if PriorityNotify is specified
;			     then this value could be larger)
;
;******************************************************************************
BeginProc VKXD_HK_Handler

    VxDCall VKD_Cancel_Hot_Key_State        ; turn off hotkey mode

    mov     ecx,VKXD_Buffer_Len
    lea     esi,VKXD_Buffer                 ; scan codes
    VxDCall VKD_Force_Keys                  ; send to VM

IFDEF DEBUG
    jnc     SHORT kbd_ok
    Trace_Out "VKXD: VKD force key error"
kbd_ok: 
ENDIF

    clc
    ret

EndProc VKXD_HK_Handler

VxD_CODE_ENDS


    END
