include local.inc
StartCDecl	VXDLDR_DevInitSucceeded@4

	pop	ecx			; Get return address
	pop	edx			; Get DeviceHandle

	VxDCall	VXDLDR_DevInitSucceeded

	sbb	edx, edx		; If carry is set, the error code
	and	eax, edx		; is correct. Otherwise, clear eax

	jmp	ecx

EndCDecl	VXDLDR_DevInitSucceeded@4
END
