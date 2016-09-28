/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: glintdma.h
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#pragma pack(2)

typedef struct _GlintDMARequest
{
    DWORD       dwContext;
    DWORD       dwSize;
    DWORD       dwPhys;
} GLDMAREQ, *LPGLDMAREQ;

typedef struct _GlintDMAAddress
{
    DWORD       dwBuffPhys;
    DWORD       dwBuffVirt;
} GLDMAADDR;

typedef struct _GlintDMABuffers
{
    DWORD       dwNBuffers;
    DWORD       dwBuffSize;
    GLDMAADDR   Addr[8];
	DWORD		dwSubBuff;
} GLDMABUFF, *LPGLDMABUFF;

#pragma pack()
