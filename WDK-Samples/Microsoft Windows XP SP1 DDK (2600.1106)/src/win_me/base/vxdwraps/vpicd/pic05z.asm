include local.inc
StartCDecl	VPICD_Phys_EOI@4

	mov	eax, [esp+1*4]	; Get HIRQ
	VxDCall	VPICD_Phys_EOI

	ret	1*4
	
EndCDecl	VPICD_Phys_EOI@4
END

