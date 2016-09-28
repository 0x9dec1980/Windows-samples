include local.inc

StartCDecl	List_Attach_Tail@8

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		pop	eax		; Get Node
		VxDCall	List_Attach_Tail
		mov	esi, ecx
		jmp	edx

EndCDecl	List_Attach_Tail@8

END
