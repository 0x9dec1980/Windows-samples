include local.inc

StartCDecl	SHELL_SYSMODAL_Message@16

		push	ebx
		push	edi
		mov	ebx, [esp+12]	; Get VMM
		mov	eax, [esp+16]	; Get dwMBFlags
		or	eax, MB_SYSTEMMODAL
		mov	ecx, [esp+20]	; Get pszMessage
		mov	edi, [esp+24]	; Get pszCaption
		VxDCall	SHELL_SYSMODAL_Message
		pop	edi
		pop	ebx
		ret	4*4

EndCDecl	SHELL_SYSMODAL_Message@16

END

