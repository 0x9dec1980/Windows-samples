include local.inc

StartCDecl	Install_Exception_Handler@4

		push	esi
		mov	esi, [esp+8]	; Get pveException
		VxDCall	Install_Exception_Handler
		setnc	al
		and	eax, 1
		pop	esi
		ret	1*4

EndCDecl	Install_Exception_Handler@4

END

