include local.inc

StartCDecl	Set_VM_Time_Out@12

		push	esi
		mov	edx, [esp+16]	; Get ulRefData
		mov	eax, [esp+12]	; Get cms
		mov	esi, [esp+8]	; Get pfnTimeout
		VxDCall	Set_VM_Time_Out
		mov	eax, esi
		pop	esi
		ret	3*4

EndCDecl	Set_VM_Time_Out@12

END

