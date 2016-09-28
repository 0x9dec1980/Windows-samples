include local.inc

StartCDecl	Get_Cur_VM_Handle

		push	ebx
		VxDCall	Get_Cur_VM_Handle
		mov	eax, ebx
		pop	ebx
		ret

EndCDecl	Get_Cur_VM_Handle

END


