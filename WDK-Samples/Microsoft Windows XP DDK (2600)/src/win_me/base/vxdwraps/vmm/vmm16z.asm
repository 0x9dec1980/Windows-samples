include local.inc

StartCDecl	Cancel_Time_Out@4

		push	esi
		mov	esi, [esp+8]	; Get htimeout
		VxDCall	Cancel_Time_Out
		pop	esi
		ret	1*4

EndCDecl	Cancel_Time_Out@4

END

