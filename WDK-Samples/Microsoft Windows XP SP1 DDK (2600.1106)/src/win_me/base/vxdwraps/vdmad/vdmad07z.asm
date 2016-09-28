include local.inc

StartCDecl VDMAD_Set_Region_Info@24

        push    ebx
        push    esi

        mov     eax, dword ptr [esp+12]	; hdma
        mov     bl, byte ptr [esp+16]	; ulBufferId
        mov     bh, byte ptr [esp+20]	; fLocked
        mov     esi, dword ptr [esp+24]	; ulLinear
        mov     ecx, dword ptr [esp+28] ; cbSize 
        mov     edx, dword ptr [esp+32] ; ulPhysical 

        VxDCall VDMAD_Set_Region_Info

        pop     esi
        pop     ebx
	ret	6*4

EndCDecl VDMAD_Set_Region_Info@24

END


