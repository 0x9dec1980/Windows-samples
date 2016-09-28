include local.inc
StartCDecl	VXDLDR_UnloadDevice@8

	pop	ecx			; Get return address
	pop	eax			; Get device ID (to be put in BX)
	pop	edx			; Get SzName

	push	ebx
	mov	ebx, eax

	VxDCall	VXDLDR_UnloadDevice
	
	sbb	edx, edx		; If carry is set, the error code
	and	eax, edx		; is correct. Otherwise, clear eax

	pop	ebx
	jmp	ecx

EndCDecl	VXDLDR_UnloadDevice@8
END
