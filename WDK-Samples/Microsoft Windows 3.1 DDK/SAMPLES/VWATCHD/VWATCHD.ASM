Title VwatchD.ASM
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1991
;
;By Keith Jin, Microsoft PSS, July 16, 1991
;
;	This code shows the basic structure of a VxD. It does nothing except
;	output debug messages to the debug terminal. Please note, the Trace_Out
;	and Debug_Out macros are only assemblyed when the 'DEBUG' switch is
;	turned on.
;
;==============================================================================

	.386p

	.xlist
	include vmm.inc
	include debug.inc
	include VwatchD.inc
	.list

VwatchD_CB_STRUCT STRUC
VM_Birth_Time	dd	?
VwatchD_CB_STRUCT ENDS

;******************************************************************************
; declare virtual device
;
;	The Declare_Virtual_Device macro creates a structure of data called
;	Device Data Block (DDB). The name is this data block is <VxD name>_DDB.
;	For this VxD, it will be VwatchD_DDB. This name must be exported in the
;	.def file. (See VwatchD.DEF). This data structure is the only place that
;	VMM will find various information about the VxD. See Virtual Device
;	Adaptation Guide for more information. If this VxD exports services,
;	you don't need to include the service table name in this macro. The
;	service table just needs to be declared using the same VxD name. See
;	VwatchD.INC.
;
;==============================================================================

Declare_Virtual_Device VwatchD, 1, 0, VwatchD_Control, VwatchD_DEVICE_ID, \
		       Undefined_Init_Order, VwatchD_V86_API,VwatchD_PM_API

;******************************************************************************
; data
;==============================================================================
VXD_DATA_SEG
	public	VwatchD_CB_Offset
VwatchD_CB_Offset 	dd	0	;

VXD_DATA_ENDS

;***********Real Mode Initialization ****************************************
VxD_REAL_INIT_SEG

BeginProc VwatchD_Real_Init
    mov    ax, Device_Load_Ok
    mov    bx, 0       ; No pages to exclude.
    mov    si, 0       ; No instance data items.
	clc
	ret
EndProc VwatchD_Real_Init

VxD_REAL_INIT_ENDS


;****************************************************************************
; init stuff
;==============================================================================

VXD_IDATA_SEG

; (some data used during initialization time)

VXD_IDATA_ENDS

VXD_ICODE_SEG		; Please see code and comments in VwatchD_Control first.

;******************************************************************************
;
;   VwatchD_Sys_Crit_Init
;
;   DESCRIPTION: Called during the first phase of PM init.
;
;   ENTRY: EBX: Sys VM handle
;
;   EXIT: carry flag clear.
;
;
;==============================================================================
BeginProc VwatchD_Sys_Crit_Init

	Trace_Out "VwatchD: Sys Critical Init."

	VMMCall _Allocate_Device_CB_Area, <<SIZE VwatchD_CB_STRUCT>, 0>
	or	eax, eax
	jz	short VwatchD_SCI_Error
	mov	[VwatchD_CB_Offset], eax
	add	eax, ebx
	mov	[eax.VM_Birth_Time], 0
	clc
	ret

VwatchD_SCI_Error:
;
; Debug_Out prints the message and breaks into the debugger.
;
	Debug_Out "VwatchD: Fail to allocate VM CB area."
	VMMjmp	Fatal_Memory_Error

EndProc VwatchD_Sys_Crit_Init

;******************************************************************************
;
;   VwatchD_Device_Init
;
;   DESCRIPTION: Called during the second phase of PM init.
;
;   ENTRY: EBX: Sys VM handle
;
;==============================================================================
BeginProc VwatchD_Device_Init

	Trace_Out "VwatchD: Device Init."
	clc
	ret

EndProc VwatchD_Device_Init

;******************************************************************************
;
;   VwatchD_Init_Complete
;
;   DESCRIPTION: Called during the final phase of PM init.
;
;   ENTRY: EBX: Sys VM handle
;
;
;==============================================================================
BeginProc VwatchD_Init_Complete

	Trace_Out "VwatchD: Init Complete"
	clc
	ret

EndProc VwatchD_Init_Complete

VXD_ICODE_ENDS

;******************************************************************************
; Code
;------------------------------------------------------------------------------
VXD_CODE_SEG

