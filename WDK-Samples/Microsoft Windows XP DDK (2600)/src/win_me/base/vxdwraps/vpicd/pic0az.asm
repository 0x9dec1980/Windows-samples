include local.inc
StartCDecl	VPICD_Physically_Unmask@4

	mov	eax, [esp+1*4]	; Get HIRQ
	VxDCall	VPICD_Physically_Unmask

	ret	1*4
	
EndCDecl	VPICD_Physically_Unmask@4
END

