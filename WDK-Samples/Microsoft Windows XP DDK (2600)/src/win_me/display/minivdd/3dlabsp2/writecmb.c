/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: writecmb.c
 *
 *	This module contains routines that will assist in setting up the 
 *	DMA buffers as being write-combine regions. The code here relies
 *	on some assembler files to get at the instructions needed to change
 *	the MTRR registers.
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


#include <basedef.h>
#include <vmm.h>
#include <vmmreg.h>
#include <vwin32.h>
#include "glint.h"
#include <regdef.h>


/********************************************************************************/

// Memory types:

#define	MEM_UC	0
#define	MEM_WC	1
#define	MEM_WT	4
#define	MEM_WP	5
#define	MEM_WB	6


#define	FIRST_MTRR_BASE		0x200
#define LAST_MTRR_BASE		0x20E
#define	FIRST_MTRR_MASK		0x201
#define	LAST_MTRR_MASK		0x20F
#define	MTRR_DEF_TYPE		0x2FF
#define MTRR_CAP_REG		0xFE
#define	MTRRS_AVAILABLE		12
#define	WC_SUPPORTED		10
#define	MTRR_64K_FIXED		0x250


#define	SET_LOW_FIELD(value, fieldSize, fieldStart)	\
	(value << fieldStart)

#define	SET_HI_FIELD(value, fieldSize, fieldStart)	\
	(value >> (32 -fieldStart))	

#define	MTRR_FIXED_ENABLE_BIT	10
#define	MTRR_ENABLE_BIT			11


// Enable the fixed MTRR registers if they are not already enabled.

static void
enableFixedMTRRs()
{
	unsigned long loMSR;
	unsigned long hiMSR;

	DISPDBG(("enableFixedMTRRs called"));

	// See if the MTRRS are enabled.  If they are not, then
	// we should make sure that the enable flags in each of
	// the variable masks is zeroed.  If we don't do this and
	// we enable the variable MTRRs, then all all hell might
	// break loose if they haven't been initialised.

	ReadMSR(MTRR_DEF_TYPE, &loMSR, &hiMSR);

	if (!(loMSR & (1 << MTRR_FIXED_ENABLE_BIT))) {

		DISPDBG(("Fixed MTRRs, not enabled, enabling them"));

		// Now enable the MTRR registers.

		loMSR |= (1 << MTRR_FIXED_ENABLE_BIT);
		WriteMSR(MTRR_DEF_TYPE, &loMSR, &hiMSR);
	}
	DISPDBG(("enableFixedMTRRs done"));
}

// Check to see if the processor will support write combining

BOOL
TestWCCapability( void )
{
	unsigned long loMSR;
	unsigned long hiMSR;
	int		 cpuFeature;

	DISPDBG(("TestWCCapability called"));

	if( HasCPUID() )
	{
		cpuFeature = GetCPUID();

		// If bit 12 is set of the feature register (EDX after the 
		// CPUID instruction is executed) then MTRRs are supported.
		if (cpuFeature & (1 << MTRRS_AVAILABLE)) {
			// Now look to see if write combining is supported.
			ReadMSR(MTRR_CAP_REG, &loMSR, &hiMSR);

			// Should be this on a PentiumPro
			if (loMSR == 0x508) {
				DISPDBG(("TestWCCapability TRUE"));
				return (TRUE);
			}
		}
	}

	DISPDBG(("testWCCapability FALSE"));
	return(FALSE);
}


static void
dumpFixedMTRR()
{
	unsigned long loVarMSR;
	unsigned long hiVarMSR;
	unsigned long loCAPMSR;
	unsigned long hiCAPMSR;
	unsigned long loDEFMSR;
	unsigned long hiDEFMSR;

	DISPDBG(("dumpFixedMTRR called"));

	// Suck the current set of tables out.

	ReadMSR(MTRR_DEF_TYPE, &loDEFMSR, &hiDEFMSR);
	ReadMSR(MTRR_CAP_REG,  &loCAPMSR, &hiCAPMSR);

	DISPDBG(("DefType  = %08x:%08x\n", hiDEFMSR, loDEFMSR));
	DISPDBG(("CapReg   = %08x:%08x\n", hiCAPMSR, loCAPMSR));

	ReadMSR(MTRR_64K_FIXED, &loVarMSR, &hiVarMSR);
		
	DISPDBG(("MTRR Reg = %08x:%08x\n", hiVarMSR, loVarMSR));
}