;******************************************************************************
;
;   VwatchD_Control
;
;   DESCRIPTION: The system (or VMM) calls this entry point for various events
;	that is occuring in the system, for example, system init, system exit,
;	VM create, VM init, or VM being destroyed... Here are some of the
;	"messages." There are others not used here. This proc dispatches the
;	call to the appropriate routines. Control_Dispatch is a macro. Please
;	see VMM.INC for more detailed information.
;==============================================================================
BeginProc VwatchD_Control

	Control_Dispatch Sys_Critical_Init,  VwatchD_Sys_Crit_Init
	Control_Dispatch Device_Init,        VwatchD_Device_Init
	Control_Dispatch Init_Complete,      VwatchD_Init_Complete

	Control_Dispatch SYS_VM_Init,        VwatchD_SYS_VM_Init

	Control_Dispatch Create_VM,          VwatchD_Create_VM
	Control_Dispatch VM_Critical_Init,   VwatchD_VM_Critical_Init
	Control_Dispatch VM_Init,            VwatchD_VM_Init
	Control_Dispatch VM_Suspend,         VwatchD_VM_Suspend
	Control_Dispatch VM_Resume,          VwatchD_VM_Resume
	Control_Dispatch Query_Destroy,      VwatchD_Query_Destroy

	Control_Dispatch Sys_VM_Terminate,   VwatchD_Sys_VM_Terminate
	Control_Dispatch VM_Terminate,       VwatchD_VM_Terminate
    Control_Dispatch Destroy_VM,         VwatchD_Destroy_VM
	Control_Dispatch VM_Not_Executeable, VwatchD_VM_Not_Executeable

	Control_Dispatch Set_Device_Focus,   VwatchD_Set_Focus

	Control_Dispatch Power_Event,        VwatchD_Power_Event

	Control_Dispatch System_Exit,        VwatchD_System_Exit
	Control_Dispatch Sys_Critical_Exit,  VwatchD_Sys_Critical_Exit

IFDEF DEBUG
	Control_Dispatch Debug_Query,	  VwatchD_Debug_Query
ENDIF

; * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
;  Various callback functions not implemented here.
;
;	Control_Dispatch Begin_PM_App,        VwatchD_Begin_PM_App
;	Control_Dispatch End_PM_App,          VwatchD_End_PM_App
;	Control_Dispatch Begin_Message_Mode,  VwatchD_Begin_Message_Mode
;	Control_Dispatch End_Message_Mode,    VwatchD_End_Message_Mode
;	Control_Dispatch Reboot_Processor,    VwatchD_Reboot_Processor

	clc
	ret

EndProc VwatchD_Control

;******************************************************************************
; Create VM
;==============================================================================

BeginProc VwatchD_Create_VM

	Trace_Out "VwatchD: VM #EBX is being created."

	mov	edi, [VwatchD_CB_Offset]
	add	edi, ebx
	VMMCall Get_System_Time
	mov	[edi.VM_Birth_Time], eax
	clc
	ret

EndProc VwatchD_Create_VM

;******************************************************************************
; System VM init
;==============================================================================
BeginProc VwatchD_SYS_VM_Init

	Trace_Out "VwatchD: System VM init (VM # is #EBX)"
	clc
	ret

EndProc VwatchD_SYS_VM_Init

;******************************************************************************
; VM init
;==============================================================================
BeginProc VwatchD_VM_Init

	Trace_Out "VwatchD: VM #EBX init"
	clc
	ret

EndProc VwatchD_VM_Init

;******************************************************************************
; VM Suspend
;==============================================================================
BeginProc VwatchD_VM_Suspend

	Trace_Out "VwatchD: VM #EBX Suspend"
	clc
	ret

EndProc VwatchD_VM_Suspend

;******************************************************************************
; VM Resume
;==============================================================================
BeginProc VwatchD_VM_Resume

	Trace_Out "VwatchD: VM #EBX Resume"
	clc
	ret

EndProc VwatchD_VM_Resume

;******************************************************************************
; VM Critical init
;==============================================================================
BeginProc VwatchD_VM_Critical_Init

	Trace_Out "VwatchD: VM #EBX Critical Init."
	clc
	ret

EndProc VwatchD_VM_Critical_Init

;******************************************************************************
; VM Terminate
;==============================================================================
BeginProc VwatchD_VM_Terminate

	Trace_Out "VwatchD: VM #EBX terminates."
	clc
	ret

EndProc VwatchD_VM_Terminate

