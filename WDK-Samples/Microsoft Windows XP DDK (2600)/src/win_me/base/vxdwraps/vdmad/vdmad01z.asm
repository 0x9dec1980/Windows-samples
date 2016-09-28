include local.inc

StartCDecl VDMAD_Phys_Mask_Channel@4

	pop	edx			; Get return address
	pop	eax			; get hdma
        VxDCall VDMAD_Phys_Mask_Channel
	jmp	edx

EndCDecl VDMAD_Phys_Mask_Channel@4

END
