include local.inc

StartCDecl	Cancel_Global_Event@4

		push	esi
		mov	esi, [esp+8]	; Get event handle
		VxDCall	Cancel_Global_Event
		pop	esi
		ret	4

EndCDecl	Cancel_Global_Event@4

END

