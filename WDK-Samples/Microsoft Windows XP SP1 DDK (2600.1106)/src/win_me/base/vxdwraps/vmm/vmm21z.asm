include local.inc

StartCDecl	List_Create@8

		pop	edx		; Get return address
		pop	eax		; Get Flags
		pop	ecx		; Get NodeSize
		push	esi
		VxDCall	List_Create
		mov	eax, esi
		pop	esi
		cmc
		sbb	ecx, ecx
		and	eax, ecx
		jmp	edx

EndCDecl	List_Create@8

END
