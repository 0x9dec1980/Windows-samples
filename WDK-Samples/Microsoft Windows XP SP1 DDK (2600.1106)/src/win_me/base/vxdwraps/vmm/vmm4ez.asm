include local.inc

StartCDecl	Get_Config_Directory

		VxDCall	Get_Config_Directory
		mov	eax, edx
		ret

EndCDecl	Get_Config_Directory

END

