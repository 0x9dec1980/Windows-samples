include local.inc

StartCDecl	Unhook_V86_Int_Chain@8

		push	esi
		mov	esi, [esp+8]	; Get pfHook
		mov	eax, [esp+12]	; Get dwinterrupt

		VxDCall	Unhook_V86_Int_Chain
		setnc	al
		pop	esi
		movzx	eax, al
		ret	2*4

EndCDecl	Unhook_V86_Int_Chain@8

END

