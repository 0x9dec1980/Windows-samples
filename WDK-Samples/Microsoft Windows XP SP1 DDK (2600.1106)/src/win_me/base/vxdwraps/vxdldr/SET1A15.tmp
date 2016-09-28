include local.inc
StartCDecl	VXDLDR_LoadDevice@16

	mov	eax, [esp+4+3*4]	; Get flags
	mov	edx, [esp+4+2*4]	; Get Filename

	VxDCall	VXDLDR_LoadDevice
	jc	VXDLDR_Done

	mov	ecx, [esp+4+0*4]	; Get DDB pointer
	mov	[ecx], eax
	mov	ecx, [esp+4+1*4]	; Get DEVICEHANDLER pointer
	mov	[ecx], edx
	xor	eax, eax

VXDLDR_Done:
	ret	4*4
	
EndCDecl	VXDLDR_LoadDevice@16
END
