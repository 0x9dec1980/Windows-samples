include local.inc

StartCDecl	List_Get_First@4

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		VxDCall	List_Get_First
		mov	esi, ecx
		jmp	edx

EndCDecl	List_Get_First@4

END
