include local.inc
StartCDecl	VPICD_Virtualize_IRQ@4

	push	edi
	mov	edi, [esp+4+1*4]

	VxDCall	VPICD_Virtualize_IRQ
	cmc
	sbb	ecx, ecx
	pop	edi
	and	eax, ecx

	ret	1*4
	
EndCDecl	VPICD_Virtualize_IRQ@4

END

