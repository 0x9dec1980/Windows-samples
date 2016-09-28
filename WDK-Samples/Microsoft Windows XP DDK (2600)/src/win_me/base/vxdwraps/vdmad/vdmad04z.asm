include local.inc

StartCDecl VDMAD_Unvirtualize_Channel@4

	pop	edx		; Get return address
	pop	eax		; get hdma
	push	edx		; Reset return address
        VxDJmp	VDMAD_Unvirtualize_Channel

EndCDecl VDMAD_Unvirtualize_Channel@4

END


