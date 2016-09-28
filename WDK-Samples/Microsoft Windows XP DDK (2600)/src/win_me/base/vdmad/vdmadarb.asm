ARBIT = 1

IFDEF ARBIT
PAGE 58,132
;******************************************************************************
TITLE VDMADARB.ASM - Virtual Direct Memory Access Device
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1988
;
;   Title:      VDMADARB.ASM -  DMA Arbitrator
;
;   Version:	3.03
;
;   Date:       23-Apr-1993
;
;   Author:     MDW
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   23-Apr-1993 MDW First version
;
;==============================================================================

	.386p


;******************************************************************************
;		 P U B L I C   D E C L A R A T I O N S
;******************************************************************************

        PUBLIC  VDMAD_PnP_Init
        PUBLIC  VDMAD_PnP_New_DevNode
        PUBLIC  VDMADArb_Alloc_Channel
        PUBLIC  VDMADArb_Unalloc_Channel
        PUBLIC  VDMADArb_Unmask_Channel
IF 0
IFDEF DEBUG
        PUBLIC  VDMAD_Debug_Arb
ENDIF
ENDIF
;******************************************************************************
;			      I N C L U D E S
;******************************************************************************


	.XLIST
	INCLUDE VMM.Inc
	INCLUDE Debug.Inc
	INCLUDE OptTest.Inc
        INCLUDE Configmg.Inc
        INCLUDE REGDEF.Inc
        INCLUDE VDMAD.INC
	INCLUDE REGSTR.INC
	.LIST


;******************************************************************************
;		   E X T E R N A L   D E C L A R A T I O N S
;******************************************************************************

VxD_ICODE_SEG

VxD_ICODE_ENDS

;------------------------------------------------------------------------------

VxD_LOCKED_CODE_SEG

VxD_LOCKED_CODE_ENDS


PAGE

;******************************************************************************
;		    I N I T I A L I Z A T I O N   D A T A
;******************************************************************************

VxD_IDATA_SEG

pRID            dd      0                       ; DMA arbitrator ID
phKey		dd	0			; Key for registry access

; Initial value for reserved mask retrieved from registry
szRSVDMask      db      "ReservedDMAMask",0
cbRSVDValueStr  EQU     16
cbszRSVDValue   dd      cbRSVDValueStr
szRSVDValue     db      cbRSVDValueStr DUP (0)
RSVDMaskType    dd      REG_SZ
szRsvdDMAKey	db	REGSTR_PATH_DMAARB, 0

IFDEF DEBUG
szARBITDebug	db	"ARBITDebug",0
ENDIF
VxD_IDATA_ENDS



;******************************************************************************
;			    L O C A L	D A T A
;******************************************************************************

VxD_PAGEABLE_DATA_SEG

	ALIGN	4
PUBLIC VDMAD_Devnode_Handle

VDMAD_Devnode_Handle	dd  0

IFDEF DEBUG
PUBLIC fDMAARBITDebug
fDMAARBITDebug     dd      0

Arb_Debug_Out  MACRO str, arg1
        Local string
        Local skipout
_LDATA	segment
string	db	str
ifb <arg1>
	db	0dh,0ah
endif
	db	0
_LDATA	ends
        pushfd
        cmp     [fDMAARBITDebug],0
        jz      SHORT skipout
        push    offset32 string
	VMMcall _Debug_Out_Service
skipout:
        popfd
        endm

Arb_Trace_Out  MACRO str, arg1
        Local string
        Local skipout
_LDATA	segment
string	db	str
ifb <arg1>
	db	0dh,0ah
endif
	db	0
_LDATA	ends
        pushfd
        cmp     [fDMAARBITDebug],0
        jz      SHORT skipout
        push    offset32 string
        VMMcall _Trace_Out_Service
skipout:
        popfd
        endm
ELSE
Arb_Debug_Out  MACRO str, arg1
        endm

Arb_Trace_Out  MACRO str, arg1
        endm

ENDIF

prd_Temp        dd      0                       ; Temp resource description

;
; Definitions for channel types (8, 16, 32 bit)
;
DMA_Ranges_s    STRUC
DR_Byte_Mask    db      ?
DR_Word_Mask    db      ?
DR_Dword_Mask   db      ?
DMA_Ranges_s    ENDS

%OUT BUGBUG: Are MCA and EISA DMA capabilities correct?
IF 0
;;; Following is to provide for selective allocation of BYTE, WORD
;;; or DWORD channels when the mask will allow any channel
pDMA_Ranges     dd      OFFSET32 ISA_Ranges
ISA_Ranges      DMA_Ranges_s <0Fh,0F0h,0>
EISA_Ranges     DMA_Ranges_s <0FFh,0FFh,0>
MCA_Ranges      DMA_Ranges_s <0FFh,0FFh,0FFh>
ENDIF


;
; The allocated DMA channel bit masks
;       8 bits of allocation bit mask (channels 0 - 7)
;

VDMAD_Arb_Data  db     0
VDMAD_Arb_Dup   db     ?
VDMAD_Arb_QF    db     ?

