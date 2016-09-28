include local.inc

StartCDecl	Write_Boot_Log@8

		pop	eax		; Get return address
		pop	ecx		; Get Length
		pop	edx		; Get string
		VMMCall	Write_Boot_Log
		jmp	eax

EndCDecl	Write_Boot_Log@8

END

