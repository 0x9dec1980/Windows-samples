include local.inc

StartCDecl	Get_Cur_Thread_Handle

		push	edi
		VxDCall	Get_Cur_Thread_Handle
		mov	eax, edi
		pop	edi
		ret

EndCDecl	Get_Cur_Thread_Handle

END



