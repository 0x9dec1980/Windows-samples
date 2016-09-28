;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
;                                                                           *
;****************************************************************************

AssertIOP	MACRO	iopptr
local	alloced, isiop
IFDEF DEBUG
	push	ebx
	mov	ebx, iopptr
	mov	ebx, [ebx].IOP_ior.IOR_private_IOS
	test	[ebx - size MED].MED_flags, MED_alloc
	jnz	alloced
	Debug_Out "DRAGON: IOP assertion failure - #ebx not allocated"
alloced:
	cmp	[ebx - size MED].MED_type, MED_IOP
	je		isiop
	Debug_Out "DRAGON: IOP assertion failure - #ebx not IOP"
isiop:
	pop	ebx
ENDIF
ENDM

AssertSRB	MACRO srbptr
local	alloced, issrb
IFDEF DEBUG
	push	ebx
	mov	ebx, srbptr
	mov	ebx, [ebx].SrbIopPointer
	mov	ebx, [ebx].IOP_ior.IOR_private_IOS
	test	[ebx - size MED].MED_flags, MED_alloc
	jnz	alloced
	Debug_Out "DRAGON: IOP assertion failure - #ebx not allocated"
alloced:
	cmp	[ebx - size MED].MED_type, MED_IOP
	je		issrb
	Debug_Out "DRAGON: IOP assertion failure - #ebx not SRB"
issrb:
	pop	ebx
ENDIF
ENDM

AssertDDB	MACRO	DDBptr
local	alloced, isDDB
IFDEF DEBUG
	cmp	[DDBptr].DDB_sig, 'DB'
	je		isDDB
	Debug_Out "DRAGON: DDB assertion failure - #ebx not DDB"
isDDB:
ENDIF
ENDM

AssertDCB	MACRO	DCBptr
local	alloced, isDCB
IFDEF DEBUG
	cmp	[DCBPtr].DCB_sig, 'CB'
	je		isDCB
	Debug_Out "DRAGON: DCB assertion failure - #DCBPtr not DCB"
isDCB:
ENDIF
ENDM

AssertVRP	MACRO	VRPptr
local	alloced, isVRP
IFDEF DEBUG
	push	ebx
	mov	ebx, [VRPptr].VRP_device_handle
	cmp	[ebx - DCB_bdd].DCB_sig, 'CB'
	je		isVRP
	Debug_Out "DRAGON: VRP assertion failure - #ebx not VRP"
isVRP:
	pop	ebx
ENDIF
ENDM


AssertPCD	MACRO	PCDptr
local	alloced, isPCD
IFDEF DEBUG
	push	ebx
	mov	ebx, PCDptr
	test	[ebx - size MED].MED_flags, MED_alloc
	jnz	alloced
	Debug_Out "DRAGON: PCD assertion failure - #ebx not allocated"
alloced:
	cmp	[ebx - size MED].MED_type, MED_PCD
	je		isPCD
	Debug_Out "DRAGON: PCD assertion failure - #ebx not PCD"
isPCD:
	pop	ebx
ENDIF
ENDM


VerPrintf	MACRO	num, fmt, vars
local	FmtData, FmtNumVars

IFDEF	DEBUG

if VERBOSE GE num

	pushfd

extrn	PrintFunc:near

	PushIt	vars
	FmtNumVars = 4
irp	v, <vars>
	FmtNumVars = FmtNumVars + 4
endm
	push	offset32 FmtData
	call	PrintFunc

	popfd

VxD_DATA_SEG
FmtData	DW	FmtNumVars
	DB	fmt, 0
VxD_DATA_ENDS

endif

ENDIF

ENDM






PushIt	MACRO	v1,v2,v3,v4,v5,v6,v7,v8,v9

ifnb	<v1>

if	(type(v1) GT 4)
.err "Non BYTE/WORD/DWORD arg passed to MACRO PUSHIT -- IODEBUG.H"
endif
			PushIt	v2,v3,v4,v5,v6,v7,v8,v9
   ifidni <v1>, <ah>
			push	dword ptr (0)
			mov	byte ptr [esp], ah
	else
   ifidni <v1>, <bh>
			push	dword ptr (0)
			mov	byte ptr [esp], bh
	else
   ifidni <v1>, <ch>
			push	dword ptr (0)
			mov	byte ptr [esp], ch
	else
   ifidni <v1>, <dh>
			push	dword ptr (0)
			mov	byte ptr [esp], dh
	else
   ifidni <v1>, <al>
			push	eax
			and	dword ptr [esp], 00FFh
	else
   ifidni <v1>, <bl>
			push	ebx
			and	dword ptr [esp], 00FFh
	else
   ifidni <v1>, <cl>
			push	ecx
			and	dword ptr [esp], 00FFh
	else
   ifidni <v1>, <dl>
			push	edx
			and	dword ptr [esp], 00FFh
	else
		push	dword ptr (v1)
		if (type (v1)) EQ 1
			and	dword ptr [esp], 0FFh
	   else
		if (type (v1)) EQ 2
			and	dword ptr [esp], 0FFFFh

		endif
  		endif
	endif
	endif
	endif
	endif
	endif
	endif
	endif
	endif
endif

ENDM
