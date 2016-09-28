include local.inc

StartCDecl	Create_Semaphore@4

		pop	edx		; Get return address
		pop	ecx		; Get lTokenCount
		VxDCall	Create_Semaphore
		cmc
		sbb	ecx, ecx
		and	eax, ecx
		jmp	edx

EndCDecl	Create_Semaphore@4

END

