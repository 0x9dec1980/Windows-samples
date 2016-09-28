include local.inc

StartCDecl	List_Destroy@4

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		VxDCall	List_Destroy
		mov	esi, ecx
		jmp	edx

EndCDecl	List_Destroy@4

END
