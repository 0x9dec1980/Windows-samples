include local.inc

StartCDecl	Destroy_Semaphore@4

		pop	edx		; Get return address
		pop	eax		; Get Semaphore
		VxDCall	Destroy_Semaphore
		jmp	edx

EndCDecl	Destroy_Semaphore@4

END

