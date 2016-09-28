include local.inc

StartCDecl	Get_Exec_Path@4

		VxDCall	Get_Exec_Path
		mov	eax, [esp+4]	; Get pulLength
		mov	[eax], ecx
		mov	eax, edx
		ret	1*4

EndCDecl	Get_Exec_Path@4

END
