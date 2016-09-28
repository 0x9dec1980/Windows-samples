/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: glddif.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

//
// GLDDIF.H
//
// Interface to Direct Draw extension for GLINT family

// Interface Initialisation
#ifndef __GLDDIF
#define __GLDDIF

// DMA extensions
typedef struct _GlintDMARequest
{
    DWORD       dwContext;
    DWORD       dwSize;
    DWORD       dwPhys;
    DWORD       dwEvent;
} GLDMAREQ, *LPGLDMAREQ;

typedef struct _GlintDMABuffer
{
    DWORD       dwBuffSize;
    DWORD       dwBuffPhys;
    ULONG_PTR   dwBuffVirt;
    DWORD       dwSubBuff;
} GLDMABUFF, *LPGLDMABUFF;

#endif // __GLDDIF

