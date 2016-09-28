/******************************Module*Header**********************************\
*
*       **************************************************
*       * Permedia 2: Direct Draw/Direct 3D  SAMPLE CODE *
*       **************************************************
*
* Module Name: d3dalloc.h
*
*       Contains Memory allocation functions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef _DLL_D3DALLOC_H_
#define _DLL_D3DALLOC_H_

extern  HANDLE                      hSharedHeap;
#define D3DSHAREDALLOC(cbSize)      HeapAlloc (hSharedHeap, HEAP_ZERO_MEMORY, (cbSize))
#define D3DSHAREDFREE(lpPtr)        HeapFree (hSharedHeap, 0, (LPVOID)(lpPtr))

#define D3DMALLOCZ(cbSize)          D3DSHAREDALLOC(cbSize)
#define D3DFREE(lpPtr)              D3DSHAREDFREE(lpPtr)


#endif //#ifndef _DLL_D3DALLOC_H_
