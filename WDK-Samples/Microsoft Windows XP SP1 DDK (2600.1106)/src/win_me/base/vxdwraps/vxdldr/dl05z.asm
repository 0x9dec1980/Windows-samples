include local.inc
StartCDecl	VXDLDR_DevInitFailed@4

	pop	ecx			; Get return address
	pop	edx			; Get DeviceHandle

	VxDCall	VXDLDR_DevInitFailed

	sbb	edx, edx		; If carry is set, the error code
	and	eax, edx		; is correct. Otherwise, clear eax

	jmp	ecx

EndCDecl	VXDLDR_DevInitFailed@4
END
