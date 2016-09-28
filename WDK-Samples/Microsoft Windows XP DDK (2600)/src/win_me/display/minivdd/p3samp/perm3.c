
/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 3: miniVDD SAMPLE CODE     *
 *                           ***************************************
 *
 * Module Name: video.c
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include <basedef.h>
#include <vmm.h>
#include <vmmreg.h>
#include <vwin32.h>
#include <winerror.h>
#include <pci.h>
#include "perm3.h"
#include <regdef.h>


//
// Retrieve the register initialization table from the ROM
//

BOOLEAN 
ReadROMInfo(
    PDEVICETABLE            pDev)
{
    DWORD                   DevNode;
    ULONG                   ulRet;
    ULONG                   ulRomSize;
    LPBYTE                  pRomBuf = NULL;

    DevNode = pDev->DevNode;

    do {

        //
        // Get the size of ROM on the card
        //

        ulRet = CM_Call_Enumerator_Function(
                    DevNode,
                    PCI_ENUM_FUNC_GET_ROM_DATA_SIZE,
                    0, 
                    &ulRomSize, 
                    sizeof(ulRomSize), 
                    0);
        if (ulRet != CR_SUCCESS) {
            break;
        }

        //
        // If the ROM size is not correct
        //

        if (ulRomSize < ROM_MAPPED_LENGTH) {
            ulRet = CR_INVALID_DATA;
            break;
        }

        //
        // Allocate buf to copy the ROM image 
        //

        pRomBuf = _HeapAllocate(ulRomSize, 0); // Non-paged pool
        if (! pRomBuf) {
            ulRet = CR_OUT_OF_MEMORY;
            break;
        }

        //
        // Ask the PCI bus driver to copy the actual data back from ROM
        // Note : Win95 will fail this call
        //

        ulRet = CM_Call_Enumerator_Function(
                    DevNode,
                    PCI_ENUM_FUNC_GET_ROM_DATA,
                    0, 
                    pRomBuf, 
                    ulRomSize, 
                    0);
    
    } while (FALSE);

    //
    // Copy the initialization table
    //

    if (ulRet == CR_SUCCESS) {

        //
        // VGA disabled so the Video BIOS has not had chance to run
        // through the P3 register initialization table in the exansion
        // ROM - we'll take a copy of this table now so that we can
        // run through the initialization ourselves, later on
        //

        CopyROMInitializationTable(pDev, pRomBuf);
    
    } else {
        
        //
        // No initialization table, but P2/P3 really needs one in order to come 
        // out of sleep mode correctly. The initialisation table will always
        // be invalid for a Dec Maui because it doesn't have a P2 BIOS at the
        // expected ROM offset
        // Note : Win95 will take this path 
        //

        GenerateInitializationTable(pDev);
    }

    //
    // Free the temporary buffer
    //

    if (pRomBuf) {
        _HeapFree(pRomBuf, 0);
    }

    return (TRUE);
}


//
// Read the chip clock speed (in MHz) from the 
// Video ROM BIOS (offset 0xA in the BIOS)
//

LONG 
ReadChipClockSpeedFromROM(
    PDEVICETABLE            pDev,
    ULONG                  *pChipClkSpeed)
{
    //
    // This involves changing the aperture 2 register so aperture better 
    // be completely idle or we could be in trouble; fortunately we only 
    // call this function during a mode change and expect aperture 2 (the 
    // FrameBuffer) to be idle
    //

    UCHAR                  *p = (UCHAR *)pDev->pFrameBuffer;
    UCHAR                   clkSpeed;
    int                     retVal = -1;
    ULONG                   Default;

    READ_GLINT_CTRL_REG(ApertureTwo, Default);
    
    //
    // R/W via aperture 2 actually go to ROM
    //

    LOAD_GLINT_CTRL_REG(ApertureTwo, Default | 0x200); 

    //
    // If we have a valid ROM then read the clock pseed
    //

    if ((*((USHORT *)p)) == 0xAA55) {
   
        //
        // Get the clock speed
        //

        clkSpeed = p[0xA];

        //
        // Some boards, such as ones by Creative, have been know to set 
        // offset xA to random-ish values. Creative use the following test
        // to determine whether they have a sensible value, so that is what
        // we will use.
        //

        if (clkSpeed > 50) {
       
            *pChipClkSpeed = clkSpeed;
            *pChipClkSpeed *= (1000*1000); 

            retVal = 0;
        }

        DISPDBG((
            "ROM clk speed value 0x%x\n", (ULONG)(p[0xA])));

    } else {
   
        DISPDBG((
            "Bad BIOS ROM header 0x%x\n", (ULONG)(*((USHORT *)p))));
    }

    //
    // Reset the aperture 2 control register to the original value
    //

    LOAD_GLINT_CTRL_REG(ApertureTwo, Default);

    return (retVal);
}


