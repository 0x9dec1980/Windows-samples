include local.inc

StartCDecl	Get_DDB@8

		push	edi
		mov	edi, [esp+12]		; Get Name
		movzx	eax, word ptr [esp+8]	; Get DeviceID
		VxDCall	Get_DDB
		mov	eax, ecx
		pop	edi
		ret	2*4

EndCDecl	Get_DDB@8

END

