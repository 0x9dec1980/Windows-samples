include local.inc

StartCDecl	Get_Next_VM_Handle@4

		push	ebx
		mov	ebx, [esp+8]	; Get hvm

		VxDCall	Get_Next_VM_Handle
		mov	eax, ebx
		pop	ebx
		ret	1*4

EndCDecl	Get_Next_VM_Handle@4

END


