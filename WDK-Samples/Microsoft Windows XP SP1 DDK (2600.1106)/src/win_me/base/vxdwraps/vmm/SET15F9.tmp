include local.inc

StartCDecl	List_Insert@12

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		pop	eax		; Get NewNode
		pop	ecx		; Get Node
		VxDCall	List_Insert
		mov	esi, ecx
		jmp	edx

EndCDecl	List_Insert@12

END
