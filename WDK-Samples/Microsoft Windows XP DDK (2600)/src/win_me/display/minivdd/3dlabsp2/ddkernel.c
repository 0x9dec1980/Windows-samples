/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: ddkernel.c
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include <basedef.h>
#include <vmm.h>
#include <debug.h>
#include <vcomm.h>
#include <vxdwraps.h>
#include <vwin32.h>
#include <winerror.h>
#include "glint.h"

#ifndef DDK_DX5_BETA1
DWORD dwIRQCallback;
DWORD _cdecl vddGetIRQInfo(DWORD dwDevHandle, DDGETIRQINFO* pGetIRQ)
{
	DISPDBG(("GLINTMVD: ** In vddGetIRQInfo Context: 0x%x", dwDevHandle));
	pGetIRQ->dwFlags = IRQINFO_HANDLED;
	return 0;
}
DWORD _cdecl vddIsOurIRQ(DWORD dwDevHandle)
{
	DISPDBG(("GLINTMVD: ** In vddIsOurIRQ Context: 0x%x", dwDevHandle));
	return 0;
}
DWORD _cdecl vddEnableIRQ(DWORD dwDevHandle, DDENABLEIRQINFO* pEnableInfo)
{

	DISPDBG(("GLINTMVD: ** In vddEnableIRQ Context: 0x%x", dwDevHandle));

	// Aparently, this code shouldnt be enabling the VBLANK at all, and its
	// just debug. However, I will remove the whole thing until someone decides
	// what to do with it, as it would break cursors! (Gary).
	return 0;
}
DWORD _cdecl vddFlipVideoPort(DWORD dwDevHandle, DDFLIPVIDEOPORTINFO* pFlipInfo)
{
	DISPDBG(("GLINTMVD: ** In vddFlipVideoPort Context: 0x%x", dwDevHandle));
	return 0;
}
DWORD _cdecl vddFlipOverlay(DWORD dwDevHandle, DDFLIPOVERLAYINFO* pFlipInfo)
{
	DISPDBG(("GLINTMVD: ** In vddFlipOverlay Context: 0x%x", dwDevHandle));
	return 0;
}
DWORD _cdecl vddGetCurrentAutoflip(DWORD dwDevHandle, DDGETAUTOFLIPININFO* pGetIn, DDGETAUTOFLIPOUTINFO* pGetOut)
{
	DISPDBG(("GLINTMVD: ** In vddGetCurrentAutoflip Context: 0x%x", dwDevHandle));
	return 0;
}

DWORD _cdecl vddGetPreviousAutoflip(DWORD dwDevHandle, DDGETAUTOFLIPININFO* pGetIn, DDGETAUTOFLIPOUTINFO* pGetOut)
{
	DISPDBG(("GLINTMVD: ** In vddGetPreviousAutoflip Context: 0x%x", dwDevHandle));
	return 0;
}

DWORD _cdecl vddLock(DWORD dwDevHandle, DDLOCKININFO* pLockIn, DDLOCKOUTINFO* pLockOut)
{
	DISPDBG(("GLINTMVD: ** In vddLock Context: 0x%x", dwDevHandle));
	return 0;
}

DWORD _cdecl vddSetState(DWORD dwDevHandle, DDSTATEININFO* pStateIn, DDSTATEOUTINFO* pStateOut)
{
	DISPDBG(("GLINTMVD: ** In vddSetState Context: 0x%x", dwDevHandle));
	return 0;
}

DWORD _cdecl vddGetFieldPolarity(DWORD dwDevHandle, DDPOLARITYININFO* pPolarityIn, DDPOLARITYOUTINFO* pPolarityOut)
{
	DISPDBG(("GLINTMVD: ** In vddGetFieldPolarity Context: 0x%x", dwDevHandle));
	return 0;
}

DWORD _cdecl vddSkipNextField(DWORD dwDevHandle, DDSKIPINFO* pSkipInfo)
{
	DISPDBG(("GLINTMVD: ** In vddSkipNextField Context: 0x%x", dwDevHandle));
	return 0;
}

#endif DDK_DX5_BETA1

