include local.inc

StartCDecl	Schedule_Global_Event@8

		push	esi
		mov	edx, [esp+12]	; Get ulRefData
		mov	esi, [esp+8]	; Get event
		VxDCall	Schedule_Global_Event
		mov	eax, esi
		pop	esi
		ret	2*4

EndCDecl	Schedule_Global_Event@8

END

