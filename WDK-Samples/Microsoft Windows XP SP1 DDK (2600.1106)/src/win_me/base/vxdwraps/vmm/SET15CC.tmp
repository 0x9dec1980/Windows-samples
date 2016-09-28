include local.inc

StartCDecl	List_Attach@8

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		pop	eax		; Get Node
		VxDCall	List_Attach
		mov	esi, ecx
		jmp	edx

EndCDecl	List_Attach@8

END
