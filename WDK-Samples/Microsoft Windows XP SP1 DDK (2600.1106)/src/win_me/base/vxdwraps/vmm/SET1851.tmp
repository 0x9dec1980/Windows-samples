include local.inc

StartCDecl	Get_Next_Thread_Handle@4

		pop	ecx
		pop	edx
		push	edi
		mov	edi, edx
		VxDCall	Get_Next_Thread_Handle
		mov	eax, edi
		pop	edi
		jmp	ecx

EndCDecl	Get_Next_Thread_Handle@4

END