VDMAD_PreA_Mask db     00h			; Mask of prealloc'd channels
VDMAD_PreA_Dup  db     ?
VDMAD_Flags     dw      0                       ; Global flags
VF_RsvdDirtyBit EQU     0
VF_RsvdDirty    EQU     1 SHL VF_RsvdDirtyBit
VF_PNPInitBit   EQU     1
VF_PNPInit      EQU     1 SHL VF_PNPInitBit
VF_ForceAllocBit EQU    2
VF_ForceAlloc   EQU     1 SHL VF_ForceAllocBit

                ALIGN 4

;
; Arbitration function jump table
;
VDMAD_Arb_Jump  dd      OFFSET32 VDMAD_Test_Alloc
                dd      OFFSET32 VDMAD_Retest_Alloc
                dd      OFFSET32 VDMAD_Set_Alloc
                dd      OFFSET32 VDMAD_Release_Alloc
                dd      OFFSET32 VDMAD_Query_Free
                dd      OFFSET32 VDMAD_Remove
                dd      OFFSET32 VDMAD_Force_Alloc

VDMAD_Arb_Number_Of_Jumps	EQU	($-VDMAD_Arb_Jump)/4

VDMADALLOCFLAG  dd      CR_FAILURE

VxD_PAGEABLE_DATA_ENDS

VxD_LOCKED_DATA_SEG

extrn	VDMAD_Rsvd_Mask:byte

VxD_LOCKED_DATA_ENDS


VxD_PNP_CODE_SEG
;******************************************************************************
;
;   VDMAD_PnP_New_DevNode
;
;   DESCRIPTION:
;
;   ENTRY:	EBX=devnode.
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_PnP_New_DevNode
        bts     [VDMAD_Flags], VF_PNPInitBit
        jnc     VDI_Initialize
	mov	eax, CR_FAILURE				; Already done
	ret

VDI_Initialize:
	mov	[VDMAD_Devnode_Handle], ebx

	VxDCall	_CONFIGMG_Set_DevNode_PowerCapabilities, <ebx, CM_POWERSTATE_D0+CM_POWERSTATE_D3, CM_CAPABILITIES_NORMAL>

        VxDCall _CONFIGMG_Register_Enumerator, <ebx,0,CM_REGISTER_ENUMERATOR_ACPI_APM>

        VxDCall _CONFIGMG_Register_Device_Driver, <ebx,0,0,CM_REGISTER_DEVICE_DRIVER_ASYNCHRONOUS+\
			CM_REGISTER_DEVICE_DRIVER_ACPI_APM>

	mov	eax, CR_SUCCESS
        ret

EndProc VDMAD_PnP_New_DevNode

VxD_PNP_CODE_ENDS

;******************************************************************************
;		   I N I T I A L I Z A T I O N	 C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
;   VDMAD_PnP_Init
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_PnP_Init

	pushad

IFDEF   DEBUG
        xor     eax,eax
        xor     esi,esi
        mov     edi,OFFSET32 szARBITDebug
	VMMcall Get_Profile_Boolean
        mov     [fDMAArbitDebug],eax
ENDIF
;
; Get reserved DMA channels
;
        VMMCall _RegOpenKey, <HKEY_LOCAL_MACHINE,<OFFSET32 szRsvdDMAKey>,<OFFSET32 pHKey>>
        cmp     eax, ERROR_SUCCESS
        jnz     SHORT VDI_NoRegRSVDMask

        VMMCall _RegQueryValueEx, <pHKey,<OFFSET32 szRSVDMask>,0,<OFFSET32 RSVDMaskType>,<OFFSET32 szRSVDValue>,<OFFSET32 cbszRSVDValue>>
        cmp     eax, ERROR_SUCCESS
        jnz     SHORT VDI_NoRegRSVDMaskClose

        mov     edx, OFFSET32 szRSVDValue
        VMMcall Convert_Hex_String
        mov     [VDMAD_Rsvd_Mask], al
VDI_NoRegRSVDMaskClose:
        VMMCall _RegCloseKey, <pHKey>


VDI_NoRegRSVDMask:
        VxDCall _CONFIGMG_Get_Version
        jc      VDI_GiveUp

        VxDCall _CONFIGMG_Register_Arbitrator, <<OFFSET32 pRID>,ResType_DMA, <OFFSET32 VDMAD_Arbitrate>,0,0,CM_REGISTER_ARBITRATOR_GLOBAL>

	.errnz	CR_SUCCESS
	xor	ecx, ecx
	sub	ecx, eax	; set carry if eax was non-0

VDI_GiveUp:
	popad
	ret

EndProc VDMAD_PnP_Init

;******************************************************************************
;
; VDMADArb_Init_Complete
;
; DESCRIPTION:
;
; ENTRY:
;       none
;
; EXIT:
;       none
;
; USES:
;       Flags
;
;==============================================================================
BeginProc VDMADArb_Init_Complete

        pushad
        VMMCall _RegCreateKey, <HKEY_LOCAL_MACHINE,<OFFSET32 szRsvdDMAKey>,<OFFSET32 pHKey>>
        cmp     eax, ERROR_SUCCESS
        jnz     VIC_Exit
        mov     al, [VDMAD_Rsvd_Mask]
        mov     edi, OFFSET32 szRSVDValue
        rol     al, 4
        call    DumpHexChar
        rol     al, 4
        call    DumpHexChar
        mov     BYTE PTR [edi], 0
        VMMCall _RegSetValueEx, <[pHKey],<OFFSET32 szRsvdMask>,0,REG_SZ,<OFFSET32 szRsvdValue>,3>
