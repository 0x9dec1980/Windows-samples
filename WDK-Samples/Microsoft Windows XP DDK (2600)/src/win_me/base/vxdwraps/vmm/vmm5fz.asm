include local.inc

StartCDecl	VMM_Add_DDB@4

		pop	edx		; Get return address
		mov	ecx, edi	; Save EDI
		pop	edi		; Get DDB
		VxDCall	VMM_Add_DDB
		sbb	eax, eax
		mov	edi, ecx	; Restore EDI
		inc	eax
		jmp	edx

EndCDecl	VMM_Add_DDB@4

END


