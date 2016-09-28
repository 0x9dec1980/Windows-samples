include local.inc

StartCDecl	Get_Execution_Focus

		push	ebx
		VxDCall	Get_Execution_Focus
		mov	eax, ebx
		pop	ebx
		ret

EndCDecl	Get_Execution_Focus

END

