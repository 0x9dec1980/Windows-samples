include local.inc

StartCDecl	List_Deallocate@8

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		pop	eax		; Get Node
		VxDCall	List_Deallocate
		mov	esi, ecx
		jmp	edx

EndCDecl	List_Deallocate@8

END