IFDEF DEBUG
        cmp     eax, ERROR_SUCCESS
        jz      VIC_ExitClose
Trace_Out "Write of reserved DMA failed, #EAX"
VIC_ExitClose:
ENDIF

        VMMCall _RegCloseKey, <pHKey>
VIC_Exit:
	popad
	clc					; Always return success
        ret
EndProc VDMADArb_Init_Complete

;******************************************************************************
;
;   DumpHexChar
;
;   Outputs the low nibble in AL to the buffer pointed to by EDI
;
;------------------------------------------------------------------------------

VxD_IDATA_SEG
Hex_Char_Tab LABEL BYTE
	db	"0123456789ABCDEF"
VxD_IDATA_ENDS

BeginProc DumpHexChar, NO_PROLOG

	push	eax
        and     eax, 0Fh
        mov     al, Hex_Char_Tab[eax]
        mov     BYTE PTR [edi], al
        inc     edi
	pop	eax
	ret

EndProc DumpHexChar

VxD_ICODE_ENDS

VxD_PNP_CODE_SEG

;******************************************************************************
;
;       @doc    EXTERNAL        ARBIT
;
;       @api    INT | VDMAD_Arbitrate | Arbitrate the DMA channel requests
;               contained the Node_List passed to the routine. Return
;               CR_Success if no conflict, CR_Failure if conflict, else other
;               error code.
;
;       @parm   INT | function | One of the standard arbitrator functions
;               defined in CONFIGMG.H (ARB_TEST_ALLOC, ...)
;
;       @parm   ULONG | reference | Reference data that was specified when
;               arbitrator was registered.
;
;       @parm   DEVNODE | DevNode | The root device node.
;
;       @parm   ULONG | Parameter | For most arbitrator functions, this
;               parameter is a Node List Header pointer. For the
;               ARB_Query_Free function this parameter is a pointer to two
;               DWORDS to hold the result (see below)
;
;       @rdesc  Returns CR_SUCCESS if the function was successful.  Otherwise,
;               one of the following errors is returned:
;
;       @flag   CR_FAILURE | if the resource requirements for DMA channels
;               specified by the node list passed in for arbitration
;               had a conflict.
;
;       @flag   CR_OUT_OF_MEMORY | if not enough memory
;
;       @flag   CR_INVALID_FLAG | if ulFlags is not zero.
;
;       @rdesc  For a successful ARB_QUERY_FREE function, the address of a
;               DMA_Arb_s structure is placed in the first DWORD of the buffer
;               pointed to by the Parameter and the length of the structure is
;               placed in the second DWORD.
;
;       @comm   The DMA channels arbitrated are a global resource so this
;               arbitrator is registered at the root device node.
;
;******************************************************************************

BeginProc VDMAD_Arbitrate

        push    ebx
        push    esi
        push    edi
        mov     eax, CR_DEFAULT
        mov     esi, [esp+4+3*4]                ; ESI -> function
        cmp     esi, VDMAD_Arb_Number_Of_Jumps
        jae     SHORT VArb_Exit
        mov     ecx,[esp+16+3*4]                ; ECX -> Node_List header
        call    [VDMAD_Arb_Jump][esi*4]
VArb_Exit:
        pop     edi
        pop     esi
        pop     ebx
        ret
EndProc VDMAD_Arbitrate

;****************************************************************************
;
;       VDMAD_Test_Alloc - Test allocation of resource
;
;       DESCRIPTION: The arbitration routine will attempt to satisfy all
;               allocation requests contained in the node list for its
;               resource. Generally, the arbitration consists
;               of sorting the list according to most likely succesful
;               allocation order, making a copy of the current allocation
;               data strucuture(s), releasing all resource currently
;               allocated to Dev_Nodes on the list from the copy data structure
;               and then attempting to satisfy allocation requests
;               by passing through the entire list, trying all possible
;               combinations of allocations before failing. The arbitrator
;               saves the resultant successful allocations, both in the
;               node list per device and the copy allocation data structure.
;               The configuration manager is expected to subsequently call
;               either Arb_Set_Alloc or Arb_Release_Alloc.
;
;       ENTRY:  ECX -> node list header
;
;       EXIT:   EAX = CR_SUCCESS or CR_FAILURE
;
;***************************************************************************/

BeginProc VDMAD_Test_Alloc

; Sort the list
        mov     ebx,[ecx.nlh_Head]              ; EBX -> head of Node_List
        call    VDMAD_Sort_Res_Desc

; Initialize duplicate data structure
        mov     al, [VDMAD_Arb_Data]
        mov     [VDMAD_Arb_Dup], al

	mov	al, [VDMAD_PreA_Mask]
	mov	[VDMAD_PreA_Dup], al

; Unallocate allocated resource in duplicate data structure
        call    VDMAD_Arb_Unalloc

; Attempt allocation of resources for Res_Desc on node list
        mov     edx,[ebx.nl_Test_Req]           ; EDX -> log configuration
        call    VDMAD_Next_Element              ; Get first DMA alloc element
        clc
        jz      SHORT VArb_Done                 ; No more, successful dev alloc

        push    -1                              ; Don't allow resvd DMAs
        call    VDMAD_Alloc_Elements            ; Alloc all on node list
        lea     esp, [esp+4]
        jnc     SHORT VArb_Done                 ; If success, all done

