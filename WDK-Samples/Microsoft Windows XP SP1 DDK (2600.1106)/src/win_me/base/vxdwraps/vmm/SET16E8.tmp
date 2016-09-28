include local.inc

StartCDecl	Get_Profile_Boolean@12

		push	esi
		push	edi
		mov	eax, [esp+20]	; Get dwDefault
		mov	edi, [esp+16]	; Get pszKeyName
		mov	esi, [esp+12]	; Get pszSection
		VxDCall	Get_Profile_Boolean
		pop	edi
		pop	esi
		and	eax, 1
		ret	3*4

EndCDecl	Get_Profile_Boolean@12

END

