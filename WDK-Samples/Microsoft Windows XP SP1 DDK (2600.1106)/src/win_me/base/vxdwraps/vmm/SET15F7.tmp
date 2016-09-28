include local.inc

StartCDecl	List_Get_Next@8

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		pop	eax		; Get Node
		VxDCall	List_Get_Next
		mov	esi, ecx
		jmp	edx

EndCDecl	List_Get_Next@8

END
