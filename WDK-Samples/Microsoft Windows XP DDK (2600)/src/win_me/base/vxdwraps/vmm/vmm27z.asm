include local.inc

StartCDecl	List_Remove@8

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		pop	eax		; Get Node
		VxDCall	List_Remove
		mov	esi, ecx
		sbb	eax, eax
		inc	eax
		jmp	edx

EndCDecl	List_Remove@8

END
