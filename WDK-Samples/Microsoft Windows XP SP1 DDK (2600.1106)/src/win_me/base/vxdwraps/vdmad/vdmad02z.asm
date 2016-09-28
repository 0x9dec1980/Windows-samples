include local.inc

StartCDecl VDMAD_Phys_Unmask_Channel@8
	
	push	ebx
	mov	edx, dword ptr [esp+8]	; get hdma
	mov	ebx, dword ptr [esp+12]	; get hVM
        VxDCall VDMAD_Phys_Unmask_Channel
	pop	ebx
	ret	2*4

EndCDecl VDMAD_Phys_Unmask_Channel@8

END

