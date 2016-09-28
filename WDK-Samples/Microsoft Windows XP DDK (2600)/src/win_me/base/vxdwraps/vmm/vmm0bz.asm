include local.inc

StartCDecl	Begin_Critical_Section@4

		pop	edx		; Get return address
		pop	ecx		; Get Flags
		VxDCall	Begin_Critical_Section
		jmp	edx

EndCDecl	Begin_Critical_Section@4

END