Arb_Trace_Out "VDMAD Arbitrate - trying reserved DMAs"
        push    0                               ; Allow resvd DMAs
        call    VDMAD_Alloc_Elements            ; Alloc all on node list
        lea     esp, [esp+4]

VArb_Done:
        mov     [VDMADALLOCFLAG], CR_SUCCESS
        jnc     SHORT VTA_Exit
        mov     [VDMADALLOCFLAG], CR_FAILURE
VTA_Exit:
        mov     eax, [VDMADALLOCFLAG]
        ret

EndProc VDMAD_Test_Alloc

;******************************************************************************
;
; VDMAD_Alloc_Elements
;
; DESCRIPTION: Alloc Elements attempts to make an allocation from the current
;       alloc element. If it succeeds, it recursively calls Alloc Elements
;       on the next alloc element. If that succeeds, return success. If it
;       fails, attempt to allocate alternative DMA from current
;       alloc element. If it succeeds, recursively call as before. If it
;       fails, return failure. The above logic is mitigated by logic to
;       reserve certain DMAs for devices that have to be on those DMAs,
;       although they may not be currently in the system (e.g. a PCMCIA
;       COM port). To handle this, we first do the above logic skipping
;       all reserved DMAs. If it fails, we try again with the reserved
;       DMAs but the next alloc still skips reserved DMAs. If that fails,
;       we try once more without the reserved DMAs but allow the next
;       alloc to access reserved DMAs. Finally, we try the reserved DMAs
;       with the next alloc also trying reserved DMAs. That is, we make
;       four passes:
;       1) Current not reserved, Next and subsequent not reserved
;       If fail and forced not reserved, exit with failure
;       2) Current reserved, Next and subsequent not reserved
;       3) Current not reserved, Next and subsequent reserved and not reserved
;       4) Current reserved, Next and subsequent reserved and not reserved
;       This algorithm attempts to exhaust allocation of non reserved
;       DMAs before any allocation of reserved DMAs. There is also
;       some additional short cuts in the algorithm that recognizes cases
;       that will not succeed without actually trying the allocations.
;
;
; ENTRY:
;       [ESP+4] = Skip reserved DMAs flag
;       EBX = Current Node_List element
;       EDX = Current Res_Desc of Node_List element
;       EDI = Pointer to current element
;       ECX = Count of OR allocs (bits set) within alloc element
;
; EXIT:
;       CF = 1 if failure, CF = 0 if success (jnz success)
;
;
; USES:
;       EAX, EBX, ECX, EDX, ESI, EDI, Flags
;
;******************************************************************************

BeginProc VDMAD_Alloc_Elements, High_Freq

; Stack parameter and local variables definitions
SkipResvd  EQU  <DWORD PTR [ebp+8]>     ; Skip reserved DMAs flag
Skipping   EQU  <DWORD PTR [ebp-4]>     ; We are now skipping flag
ChildSkip  EQU  <DWORD PTR [ebp-8]>     ; Child invocation is skipping


        enter   8,0
        mov     eax,SkipResvd
        mov     Skipping, eax
        mov     eax,-1
        mov     ChildSkip,eax
VAE_Start:
        mov     eax,80h                         ; AX = initial DMA to try
VAE_TestDMA:
        test    [edi.DD_Req_Mask], al           ;Q: EAX DMA a choice?
        jz      SHORT VAE_NextChoice2           ;   N: Look for a choice
        call    VDMAD_CheckResvd                ; Q: Skip this choice?
IFDEF   DEBUG
        jc      SHORT VAE_Skipped
ELSE
        jc      SHORT VAE_NextChoice            ;   Y: Try next
ENDIF
        test    [VDMAD_Arb_Dup], al             ; Q: Is DMA available?
        jz      DEBFAR VAE_Got_Alloc            ;   Y: Allocate it
                                                ;   N: Try next alternative
VAE_NextChoice:
IFDEF DEBUG
        push    eax
        push    ebx
        push    ecx
        mov     ecx, skipping
        mov     bh,cl
        mov     ecx, ChildSkip
        mov     bl,cl
        mov     ah, [edi.DD_Req_Mask]
Arb_Trace_Out "Allocate DMA #AX to #EDI failed: skip #BX"
        pop     ecx
        pop     ebx
        pop     eax
ENDIF
VAE_NextChoice2:
        shr     al, 1
        jnz     VAE_TestDMA                     ; If CH correct, this always JMPs

; Failed allocation, either fail exit or retry with different flags
VAE_Fail:
        test    SkipResvd, -1                   ; Q: Force skip reserved?
        jnz     SHORT VAE_FailExit              ;    Y: Exit now

; When not forced to skip reserved, we just completed one of following:
;       1) Current alloc don't skip, next alloc skip
;       2) Current alloc skip, next alloc don't skip
;       3) Current alloc don't skip, next alloc don't skip
;
; Move to next case or exit

        xor     Skipping, -1                    ; Q: Skipping now?
IFDEF DEBUG
        jz      SHORT VAE_DebugStart