;******************************************************************************
; Sys VM Terminate
;==============================================================================
BeginProc VwatchD_Sys_VM_Terminate

	Trace_Out "VwatchD: Sys VM terminates (VM # #EBX)."
	clc
	ret

EndProc VwatchD_Sys_VM_Terminate

;******************************************************************************
; VM Not Executable
;==============================================================================
BeginProc VwatchD_VM_Not_Executeable

	Trace_Out "VwatchD: VM #EBX not executeable."
	clc
	ret

EndProc VwatchD_VM_Not_Executeable

;******************************************************************************
; VM Query Destroy
;==============================================================================
BeginProc VwatchD_Query_Destroy

	Trace_Out "VwatchD: VM #EBX Query Destroy."
	clc
	ret

EndProc VwatchD_Query_Destroy

;******************************************************************************
; VM Destroy
;==============================================================================
BeginProc VwatchD_Destroy_VM

	mov	edi, [VwatchD_CB_Offset]
	add	edi, ebx
	VMMCall Get_System_Time
	sub	eax, [edi.VM_Birth_Time]
	Trace_Out "VwatchD: VM #EBX Destroyed. It was around for #eax milliseconds."
	clc
	ret

EndProc VwatchD_Destroy_VM

;******************************************************************************
;VwatchD_Set_Focus
;==============================================================================

BeginProc VwatchD_Set_Focus

	mov	eax, edx
	Trace_out "VwatchD: Set Device #EAX Focus to VM #EBX"
	clc
	ret

EndProc VwatchD_Set_Focus

;******************************************************************************
; VM Power Event
;==============================================================================
BeginProc VwatchD_Power_Event

	Trace_Out "VwatchD: Power Event."
	clc
	ret

EndProc VwatchD_Power_Event

;******************************************************************************
;VwatchD_System_Exit
;==============================================================================
BeginProc VwatchD_System_Exit

	Trace_Out "VwatchD: System Exit time."
	clc
	ret

EndProc VwatchD_System_Exit

;******************************************************************************
;VwatchD_Sys_Critical_Exit
;==============================================================================
BeginProc VwatchD_Sys_Critical_Exit

	Trace_Out "VwatchD: Sys Critical Exit."
	clc
	ret

EndProc VwatchD_Sys_Critical_Exit

;******************************************************************************
; VwatchD_Get_Version
;	The only VwatchD service provided in this code. VxD services can be
;	called by other VxDs. For example, "VxDCall VwatchD_Get_Version"
;==============================================================================
BeginProc VwatchD_Get_Version, Service

	Trace_Out "VwatchD: Service get version."
	mov	eax, 100h
	clc
	ret

EndProc VwatchD_Get_Version
;******************************************************************************
;VwatchD_V86_API
;	Entry point for V86 API. This procedure can be a dispatch point for
;	different functions of the API.
;
;Entry: EBX: Current VM handle
;	EBP -> Client Register Struct
;
;Exit: Client_DX:AX = VM's birth time
;==============================================================================

BeginProc VwatchD_V86_API

	Trace_Out "VwatchD: V86 API call from VM #EBX"
	mov	edi, [VwatchD_CB_Offset]
	add	edi, ebx
	mov	eax, [edi.VM_Birth_Time]
	mov	[ebp.Client_AX], ax
	shr	eax, 16
	mov	[ebp.Client_DX], ax
	clc
	ret

EndProc VwatchD_V86_API

;******************************************************************************
;VwatchD_PM_API
;	Entry point for PM API. This procedure can be a dispatch point for
;	different functions of the API.
;
;Entry: EBX: Current VM handle
;	EBP -> Client Register Struct
;Exit: VwatchD version number in Client_DX:AX
;==============================================================================

BeginProc VwatchD_PM_API
	Trace_Out "VwatchD: PM API call from VM #EBX"
	VxDCall VwatchD_Get_Version		; other VxDs can do the same.
	mov	[ebp.Client_AX], ax
	shr	eax, 16
	mov	[ebp.Client_DX], ax
	clc
	ret
EndProc VwatchD_PM_API

IFDEF DEBUG
;************************************************************************
;*
;* The Debug_Query section!
;*
;* We get control here when a user enters  ".VwatchD"  when in WDEB386.
;*
;************************************************************************
BeginProc VwatchD_Debug_Query
        Trace_Out "VwatchD: We are displaying debug info now."
        clc
        ret
EndProc VwatchD_Debug_Query

ENDIF

VXD_CODE_ENDS

	END