BOOL
SetFixedMTRRToWC( unsigned long baseAddr, unsigned long nPages )
{
	unsigned long mtrrPhysHigh = 0;
	unsigned long mtrrPhysLow = 0;
	unsigned long slot, slotBase, slotCount, *pMTRR;

	DISPDBG(("SetFixedMTRRToWC (%08x, %08x)", baseAddr, nPages ));

	dumpFixedMTRR();

	if( baseAddr & 0xffff )
		return( FALSE );

	slotCount = nPages >> 4;

	if( !slotCount )
	{
		DISPDBG(( "Memory range too small to set to WC" ));
		return( FALSE );
	}

	slotBase = baseAddr >> 16;

	enableFixedMTRRs();

	// Read current state

	ReadMSR(MTRR_64K_FIXED, &mtrrPhysLow, &mtrrPhysHigh);

	for( slot = slotBase; slot < ( slotBase + slotCount ); slot++ )
	{
		unsigned long copySlot, origType;

		// Determine which dword of the 64 bit register to modify

		copySlot = slot;

		if( copySlot > 3 )
		{
			copySlot -= 4;
			pMTRR = &mtrrPhysHigh;
		}
		else
		{
			pMTRR = &mtrrPhysLow;
		}

		origType = ( *pMTRR >> ( copySlot * 8 )) & 0xff;

		if( origType != MEM_WB )
		{
			DISPDBG(( "Original memory type not WB, giving up" ));
			return( FALSE );
		}

		// Mask out original value and set field to write-combine type

		*pMTRR &= ~(0xff << ( copySlot * 8 ));
		*pMTRR |= MEM_WC << ( copySlot * 8 );
	}

	// Write back new state

	WriteMSR(MTRR_64K_FIXED, &mtrrPhysLow, &mtrrPhysHigh);

	dumpFixedMTRR();

	DISPDBG(("SetFixedMTRRToWC returning"));

	return (TRUE);
}

/****************************************************************************/


static void
dumpVariableMTRRs()
{
	unsigned long loVarMSR[16];
	unsigned long hiVarMSR[16];
	unsigned long loCAPMSR;
	unsigned long hiCAPMSR;
	unsigned long loDEFMSR;
	unsigned long hiDEFMSR;
	int i, j;

	DISPDBG(("dumpVariableMTRRs called"));

	// Suck the current set of tables out.

	ReadMSR(MTRR_DEF_TYPE, &loDEFMSR, &hiDEFMSR);
	ReadMSR(MTRR_CAP_REG,  &loCAPMSR, &hiCAPMSR);

	for (j = 0, i = FIRST_MTRR_BASE; i <= LAST_MTRR_BASE; i+= 2, j+= 2)
	{
		ReadMSR(i, &loVarMSR[j], &hiVarMSR[j]);			// Base
		ReadMSR((i+1), &loVarMSR[j+1], &hiVarMSR[j+1]);	// Mask
	}

	DISPDBG(("DefType  = %08x:%08x", hiDEFMSR, loDEFMSR));
	DISPDBG(("CapReg   = %08x:%08x", hiCAPMSR, loCAPMSR));

	for (i = 0; i < 16; i++)
	{
		DISPDBG(("MTRR Reg = %08x:%08x", hiVarMSR[i], loVarMSR[i]));
	}
}

// Enable the variable MTRR registers if they are not already enabled.

static void
enableVariableMTRR()
{
	unsigned long loMSR;
	unsigned long hiMSR;
	unsigned long mtrrMask;

	DISPDBG(("enableVariableMTRR called"));

	// See if the MTRRS are enabled.  If they are not, then
	// we should make sure that the enable flags in each of
	// the variable masks is zeroed.  If we don't do this and
	// we enable the variable MTRRs, then all all hell might
	// break loose if they haven't been initialised.

	ReadMSR(MTRR_DEF_TYPE, &loMSR, &hiMSR);

	if (!(loMSR & (1 << MTRR_ENABLE_BIT))) {

		DISPDBG(("MTRRs, not enabled, enabling them"));

		// Not enabled, so disable all the Variable MTRRs before
		// enabling this.

		for (mtrrMask = FIRST_MTRR_MASK; mtrrMask <= LAST_MTRR_MASK; mtrrMask += 2)
		{
			DISPDBG(("Disabling MTRR(%08x)", mtrrMask));

			ReadMSR(mtrrMask, &loMSR, &hiMSR);
			loMSR &= ~(1 << MTRR_ENABLE_BIT);
			WriteMSR(mtrrMask, &loMSR, &hiMSR);
		}

		// Now enable the MTRR registers.

		ReadMSR(MTRR_DEF_TYPE, &loMSR, &hiMSR);
		loMSR |= (1 << MTRR_ENABLE_BIT);
		WriteMSR(MTRR_DEF_TYPE, &loMSR, &hiMSR);
	}

	DISPDBG(("enableVariableMTRR done"));
}

