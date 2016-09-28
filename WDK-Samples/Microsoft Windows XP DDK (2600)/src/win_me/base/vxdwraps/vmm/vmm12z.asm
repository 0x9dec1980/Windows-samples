include local.inc

StartCDecl	Wait_Semaphore@8

		pop	edx		; Get return address
		pop	eax		; Get Semaphore 
		pop	ecx		; Get flags
		VxDCall	Wait_Semaphore
		jmp	edx

EndCDecl	Wait_Semaphore@8

END

