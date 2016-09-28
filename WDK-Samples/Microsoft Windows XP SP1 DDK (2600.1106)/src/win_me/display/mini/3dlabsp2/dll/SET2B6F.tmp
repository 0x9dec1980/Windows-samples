/******************************Module*Header**********************************\
*
*       **************************************************
*       * Permedia 2: Direct Draw/Direct 3D  SAMPLE CODE *
*       **************************************************
*
* Module Name: debug.h
*
*       Contains prototypes and definitions for debug functions.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef _DLL_DEBUG_H_
#define _DLL_DEBUG_H_

#ifdef DEBUG

#define DISPDBG(arg) DebugPrint arg
#define RIP(x) { DebugPrint(-1000, x); _asm { int 1 };}
#define ASSERTDD(x, y) if (!(x)) RIP (y)
#define ASSERTD3D(x, y) ASSERTDD(x, y)

void __cdecl DebugPrint(DWORD DebugLevelPrint,  LPSTR szFormat, ... );

extern DWORD DebugFilter;

#define DEBUG_FILTER_D3D   0x000000FF
#define DEBUG_FILTER_DD    0x0000FF00
#define DEBUG_FILTER_GDI   0x00FF0000

#define DBG_COMPONENT(arg, component)        \
{       DebugPrintFilter = component;        \
        DebugPrint arg ;                     \
        DebugPrintFilter = 0;                \
}

#define DBG_D3D(arg)        if (DebugFilter & DEBUG_FILTER_D3D) DISPDBG(arg)
#define DBG_DD(arg)         if (DebugFilter & DEBUG_FILTER_DD) DISPDBG(arg)
#define DBG_GDI(arg)        if (DebugFilter & DEBUG_FILTER_GDI) DISPDBG(arg)


#else /* #ifdef DEBUG */

#define DISPDBG(arg)
#define RIP(x)
#define ASSERTDD(x, y)
#define ASSERTD3D(x, y)
#define DBG_D3D(arg)        
#define DBG_DD(arg)        
#define DBG_GDI(arg)      

#endif /* #ifdef DEBUG */

#endif /* #ifndef _DLL_DEBUG_H_ */

