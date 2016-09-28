include local.inc

StartCDecl	Call_Restricted_Event@24

		push	esi
		push	edi
		push	ebx
		mov	edi, [esp+36]	; Get timeout
		mov	esi, [esp+32]	; Get procedure
		mov	edx, [esp+28]	; Get ref data
		mov	ecx, [esp+24]	; Get flags
		mov	ebx, [esp+20]	; Get handle
		mov	eax, [esp+16]	; Get boost
		VxDCall	Call_Restricted_Event
		mov	eax, esi
		pop	ebx
		pop	edi
		pop	esi
		ret	6*4

EndCDecl	Call_Restricted_Event@24

END

