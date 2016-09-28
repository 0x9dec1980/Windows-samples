include local.inc

StartCDecl VDMAD_Virtualize_Channel@8

	mov	ecx, esi	; save esi
	pop	edx		; Get return address
	pop	eax		; ulChannel
	pop	esi		; pHandler

        VxDCall VDMAD_Virtualize_Channel
	mov	esi, ecx	; restore esi
        cmc
        sbb     ecx, ecx
        and     eax, ecx
	jmp	edx

EndCDecl VDMAD_Virtualize_Channel@8

END

