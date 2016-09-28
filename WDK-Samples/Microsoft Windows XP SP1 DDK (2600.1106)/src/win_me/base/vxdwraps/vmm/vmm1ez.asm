include local.inc

StartCDecl	List_Allocate@4

		pop	edx		; Get return address
		mov	ecx, esi	; save esi
		pop	esi		; Get List
		VxDCall	List_Allocate
		mov	esi, ecx
		cmc
		sbb	ecx, ecx
		and	eax, ecx
		jmp	edx

EndCDecl	List_Allocate@4

END
