;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: glint.h
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/


#pragma warning(disable:4103)
#include <windows.h>
#pragma warning(default:4103)
#include <winerror.h>
// #include <stdio.h>
#include "brdconf.h"
#include "dibeng.h"

/* For now remove the Sync With Glint macro		*/
#define SYNC_WITH_GLINT
#define VideoDebugPrint(arg)

typedef DWORD FAR*       PULONG;

typedef struct {
	union {
		struct GlintReg		Glint;
	};
	
}	*PREGISTERS; 

#define wScreenWidth        ((unsigned short)(pGLInfo->dwScreenWidth))
#define wScreenHeight       ((unsigned short)(pGLInfo->dwScreenHeight))
#define wBpp                ((unsigned short)(pGLInfo->dwBpp))
#define wScreenWidthBytes   ((unsigned short)(pGLInfo->dwScreenWidthBytes))

extern LPGLINTINFO PASCAL pGLInfo;


// Some defines for text initialisation	in InitialiseFontCache
extern void far PASCAL InitialiseFontCacheFromC(DWORD);
#define TEXT_CACHED_IN_OFFSCREEN     0x3d01
#define TEXT_CACHED_IN_MEMORY        0x3d02


extern void far PASCAL BeginAccess();
extern void far PASCAL EndAccess();
extern VOID FAR PASCAL BitBlt16(DIBENGINE far *, WORD, WORD, DIBENGINE far *, 
        WORD, WORD, WORD, WORD, DWORD, VOID *, VOID *);

extern WORD MiniVDD_AllocateGlobalMemory(DWORD Size);
extern WORD MiniVDD_FreeGlobalMemory(DWORD Selector);

