include local.inc

StartCDecl	Cancel_Restricted_Event@4

		push	esi
		mov	esi, [esp+8]	; Get handle

		VxDCall	Cancel_Restricted_Event
		pop	esi
		ret	1*4

EndCDecl	Cancel_Restricted_Event@4

END

