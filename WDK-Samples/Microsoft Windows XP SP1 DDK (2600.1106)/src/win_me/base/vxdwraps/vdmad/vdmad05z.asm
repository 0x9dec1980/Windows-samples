include local.inc

StartCDecl VDMAD_Get_Phys_Count@8

	pop	edx		; Get return address
	mov	ecx, ebx	; Save EBX
	pop	eax		; get hdma
	pop	ebx		; get hVM

        VxDCall VDMAD_Get_Phys_Count
	mov	ebx, ecx
	jmp	edx

EndCDecl VDMAD_Get_Phys_Count@8

END


