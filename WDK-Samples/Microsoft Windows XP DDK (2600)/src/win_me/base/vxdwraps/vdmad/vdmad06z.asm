include local.inc

StartCDecl VDMAD_Set_Phys_State@16

	push	ebx

        mov     eax, dword ptr [esp+8]	; hdma
        mov     ebx, dword ptr [esp+12]	; hVM
        mov     dl, byte ptr [esp+16]	; ulMode
        mov     dh, byte ptr [esp+20]	; ulExtMode

        VxDCall VDMAD_Set_Phys_State
        pop     ebx
	ret	4*4

EndCDecl VDMAD_Set_Phys_State@16

END