//
// This routine sets a specified portion of the color lookup table settings.
//

ULONG
Perm3SetColorLookup(
    PDEVICETABLE            pDev,
    PVIDEO_CLUT             ClutBuffer,
    ULONG                   ClutBufferSize,
    BOOLEAN                 ForceRAMDACWrite,
    BOOLEAN                 UpdateCache)
{
    USHORT                  i, j;
    PVIDEO_CLUT             LUTCachePtr; 
    P3RDRAMDAC             *pP3RDRegs; 
#ifdef DAC_DBG
    DWORD                   ulValue;
#endif

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if ( (ClutBufferSize < (sizeof(VIDEO_CLUT) - sizeof(ULONG))) ||
         (ClutBufferSize < (sizeof(VIDEO_CLUT) +
                     (sizeof(ULONG) * (ClutBuffer->NumEntries - 1))))) {

        DISPDBG((
            "Perm3SetColorLookup: insufficient buffer (was %d, min %d)\n",
            ClutBufferSize,
            (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * (ClutBuffer->NumEntries - 1)))));
        
        return (ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // Check to see if the parameters are valid.
    //

    if ( (ClutBuffer->NumEntries == 0) ||
         (ClutBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER) ||
         (ClutBuffer->FirstEntry + ClutBuffer->NumEntries >
                                     VIDEO_MAX_COLOR_REGISTER + 1)) {

        DISPDBG(("Perm3SetColorLookup: invalid parameter\n"));
        return (ERROR_INVALID_PARAMETER);

    }

    //
    // Set CLUT registers directly on the hardware. For the moment we'll switch
    // between different RAMDACs. It may turn out that we can employ some
    // uniform way of doing this in the future.
    //
    
    pP3RDRegs   = (P3RDRAMDAC *)&pDev->pRegisters->Glint.ExtVCReg;
    LUTCachePtr = &(pDev->LUTCache.LUTCache);

    //
    // RAMDAC Initialisation phase
    //

    switch (pDev->DacId) {

        case P3RD_RAMDAC:
            break;

        default:
            return (ERROR_DEV_NOT_EXIST);
    }

    //
    // RAMDAC Programming phase
    //

    for (i = 0, j = ClutBuffer->FirstEntry; 
         i < ClutBuffer->NumEntries; 
         i++, j++)  {

        //
        // Update the RAMDAC entry if it has changed or if we have been 
        // told to overwrite it.
        //

        if (ForceRAMDACWrite || 
                ( LUTCachePtr->LookupTable[j].RgbLong != 
                  ClutBuffer->LookupTable[i].RgbLong)  ) {
       
            switch (pDev->DacId) {
           
                case P3RD_RAMDAC:
                    P3RD_LOAD_PALETTE_INDEX (
                        j,
                        ClutBuffer->LookupTable[i].RgbArray.Red,
                        ClutBuffer->LookupTable[i].RgbArray.Green,
                        ClutBuffer->LookupTable[i].RgbArray.Blue );
                    break;
            }

        }

        //
        // Update the cache, if instructed to do so
        //

        if (UpdateCache) {
    
            LUTCachePtr->LookupTable[j].RgbLong = 
                ClutBuffer->LookupTable[i].RgbLong;
        }
    }

#ifdef DAC_DBG
    P3RD_PALETTE_START_RD(0);
    for (i = 0; i < 256; i++) {

        UCHAR red, green, blue;
        
        P3RD_READ_PALETTE(red, green, blue);
        DISPDBG(("Palette (%d) : r=%d, g=%d, b=%d\n", i, red, green, blue));
    }
    
    P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
    DISPDBG(("P3RD_MISC_CONTROL = %x\n", ulValue));

    P3RD_READ_INDEX_REG(P3RD_COLOR_FORMAT, ulValue);
    DISPDBG(("P3RD_COLOR_FORMAT = %x\n", ulValue));

    P3RD_READ_INDEX_REG(P3RD_PIXEL_SIZE, ulValue);
    DISPDBG(("P3RD_PIXEL_SIZE = %x\n", ulValue));

    P3RD_READ_INDEX_REG(P3RD_DAC_CONTROL, ulValue);
    DISPDBG(("P3RD_DAC_CONTROL = %x\n", ulValue));

    P3RD_READ_PIXEL_READMASK(ulValue);
    DISPDBG(("P3RD_PIXEL_MASK = %x\n", ulValue));
#endif

    return (NO_ERROR);

} // End of Perm3SetColorLookup()

