include local.inc
StartCDecl	Get_Machine_Info@16

	push	ebx
	push	esi
	push	edi

	VxDCall	Get_Machine_Info

	mov	edi, eax
	lea	esi, [esp+4+3*4]

	lodsd
	or	eax, eax
	jz	@f
	mov	dword ptr [eax], edi
@@:

	lodsd
	or	eax, eax
	jz	@f
	mov	dword ptr [eax], ebx
@@:

	lodsd
	or	eax, eax
	jz	@f
	mov	dword ptr [eax], ecx
@@:

	lodsd
	or	eax, eax
	jz	@f
	mov	dword ptr [eax], edx
@@:

	pop	edi
	pop	esi
	pop	ebx
	ret	16

EndCDecl	Get_Machine_Info@16
END

