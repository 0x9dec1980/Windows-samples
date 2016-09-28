include local.inc

StartCDecl	Test_Cur_VM_Handle@4

		push	ebx
		mov	ebx, [esp+8]	; Get hvm

		VxDCall	Test_Cur_VM_Handle
		sete	al
		pop	ebx
		movzx	eax, al
		ret	1*4

EndCDecl	Test_Cur_VM_Handle@4

END