int
SetVariableMTRRToWC( unsigned long pAddress, unsigned long pSize )
{
	int	i;
	unsigned long size;
	unsigned long maxPAddress;
	int	n;
	unsigned long mtrrBase;
	long mtrrMask;				// Needs to be long for a right shift op.
	unsigned long mtrrMaskHigh = 0;
	unsigned long mtrrMaskLow = 0;
	unsigned long mtrrPhysHigh = 0;
	unsigned long mtrrPhysLow = 0;

	unsigned long freeMtrrBase;
	unsigned long freeMtrrMask;
	unsigned long lowMSR;
	unsigned long hiMSR;

	DISPDBG(("SetVariableMTRRToWC (%08x, %08x)", pAddress, pSize ));

	dumpVariableMTRRs();

	// Check the memory parameters specified.  The minimum size is 4K as is the 
	// minimum alignment.  For areas of memory that are larger than 4k, they have
	// to be of size 2^^N, where N > 12,  They must also be aligned on a boundary
	// of 2^^N.

	size = pSize;
	for (n = 0; n < 32; n++)
	{
		size >>= 1;
		if (size == 0)
			break;
	}

	// If the size is less than 4k, return fail.

	if (n < 12)	
		return (FALSE);

	// Check that the size is 2^^n

	if (pSize != (1UL << n))
		return (FALSE);

	// Check that the alignment is a multiple of the size.

	if ((pAddress % pSize) != 0)
		return (FALSE);

	// Work out what the Base and Mask flags are.  The base is simply the physical
	// base with the bottom 12 bits truncated.

	mtrrBase = pAddress >> 12;
	mtrrBase &= 0x00ffffff;

	// Evaluate the mask, such that if the maximum address in the range and the 
	// minimum address in the range, are each anded with the mask, the same result
	// is produces.

	mtrrMask = 0xffffffff;
	maxPAddress = pAddress + pSize -1;

	for (i = 0; i < 32; i++) {
		if ((pAddress & mtrrMask) == (maxPAddress & mtrrMask))
			break;
		mtrrMask -= (1 << i);
	}

	// Truncate the bottom 12 bits of the mask.

	mtrrMask >>= 12;
	mtrrMask &= 0x00ffffff;

	// Mask the field - it's 24 bits long in the register.

	mtrrMaskLow  |= SET_LOW_FIELD(mtrrMask, 24, 12);
	mtrrMaskHigh |= SET_HI_FIELD(mtrrMask, 24, 12);

	// Enable the MTRR

	mtrrMaskLow  |= SET_LOW_FIELD(1, 1, 11);
	
	// Set the physical address Base.

	mtrrPhysLow  |= SET_LOW_FIELD(mtrrBase, 24, 12);
	mtrrPhysHigh |= SET_HI_FIELD(mtrrBase, 24, 12);

	// Set the physical address type

	mtrrPhysLow  |= SET_LOW_FIELD( MEM_WC, 8, 0);

	DISPDBG(("MTRR settings (mask): %08x:%08x", mtrrMaskHigh, mtrrMaskLow));
	DISPDBG(("MTRR settings (mask): %08x:%08x", mtrrPhysHigh, mtrrPhysLow));

	enableVariableMTRR();

	// We have the values that we need to write, now we have to locate
	// the next free table entry to use.

	// Force the first slot to be used.

	freeMtrrBase = 0;
	for (i = 0, freeMtrrMask = 0x201; i < 7; i++, freeMtrrMask += 2)
	{
		ReadMSR (freeMtrrMask, &lowMSR, &hiMSR);

		if ((lowMSR & (1 << MTRR_ENABLE_BIT)) == 0)
		{
			freeMtrrBase = freeMtrrMask-1;
			break;
		}
	}

	if (freeMtrrBase == 0)
		return(FALSE);

	DISPDBG(("MTRR found a free slot (%08x)", freeMtrrBase));

	// If we have found one, then update the two registers with the 
	// new data.

	WriteMSR(freeMtrrBase, &mtrrPhysLow, &mtrrPhysHigh);
	WriteMSR(freeMtrrMask, &mtrrMaskLow, &mtrrMaskHigh);

	dumpVariableMTRRs();

	DISPDBG(("SetVariableMTRRToWC returning with success"));

	return (TRUE);
}