ELSE
        jz      VAE_Start                       ;    Y: Restart, no skipping
ENDIF
; Current not skipping, case 1 or 3. If 3, all done.
        xor     ChildSkip, -1                   ; Q: Have children not skipped?
IFDEF DEBUG
        jz      SHORT VAE_DebugStart
ELSE
        jz      VAE_Start                       ;    N: Try child skip
ENDIF

; Unsuccessful alloc
VAE_FailExit:
        leave
        stc
        ret

IFDEF DEBUG
VAE_Skipped:
        push    eax
        push    ebx
        push    ecx
        mov     ecx, skipping
        mov     bh,cl
        mov     ecx, ChildSkip
        mov     bl,cl
        mov     ah, [edi.DD_Req_Mask]
Arb_Trace_Out "Allocate DMA #AX to #EDI skipped, skip: #BX"
        pop     ecx
        pop     ebx
        pop     eax
ENDIF
        jmp     VAE_NextChoice2

IFDEF DEBUG
VAE_DebugStart:
        push    eax
        push    ebx
        push    ecx
        mov     ecx, skipping
        mov     bh,cl
        mov     ecx, ChildSkip
        mov     bl,cl
        mov     ah, [edi.DD_Req_Mask]
Arb_Trace_Out "Restart #AX, #EDI: skip #BX"
        pop     ecx
        pop     ebx
        pop     eax
        jmp     VAE_Start
ENDIF


VAE_Got_Alloc:
IFDEF DEBUG
        push    eax
        push    ebx
        push    ecx
        mov     ecx, skipping
        mov     bh,cl
        mov     ecx, ChildSkip
        mov     bl,cl
        mov     ah, [edi.DD_Req_Mask]
Arb_Trace_Out "Allocate DMA #AX to #EDI successful: skip #BX"
        pop     ecx
        pop     ebx
        pop     eax
ENDIF
        mov     ah, [VDMAD_Arb_Dup]             ; AH = old arbitration state
        or      [VDMAD_Arb_Dup], al             ; Allocate DMA
        pushad
        call    VDMAD_Next_Element              ; Next AND element
        jz      SHORT VAE_Success
        push    ChildSkip
        call    VDMAD_Alloc_Elements            ; Alloc rest of DMAs
        lea     esp, [esp+4]
        jnc     SHORT VAE_Success

; Unsuccesful, remove alloc then try alternative
        popad
IFDEF DEBUG
        push    eax
        push    ebx
        push    ecx
        mov     ecx, skipping
        mov     bh,cl
        mov     ecx, ChildSkip
        mov     bl,cl
        mov     ah, [edi.DD_Req_Mask]
Arb_Trace_Out "Unalloc DMA #AX, #EDI: skip #BX"
        pop     ecx
        pop     ebx
        pop     eax
ENDIF
        mov     [VDMAD_Arb_Dup],ah              ; Restore arbitration data
IFDEF DEBUG
        jmp     VAE_NextChoice2
ELSE
        jmp     VAE_NextChoice
ENDIF

; Successfully allocated all DMA descriptors
VAE_Success:
        popad
        add     esp,4
        bsf     eax, eax
        mov     [edi.DD_Alloc_Chan],al          ; Save DMA allocation
IFDEF DEBUG
        push    eax
        mov     ah, [edi.DD_Req_Mask]
Arb_Trace_Out "Allocated DMA #AX to #EDI"
        pop     eax
ENDIF
        leave
        clc
        ret

EndProc VDMAD_Alloc_Elements


;******************************************************************************
;
; VDMAD_Next_Element
;
; DESCRIPTION:
;       Find next AND element of current descriptor, or, if none, the
;       first DMA element in the next Res_Des, or, if none, the first
;       DMA element in the first Res_Des of the next node list element or,
;       if none, return zero flag set (out of elements).
;
; ENTRY:
;       EBX = Current Node_List element
;       EDX = Current Res_Des element in Node_List test request
;
; EXIT:
;       ZF = 1 if no more DMA elements on node list
;       EBX = Current Node_List element
;       EDX = Current Res_Des element in Node_List test request
;       EDI -> Current element header
;       ECX = Count of OR allocs within element
;
; USES:
;       EAX, EBX, ECX, EDX, EDI, Flags
;
;******************************************************************************
BeginProc VDMAD_Next_Element, High_Freq

VNE_Get_Next:
        VxDCall _CONFIGMG_Get_Next_Res_Des, <<OFFSET32 pRD_Temp>,edx,ResType_DMA,0,0>
        cmp     eax,CR_SUCCESS
        jnz     SHORT VNE_Next_Node
        mov     edi,[pRD_Temp]
	mov	edx, edi
        mov     al, [edi.DD_Req_Mask]
        call    VDMAD_Get_DMA_Count
        or      edi,edi
VNE_Done:
        ret

VNE_Next_Node:
        mov     ebx,[ebx.nl_Next]
        or      ebx,ebx
        jz      VNE_Done

VNE_Get_First:
        mov     edx,[ebx.nl_Test_Req]           ; EDX -> log configuration
        jmp     SHORT VNE_Get_Next              ; Get 1st Res_Des of log conf

EndProc VDMAD_Next_Element

