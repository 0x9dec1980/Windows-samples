include local.inc

StartCDecl	List_Remove_First@4

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		VxDCall	List_Remove_First
		mov	esi, ecx
		jmp	edx

EndCDecl	List_Remove_First@4

END
