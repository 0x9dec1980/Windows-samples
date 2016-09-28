include local.inc

StartCDecl	MTRRSetPhysicalCacheTypeRange@12

		VxDCall	MTRRSetPhysicalCacheTypeRange, < \
			DWORD PTR [esp+16], 0, \
			DWORD PTR [esp+12], DWORD PTR [esp+12]>

		ret	4*3

EndCDecl	MTRRSetPhysicalCacheTypeRange@12

END