;******************************************************************************
;
; VDMAD_Get_DMA_Count
;
; DESCRIPTION:
;       Count bits set in AL
;
; ENTRY:
;       AL = Mask of alternative DMA allocations
;
; EXIT:
;       ECX = Count of alternative DMA allocations
;
; USES:
;       ECX, flags
;
;******************************************************************************
BeginProc VDMAD_Get_DMA_Count

%OUT BUGBUG Use a loop here
        push    eax
; First add every other bit to the next bit
        mov     ah,0AAh
        and     ah,al
        xor     al,ah
        add     ah,ah
        adc     al,ah
; Now combine the four 2 bit counts into two nibble bit counts
        mov     ah,0CCh
        and     ah,al
        xor     al,ah
        shr     ah,2
        add     al,ah
; Next, combine two nibble counts
        mov     ah,al
        shr     ah,4
        and     al,0Fh
; Add the two counts together to get final result
        add     al,ah
        movzx   ecx,al
        pop     eax
        ret
EndProc VDMAD_Get_DMA_Count

;******************************************************************************
;
; VDMAD_CheckResvd
;
; DESCRIPTION:
;       See if we should skip this DMA request
;
; ENTRY:
;       AX = Mask of alternative DMA allocations
;       ECX = Count of alternative DMA allocations
;
; EXIT: CF = 1 if skip allocation
;
; USES:
;       ECX, flags
;
;******************************************************************************
BeginProc VDMAD_CheckResvd

        cmp     ecx,1                           ; Q: More than one choice?
        jbe     SHORT VCR_OneChoice             ;    Y: Never skip it
        test    [VDMAD_Rsvd_Mask], al
        jz      SHORT VCR_NotResvd

; DMA(s) in reserved table
VCR_Resvd:
        cmp     [Skipping],0                    ; Q: Skip flag set?
        jz      SHORT VCR_NoSkip                ;    N: return CY=0
VCR_Skip:
        stc                                     ;    Y: return CY=1
        ret

; DMA(s) not in reserved table
VCR_NotResvd:
        cmp     [Skipping],0                    ; Q: Skip flag set?
        jz      SHORT VCR_Skip                  ;    N: Skip DMA (CY = 1)
VCR_NoSkip:
        clc
        ret

; Single choice IRQ, put in reserved table if not already there
VCR_OneChoice:
        test    [VDMAD_Rsvd_Mask], al           ; Q: Already reserved?
        jz      SHORT VCR_NewReserve            ;    N: Make it so
        clc                                     ;    Y: Don't skip (CY = 0)
        ret
VCR_NewReserve:
        or      [VDMAD_Rsvd_Mask], al           ; Make this IRQ reserved IRQ
        SetFlag VDMAD_Flags, VF_RsvdDirty       ; Newly reserved, remember chg
        clc                                     ; Don't skip (CY = 0)
        ret

EndProc VDMAD_CheckResvd

;****************************************************************************
;
;	VDMADArb_Alloc_Channel - Preallocate DMA channel
;
;	DESCRIPTION: This routine unconditionally preallocates a channel. It is
;               called when VDMAD_Virtualize_Channel service is called. A PnP
;               VxD will have already reserved the channel and so this call
;               will be a NOP. Non PnP VxDs will prevent the channel from being
;               incorrectly used in the future by PnP VxDs.
;
;       ENTRY:  EAX = channel number
;
;       EXIT:   none
;
;       USES:   Flags
;
;***************************************************************************/

BeginProc VDMADArb_Alloc_Channel

        cmp     eax, 8
	jae	SHORT @F
	bts	DWORD PTR [VDMAD_Arb_Data], eax 	; Q: DMA allocated?
	jc	SHORT @F
	bts	DWORD PTR [VDMAD_PreA_Mask], eax	;    N: DMA Prealloc'd
@@:
        ret
EndProc VDMADArb_Alloc_Channel

;****************************************************************************
;
;       VDMADArb_Unalloc_Channel - unallocate channel
;
;       DESCRIPTION:
;
;       ENTRY:  EAX = channel number
;
;       EXIT:   none
;
;       USES:   Flags
;
;***************************************************************************/
BeginProc VDMADArb_Unalloc_Channel

        cmp     eax, 8
        jae     @F
	btr	DWORD PTR [VDMAD_PreA_Mask], eax	; Q: DMA preallocated?
	jnc	SHORT @F
	btr	DWORD PTR [VDMAD_Arb_Data], eax 	;   Y: unallocate it
@@:
        ret
EndProc VDMADArb_Unalloc_Channel

;****************************************************************************
;
;       VDMAD_Retest_Alloc - Test allocation of resource
;
;       DESCRIPTION: The arbitration routine will attempt to satisfy all
;               allocation requests contained in the node list for its
;               resource. Generally, the arbitration consists
;               of sorting the list according to most likely succesful
;               allocation order, making a copy of the current allocation
;               data strucuture(s), releasing all resource currently
;               allocated to Dev_Nodes on the list from the copy data structure
;               and then attempting to satisfy allocation requests
;               by passing through the entire list, trying all possible
;               combinations of allocations before failing. The arbitrator
;               saves the resultant successful allocations, both in the
;               node list per device and the copy allocation data structure.
;               The configuration manager is expected to subsequently call
;               either Arb_Set_Alloc or Arb_Release_Alloc.
;
;       ENTRY:  ECX -> node list header
;
;       EXIT:   EAX = CR_SUCCESS or CR_FAILURE
;
;***************************************************************************/

