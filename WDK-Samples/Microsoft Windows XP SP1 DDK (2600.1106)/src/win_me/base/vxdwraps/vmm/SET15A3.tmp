include local.inc

StartCDecl	Directed_Sys_Control@24

		push	ebx
		push	esi
		push	edi
		mov	edi, [esp+36]	; Get EDI
		mov	esi, [esp+32]	; Get ESI
		mov	edx, [esp+28]	; Get EDX
		mov	ebx, [esp+24]	; Get EBX
		mov	eax, [esp+20]	; Get SysControl
		mov	ecx, [esp+16]	; Get pDDB
		VxDCall	Directed_Sys_Control
		sbb	ebx, ebx
		and	eax, ebx
		pop	edi
		pop	esi
		pop	ebx
		ret	6*4

EndCDecl	Directed_Sys_Control@24

END

