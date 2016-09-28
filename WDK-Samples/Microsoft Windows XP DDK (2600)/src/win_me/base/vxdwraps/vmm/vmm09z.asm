include local.inc
StartCDecl	_Debug_Printf_Service

	lea	eax, [esp+8]
	push	eax
	push	dword ptr [esp+8]

	VxDCall	_Debug_Printf_Service
	add	esp, 2*4
	ret

EndCDecl	_Debug_Printf_Service
END