BeginProc VDMAD_Retest_Alloc

	btr	[VDMAD_Flags], VF_ForceAllocBit

;*******
; Entry point for FORCE_ALLOC with VF_ForceAllocBit set
;*******
VDMAD_Retest_Common:
        mov     ebx,[ecx.nlh_Head]              ; EBX -> head of Node_List

; Initialize duplicate data structure
        mov     al, [VDMAD_Arb_Data]
        mov     [VDMAD_Arb_Dup], al

	mov	al, [VDMAD_PreA_Mask]
	mov	[VDMAD_PreA_Dup], al

; Unallocate allocated resource in duplicate data structure
        call    VDMAD_Arb_Unalloc
        cmp     eax, CR_SUCCESS     ; Unalloc went okay?
        jnz     SHORT VRA_Done

; Attempt allocation of resources for Res_Desc on node list
        mov     edx,[ebx.nl_Test_Req]           ; EDX -> log configuration
VRA_NextElement:
        call    VDMAD_Next_Element              ; Get first DMA alloc element
        jz      SHORT VRA_Success               ; No more, successful dev alloc
        movzx   eax, [edi.DD_Alloc_Chan]        ; Get alloc channel
        cmp     eax, 7
        ja      SHORT VRA_Failure2
	bts	DWORD PTR [VDMAD_Rsvd_Mask], eax ; Reserve the channel
	bt	DWORD PTR [VDMAD_Arb_Dup], eax	; Q: already allocated?
        jc      SHORT VRA_Failure               ;    Y: failure
VRA_Okay:
Arb_Trace_Out "Allocated DMA #AX to #EDI"
	bts	DWORD PTR [VDMAD_Arb_Dup], eax	; allocate
	btr	DWORD PTR [VDMAD_PreA_Dup], eax  ; reset preallocate
        jmp     VRA_NextElement

VRA_Success:
        mov     eax, CR_SUCCESS
VRA_Done:
        mov     [VDMADALLOCFLAG], eax
        ret

VRA_Failure:
	bt	DWORD PTR [VDMAD_PreA_Dup], eax  ; Q: preallocated?
	jc	SHORT VRA_Okay			;    Y: alloc anyway
	bt	[VDMAD_Flags], VF_ForceAllocBit ; Q: Force Alloc?
	jc	SHORT VRA_Okay			;    Y: alloc anyway
VRA_Failure2:
        mov     eax, CR_FAILURE
        jmp     VRA_Done
EndProc VDMAD_Retest_Alloc

;****************************************************************************
;
;       VDMAD_Force_Alloc - Allocate DMA resource
;
;       DESCRIPTION: The arbitration routine will satisfy all
;               allocations contained in the node list for the DMA
;               resource.
;
;       ENTRY:  ECX -> node list header
;
;       EXIT:   EAX = CR_SUCCESS or error code
;
;***************************************************************************/

BeginProc VDMAD_Force_Alloc

        bts     [VDMAD_Flags], VF_ForceAllocBit
        jmp     VDMAD_Retest_Common
EndProc VDMAD_Force_Alloc

;****************************************************************************
;
;       VDMAD_Set_Alloc - Make test alloc current alloc
;
;       DESCRIPTION: Saves current Test_Alloc or Retest_Alloc result as
;               permanent DMA allocation. If Test_Alloc or Retest_Alloc has
;               not been called, or did not return success, this routine
;               just releases the data structure and returns the error code.
;
;       ENTRY:  ECX -> node list header
;
;       EXIT:   EAX = Result of previous Test_Alloc or Retest_Alloc
;
;       USES:   All except EBX, ESI, EDI
;
;***************************************************************************/

BeginProc VDMAD_Set_Alloc

        cmp     [VDMADALLOCFLAG], CR_SUCCESS
	mov	eax, CR_FAILURE
        mov     [VDMADALLOCFLAG], eax
        jnz     SHORT VSA_Exit

; Copy DUP data back to arbitration data structure
        mov     al, [VDMAD_Arb_Dup]
        mov     [VDMAD_Arb_Data], al

	mov	al, [VDMAD_PreA_Dup]
	mov	[VDMAD_PreA_Mask], al

        mov     eax, CR_SUCCESS
VSA_Exit:
        ret
EndProc VDMAD_Set_Alloc

;****************************************************************************
;
;       VDMAD_Release_Alloc - Release test alloc
;
;       DESCRIPTION: Unconditionally releases the DUP data structure and
;               returns the result of the previous Test_Alloc or Retest_Alloc.
;
;       ENTRY:  ECX -> node list header
;
;       EXIT:   EAX = CR_Success
;
;       USES:   All except EBX, ESI, EDI
;
;***************************************************************************/

BeginProc VDMAD_Release_Alloc

        mov     [VDMADALLOCFLAG], CR_FAILURE
        mov     eax, CR_SUCCESS
        ret
EndProc VDMAD_Release_Alloc

