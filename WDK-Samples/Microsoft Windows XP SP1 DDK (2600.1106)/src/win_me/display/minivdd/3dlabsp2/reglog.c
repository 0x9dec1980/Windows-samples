/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: reglog.c
 *  Content:    write current driver info into registry
 *      \HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\Display      
 *      Registry Logging Contents:
 *              ChipType        string
 *              DACType         string
 *              VideoMemorySize ulong
 *              MMIO            bool
 *              LinearAccess    bool
 *              HWCursor        bool *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include <basedef.h>
#include <vmmreg.h>
#include <vmm.h>
#include <vwin32.h>
#include "glint.h"
#include <pci.h>
#include "3dver.h"
#include <regdef.h>

#define MMIO_BIT        0x01
#define LINEAR_BIT      0x02
#define HWCURSOR_BIT    0x04

extern char szDefaultKey[], szInfoKey[];

static char szchiptype[]  = "ChipType";
static char szchipfamily[]= "ChipFamily";
static char szboardtype[] = "BoardType";
static char szrevision[]  = "Revision";
static char szdactype[]   = "DACType";
static char szvidmem[]    = "VideoMemory";
static char szlocalmem[]  = "LocalMemory";
static char szmmio[]      = "MMIO";
static char szlinear[]    = "Linear";
static char szhwcursor[]  = "HWCursor";
static char szunknown[]   = "Unknown";

#define SIZE_INFOSTRING 64

void RegistryLog(PDEVICETABLE pDev)
{
    CONFIGRET   result;
    BYTE        yes=1;
    BYTE        no=0;
    char        *ChipType, *DACType, *BoardType, *FamilyType;
    char        InfoString[SIZE_INFOSTRING];
    DWORD       Size;

    if (pDev->DevNode == 0) 
    {
        DISPDBG(("INVALID DEVNODE Registry Not Written"));
        return;
    }

    switch (pDev->ChipID) 
    {
	case PERMEDIA2_ID:
	    ChipType = "PERMEDIA 2";
	    BoardType = "3Dlabs PERMEDIA 2 Reference Board";
	break;

	case TIPERMEDIA2_ID:
	    ChipType = "PERMEDIA 2.";
	    BoardType = "3Dlabs PERMEDIA 2 Reference Board";
	break;

	default:
	    ChipType = szunknown;
        break;
    }

    if (pDev->FamilyID == PERMEDIA_ID) 
    {
	char	SpeedStr[8];
	long	Speed = pDev->ClkSpeed / 10000;

	FamilyType = "PERMEDIA";

	// First copy ChipInfo to a buffer
	strcpy(InfoString, ChipType);

	// Set current speed to Spaces
	strcpy(SpeedStr, "    MHz");

	// Start writing speed here
	ChipType = SpeedStr + 3;

	if (Speed > 999) Speed = 999;

	while (Speed) 
        {
	    *ChipType = (Speed % 10) + '0';
	    Speed /= 10;
	    ChipType--;
	}

	// Append speed onto chipID 
	strcat(InfoString, ChipType);
	ChipType = InfoString;
    }
    else 
    {
	FamilyType = "GLINT";
    }
    //  write the Chiptype
    DISPDBG(("ChipType:   %s  Revision: %d", ChipType, (pDev->RevisionID & 255)));
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szchiptype,
				    REG_SZ, 
                                    ChipType, 
                                    strlen(ChipType), 
                                    CM_REGISTRY_SOFTWARE);
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szchipfamily,
				    REG_SZ, 
                                    FamilyType, 
                                    strlen(FamilyType), 
                                    CM_REGISTRY_SOFTWARE);
    if (result != CR_SUCCESS) 
    {
        return;
    }

    //  write the chip rev. This is just a Byte.
    InfoString[0] = (char)(pDev->RevisionID & 255) + '0';	// Rev 1 typically
    InfoString[1] = 0;
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szrevision,
				    REG_SZ, 
                                    InfoString, 
                                    1, 
                                    CM_REGISTRY_SOFTWARE);

    // Write the board type. This requires a possible vendor specific override from the
    // default key  Check for this first
    Size = SIZE_INFOSTRING;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
                                szboardtype,
			        REG_SZ, 
                                InfoString, 
                                &Size,
			        CM_REGISTRY_SOFTWARE) == CR_SUCCESS) 
    {
        // We have a vendor override for board type
        InfoString[SIZE_INFOSTRING-1] = 0;
        BoardType = InfoString;
    }

    // Write the value;
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szboardtype,
				    REG_SZ, 
                                    BoardType, 
                                    Size, 
                                    CM_REGISTRY_SOFTWARE);

    switch (pDev->ActualDacId) {
        case TVP4020_RAMDAC:
            DACType = "TVP4020";
        break;
        default:
            DACType = szunknown;
        break;
    }

    // write the DAC type
    DISPDBG(("DACType:    %s", DACType));
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szdactype,
				    REG_SZ, 
                                    DACType, 
                                    strlen(DACType), 
                                    CM_REGISTRY_SOFTWARE);

    //  write the amount of vidmemory
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szvidmem,
				    REG_DWORD, 
                                    &(pDev->dwFrameBufferSize), 
                                    sizeof(ULONG), 
                                    CM_REGISTRY_SOFTWARE);

    // write the flags (MMIO, LINEAR, HWCURSOR)
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szmmio,
				    REG_BINARY, 
                                    &yes, 
                                    1, 
                                    CM_REGISTRY_SOFTWARE);
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szlinear,
				    REG_BINARY, 
                                    &yes, 
                                    1, 
                                    CM_REGISTRY_SOFTWARE);
    result=CM_Write_Registry_Value( pDev->DevNode, 
                                    szInfoKey, 
                                    szhwcursor,
				    REG_BINARY, 
                                    &yes, 
                                    1, 
                                    CM_REGISTRY_SOFTWARE);
}