;******************************************************************************
;
; VDMAD_Query_Free
;
; DESCRIPTION: This routine fills in the inverse list and returns a pointer
;       to the range list header
;
; ENTRY:
;       ECX -> Buffer Descriptor
;
; EXIT:
;       EAX = CR_Success
;
; USES:
;       All except EBX, ESI, EDI
;
;******************************************************************************

BeginProc VDMAD_Query_Free

        mov     al, [VDMAD_Arb_Data]
        not     al
        mov     [VDMAD_Arb_QF], al
        mov     DWORD PTR [ecx], OFFSET32 VDMAD_ARB_QF
        mov     DWORD PTR [ecx+4],1
        mov     eax, CR_Success
VQF_Done:
        ret
EndProc VDMAD_Query_Free

;******************************************************************************
;
; VDMAD_Remove
;
; DESCRIPTION: This routine does nothing
;
; ENTRY:
;       ECX -> node list header
;
; EXIT:
;       EAX = CR_Success
;
; USES:
;       Flags, EAX
;
;******************************************************************************

BeginProc VDMAD_Remove
        mov     eax, CR_Success
        ret
EndProc VDMAD_Remove

;****************************************************************************
;
;       VDMAD_Sort_Res_Desc - Sort node list
;
;       DESCRIPTION:
;
;       ENTRY:  ECX -> node list header
;               EBX -> head of node list
;
;       EXIT:
;
;***************************************************************************/

BeginProc VDMAD_Sort_Res_Desc
        ret
EndProc VDMAD_Sort_Res_Desc

;****************************************************************************
;
;       VDMAD_Arb_Unalloc - Unalloc channels allocated in node list from DUP
;
;       DESCRIPTION:
;
;       ENTRY:  ECX -> node list header
;               EBX -> head of node list
;
;       EXIT:
;
;***************************************************************************/

BeginProc VDMAD_Arb_Unalloc
; Unallocate any currently allocated DMA channels assigned to nodes in Node_List
        pushad
VAU_Node:
        VxDCall _CONFIGMG_Get_First_Log_Conf,<<OFFSET32 pRD_Temp>,[ebx].nl_ItsDevNode,ALLOC_LOG_CONF>
        cmp     eax,CR_Success
        jnz     DEBFAR VAU_NoLogConf
VAU_NextElement:
        mov     eax,[pRD_Temp]
        VxDCall _CONFIGMG_Get_Next_Res_Des, <<OFFSET32 pRD_Temp>,eax,ResType_DMA,0,0>
        cmp     eax,CR_Success                  ; Q: Any allocation?
        jnz     SHORT VAU_NoResDes              ;    N: next node in list

        mov     edi,[pRD_Temp]
        movzx   eax, [edi.DD_Alloc_Chan]        ; Get allocated channel num
        btr     DWORD PTR [VDMAD_Arb_Dup], eax  ; Clear global alloc mask
        jmp     VAU_NextElement

VAU_NoResDes:
IFDEF DEBUG
        cmp     eax, CR_NO_MORE_RES_DES
        jz      SHORT VAU_NextNode
Arb_Debug_Out "Unexpected error #EAX finding RES_DES for node at #EBX"
        jmp     SHORT VAU_NextNode
ENDIF


VAU_NoLogConf:
IFDEF DEBUG
        mov     ecx,[ebx.nl_ItsDevNode]
Arb_Trace_Out "No Allocated Log Conf for node at #ECX"
        cmp     eax, CR_NO_MORE_LOG_CONF
        jz      SHORT VAU_NextNode
Arb_Debug_Out "Unexpected error #EAX finding Log Conf for node at #ECX"
        jmp     SHORT VAU_NextNode
ENDIF


VAU_NextNode:
        mov     ebx,[ebx.nl_Next]               ; EBX = next node in list
        or      ebx,ebx
        jnz     VAU_Node
        popad
        mov     eax, CR_Success
        ret
EndProc VDMAD_Arb_Unalloc

VxD_PNP_CODE_ENDS


VxD_LOCKED_CODE_SEG

;****************************************************************************
;
;       VDMADArb_Unmask_Channel - Put channel on reserved list
;
;       DESCRIPTION: This routine will put the unmasked channel on the
;		reserved list unconditionally
;
;       ENTRY:  EAX = channel number
;
;       EXIT:   none
;
;       USES:   Flags
;
;***************************************************************************/
BeginProc VDMADArb_Unmask_Channel

        cmp     eax, 8
	jae	@F
	bts	DWORD PTR [VDMAD_Rsvd_Mask], eax	;   N: Reserve it
@@:
        ret
EndProc VDMADArb_Unmask_Channel

VxD_LOCKED_CODE_ENDS


%OUT BUGBUG: Need debug output
IF 0
IFDEF DEBUG
;******************************************************************************
;		       D E B U G G I N G   C O D E
;******************************************************************************


VxD_DEBUG_ONLY_CODE_SEG

;******************************************************************************
;
;   VDMAD_Debug_Arb
;
;   DESCRIPTION:
;	This procedure prints interesting information about the state of the
;       DMA channel arbitration/allocation.
;
;   ENTRY:
;	None
;
;   EXIT:
;	None
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Debug_Arb

	ret

EndProc VDMAD_Debug_Arb
VxD_DEBUG_ONLY_CODE_ENDS
ENDIF

ENDIF

ENDIF

	END
