/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: init.c
 *
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

#define FB_PROBE_SIZE		0x1000
#define FB_PROBE_PATTERN	0x55aa33cc
#define LB_PROBE_SIZE		0x10000
#define LB_PROBE_PATTERN	0x55aa33cc

//
// magic bits in the FBMemoryCtl and LBMemoryCtl registers
//
#define LBCTL_RAS_CAS_LOW_MASK      (3 << 3)
#define LBCTL_RAS_CAS_LOW_2_CLK     (0 << 3)
#define LBCTL_RAS_CAS_LOW_3_CLK     (1 << 3)
#define LBCTL_RAS_CAS_LOW_4_CLK     (2 << 3)
#define LBCTL_RAS_CAS_LOW_5_CLK     (3 << 3)

#define LBCTL_RAS_PRECHARGE_MASK    (3 << 5)
#define LBCTL_RAS_PRECHARGE_2_CLK   (0 << 5)
#define LBCTL_RAS_PRECHARGE_3_CLK   (1 << 5)
#define LBCTL_RAS_PRECHARGE_4_CLK   (2 << 5)
#define LBCTL_RAS_PRECHARGE_5_CLK   (3 << 5)

#define LBCTL_CAS_LOW_MASK          (3 << 7)
#define LBCTL_CAS_LOW_1_CLK         (0 << 7)
#define LBCTL_CAS_LOW_2_CLK         (1 << 7)
#define LBCTL_CAS_LOW_3_CLK         (2 << 7)
#define LBCTL_CAS_LOW_4_CLK         (3 << 7)

#define FBCTL_RAS_CAS_LOW_MASK      (3 << 0)
#define FBCTL_RAS_CAS_LOW_2_CLK     (0 << 0)
#define FBCTL_RAS_CAS_LOW_3_CLK     (1 << 0)
#define FBCTL_RAS_CAS_LOW_4_CLK     (2 << 0)
#define FBCTL_RAS_CAS_LOW_5_CLK     (3 << 0)

#define FBCTL_RAS_PRECHARGE_MASK    (3 << 2)
#define FBCTL_RAS_PRECHARGE_2_CLK   (0 << 2)
#define FBCTL_RAS_PRECHARGE_3_CLK   (1 << 2)
#define FBCTL_RAS_PRECHARGE_4_CLK   (2 << 2)
#define FBCTL_RAS_PRECHARGE_5_CLK   (3 << 2)

#define FBCTL_CAS_LOW_MASK          (3 << 4)
#define FBCTL_CAS_LOW_1_CLK         (0 << 4)
#define FBCTL_CAS_LOW_2_CLK         (1 << 4)
#define FBCTL_CAS_LOW_3_CLK         (2 << 4)
#define FBCTL_CAS_LOW_4_CLK         (3 << 4)

//
// DisconnectControl bits
//
#define DISCONNECT_INPUT_FIFO_ENABLE    0x1
#define DISCONNECT_OUTPUT_FIFO_ENABLE   0x2
#define DISCONNECT_INOUT_ENABLE         (DISCONNECT_INPUT_FIFO_ENABLE | \
                                         DISCONNECT_OUTPUT_FIFO_ENABLE)

#define OFFSET_GLINT_REG(reg) (WORD) &(((PGLREG)0)->reg)


/****************************************************************************
    Global Data
****************************************************************************/
#pragma CM_PAGEABLE_DATA

DEVICETABLE		DevTable[MAX_GLINT_DEVICES];

DWORD           PrimaryDevNode = 0;
PDEVICETABLE    pDevWithVGAEnabled = 0;
DWORD			DMABuffersAllocated = 0;
DMAINFO			DMABuffers;

DWORD           InitialisedIntDMA = 0;

// Interrupt information
VID				IRQDesc;

// Board specific information
char	szDefaultKey[] 		    = "DEFAULT";
char	szInfoKey[] 		    = "INFO";
char	szVTGSerialClock[] 	    = "VTGSerialClock";
char	szLBMemoryCtl[] 	    = "LBMemoryCtl";
char	szLBMemoryCtlMask[]     = "LBMemoryCtlMask";
char	szLBMemoryEDO[] 	    = "LBMemoryEDO";
char	szFBMemoryCtl[] 	    = "FBMemoryCtl";
char	szFBMemoryCtlMask[]     = "FBMemoryCtlMask";
char	szFBModeSel[]	 	    = "FBModeSel";
char	szFBModeSelMask[]	    = "FBModeSelMask";
char    szVSConfiguration[]     = "VSConfiguration";
char    szMemControl[]          = "MemControl";
char    szBootAddress[]         = "BootAddress";
char    szMemConfig[]           = "MemConfig";
char    szFifoControl[]         = "FifoControl";
char	szRefClk[]		 	    = "ReferenceClock";
char	szSysClk[]	            = "SystemClock";
char	szVGAHost[]	            = "3DlabsVGAHost";

// General configuration information
char	szInformationKeyName[]  = "Configuration";
char	szDisableIRQ[]          = "DisableIRQ";
char	szEnableDMA[]           = "EnableDMA";
char	szInterceptGDI[]        = "InterceptGDI";
char	szOffScrnBitMaps[]      = "OffScreenBitMaps";
char	szDMABufferCount[]      = "DMABufferCount";
char	szDMABufferSize[]       = "DMABufferSize";
char	szFontCacheSize[]       = "FontCache";
char	szPrefRefresh[]         = "PreferredRefreshRate";

/****************************************************************************
    Functions start here
****************************************************************************/
#pragma CM_PAGEABLE_CODE	


PDEVICETABLE 
InitialiseDeviceNode(ULONG DevNode) 
{
    PDEVICETABLE pDev;

    pDev = InitialiseHardwareNode(DevTable, DevNode);
    if (!pDev)
        return (NULL);

    // We found a Glint device on the HW node on which we are running
    // Map in the glint registers. These will be accessed by the DDC code.
    pDev->pRegisters = _MapPhysToLinear(pDev->CMConfig.dMemBase[0], pDev->CMConfig.dMemLength[0], 0);
    if (pDev->pRegisters == (PREGISTERS)0xFFFFFFFF) 
    {
        DISPDBG(( "Cannot Map Glint Control Registers"));
        return (NULL);
    }

    DISPDBG(("Mapped P2 Registers. Phys  :0x%x", pDev->CMConfig.dMemBase[0])); 
    DISPDBG(("                     Virt  :0x%x", pDev->pRegisters)); 
    DISPDBG(("                     Length:0x%x", pDev->CMConfig.dMemLength[0])); 
    return(pDev);
}

PDEVICETABLE _cdecl 
InitialiseDynamicInit(ULONG DevNode)
{
    PDEVICETABLE pDev;

    // Save this for future use.
    PrimaryDevNode = DevNode;

    // First zero the pDev table. This means all pointers are null.
    memset(DevTable, 0, sizeof(DEVICETABLE) * MAX_GLINT_DEVICES);

    pDev = InitialiseDeviceNode(DevNode);

    pDevWithVGAEnabled = pDev;

    DISPDBG(("InitialiseDynamicInit"));

    InitialisePrimaryDispatchTable();

    DISPDBG(("Install Power Management callback"));
    _SHELL_HookSystemBroadcast((SYSBHOOK_CALLBACK) &ShellBroadcastCallback, (DWORD)DevTable, 0);

    return(pDev);
}


PDEVICETABLE _cdecl 
InitialiseHardware(PDEVICETABLE pDev, 
                   BOOL         Destructive)
{
    DWORD ulInterceptGDI;
    DWORD ulOffScrnBitMaps;
    ULONG ulValue;
    DWORD ulVirtualize;
    HKEY  hKey;

    // Main function to locate and initialise hardware.
    DISPDBG(("InitialiseHardware"));

    // Read the location of the general configuraion information from the registry.
    ulValue = SIZE_INFORMATION_KEY-18;
    strcpy(pDev->szInformationKey, REGKEYROOT);
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
                                szInformationKeyName,
			        REG_SZ, 
                                &(pDev->szInformationKey[9]), 
                                &ulValue,
				CM_REGISTRY_SOFTWARE) != CR_SUCCESS) 
    {
        strcat(pDev->szInformationKey, "3Dlabs");
    }

    strcat(pDev->szInformationKey, REGKEYDISPLAYSUFFIX);

    // Map in the memory regions
    if (!MapInitAndSizeMemoryRegions(pDev, pDev->pDeltaDev, Destructive)) 
    {
	DISPDBG(("MapInitAndSizeMemoryRegions Failed"));
	return(NULL);
    }

    // Put the Chip into a usable state
    if (!InitialiseChip(pDev)) 
    {
	DISPDBG(("InitialiseChip Failed"));
	return(NULL);
    }

    // Initialise DMA buffers
    if (!InitialiseDMA(pDev)) 
    {
	DISPDBG(("InitialiseDMA Failed"));
	return(NULL);
    }

    if (_RegOpenKey(HKEY_LOCAL_MACHINE, pDev->szInformationKey, &hKey) != ERROR_SUCCESS) 
    {
        DISPDBG(("Couldnt open HKLM\\%s", pDev->szInformationKey));
    }

    ulValue = 4;
    pDev->dwFlags &= ~GMVF_INTCPTGDI;	// Assume No GDI interception
    if (_RegQueryValueEx(   hKey, 
                            szInterceptGDI, 
                            NULL, 
                            NULL, 
                            &ulInterceptGDI, 
                            &ulValue) == ERROR_SUCCESS) 
    {
	if (ulInterceptGDI)
	    pDev->dwFlags |= GMVF_INTCPTGDI;	// Enable GDI Interception
    }

    ulValue = 4;
    pDev->dwFlags |= GMVF_OFFSCRNBM;	// Assume Offscreen bitmaps required
    if (_RegQueryValueEx(   hKey, 
                            szOffScrnBitMaps, 
                            NULL, 
                            NULL, 
                            &ulOffScrnBitMaps, 
                            &ulValue) == ERROR_SUCCESS) 
    {
	if (!ulOffScrnBitMaps)
	    pDev->dwFlags &= ~GMVF_OFFSCRNBM;	// Disable offscreen bitmaps
    }

    ulValue = 4;
    if ((_RegQueryValueEx(  hKey, 
                            "VGAVirtualization", 
                            NULL, 
                            NULL, 
                            (char *)&ulVirtualize, 
                            &ulValue) == ERROR_SUCCESS) && ulVirtualize) 
    {
	pDev->dwFlags |= GMVF_TRYTOVIRTUALISE4PLANEVGA;
    }

    ulValue = 4;
    if ((_RegQueryValueEx(  hKey, 
                            "Enable24bpp", 
                            NULL, 
                            NULL, 
                            (char *)&ulVirtualize, 
                            &ulValue) == ERROR_SUCCESS) && ulVirtualize) 
    {
	pDev->dwFlags |= GMVF_EXPORT24BPP;
    }

    if (_RegCloseKey(hKey) != ERROR_SUCCESS)
        DISPDBG(("Error closing registry key"));

    // Update registry information for Version number
    if (CM_Write_Registry_Value(pDev->DevNode, 
                                NULL, 
                                "Ver",
			        REG_SZ, 
                                VER_3DLABS_INF_STR, 
                                sizeof(VER_3DLABS_INF_STR),
			        CM_REGISTRY_SOFTWARE) != CR_SUCCESS) 
    {
	DISPDBG(("Writing Ver Failed"));
    }

    DISPDBG(( "Initialised Sucessfully. pGlintDev = 0x%x", pDev));
    return(pDev);
}

void 
InitialiseCachedMemoryTimings(PDEVICETABLE pDev)
{
    ULONG   ulValue, ulSize;

    // On P2 this has to be hard coded. Initially assume 4 banks.
    pDev->dwVSConfiguration = 0x000001f0;
    pDev->dwMemControl      = 0x00000000;
    pDev->dwBootAddress     = 0x00000020;
    pDev->dwMemConfig       = 0xe6002021;
    pDev->dwFifoControl     = 0x00001808;

    ulSize = 4;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
			        szVSConfiguration, 
                                REG_BINARY, 
                                &ulValue, 
                                &ulSize,
			        CM_REGISTRY_SOFTWARE) == CR_SUCCESS)
    {
        pDev->dwVSConfiguration = ulValue;
        DISPDBG(("Setting VSConfiguration to 0x%x", pDev->dwVSConfiguration));
    }

    ulSize = 4;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
				szMemControl, 
                                REG_BINARY, 
                                &ulValue, 
                                &ulSize,
				CM_REGISTRY_SOFTWARE) == CR_SUCCESS)
    {
        pDev->dwMemControl = ulValue;
        DISPDBG(("Setting MemControl to 0x%x", pDev->dwMemControl));
    }

    ulSize = 4;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
				szBootAddress, 
                                REG_BINARY, 
                                &ulValue, 
                                &ulSize,
				CM_REGISTRY_SOFTWARE) == CR_SUCCESS)
    {
        pDev->dwBootAddress = ulValue;
        DISPDBG(("Setting BootAddress to 0x%x", pDev->dwBootAddress));
    }

    ulSize = 4;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
			        szMemConfig, 
                                REG_BINARY, 
                                &ulValue, 
                                &ulSize,
				CM_REGISTRY_SOFTWARE) == CR_SUCCESS) 
    {
        pDev->dwMemConfig = ulValue | 0x60000000;
        DISPDBG(("Setting MemConfig to 0x%x", pDev->dwMemConfig));

    }

    ulSize = 4;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
				szFifoControl, 
                                REG_BINARY, 
                                &ulValue, 
                                &ulSize,
				CM_REGISTRY_SOFTWARE) == CR_SUCCESS) 
    {
        pDev->dwFifoControl = ulValue;
        DISPDBG(("Setting FifoControl to 0x%x", pDev->dwFifoControl));

    }
}

DWORD 
InitialiseChipAndMemoryPreSizing(PDEVICETABLE pDev)
{
    DWORD	ulValue;

    LOAD_GLINT_CTRL_REG(VSConfiguration,    pDev->dwVSConfiguration);

    // ensure PCI Retry Discon-without-data disabled on P2. This should
    // be done with a resistor, but just in case:
    READ_GLINT_CTRL_REG(ChipConfig,         ulValue);

    // Set up Delta speed to higher of PCI speed or MClk/2 (keep below 50MHz!)
    if (pDev->ClkSpeed > 660000)
	ulValue = (ulValue & ~(3 << 10)) | (3 << 10);   // MClk/2
    else 
    {
        if (ulValue & PM_CHIPCONFIG_AGPCAPABLE)
            ulValue = (ulValue & ~(3 << 10)) | (1 << 10);// PCIClk/2
        else
            ulValue = (ulValue & ~(3 << 10)) | (0 << 10);// PCIClk
    }

    ulValue |= 0x20;
    LOAD_GLINT_CTRL_REG(ChipConfig,         ulValue);

    LOAD_GLINT_CTRL_REG(IntEnable, 0);	                // Switch off all interrupts
    pDev->pGLInfo->dwFlags &= ~GMVF_VBLANK_ENABLED;

    LOAD_GLINT_CTRL_REG(VideoControl,       0);	        // Switch of video
    LOAD_GLINT_CTRL_REG(VGAControlReg,      0);	        // and the VGA
    SYNC_WITH_GLINT;				        // Sync chip to ensure quiescence
    LOAD_GLINT_CTRL_REG(MemControl,         pDev->dwMemControl);// Load up memory control registers
    LOAD_GLINT_CTRL_REG(BootAddress,        pDev->dwBootAddress);
    LOAD_GLINT_CTRL_REG(MemConfig,          pDev->dwMemConfig);  // Must load after bootaddress
    LOAD_GLINT_CTRL_REG(FifoControl,        pDev->dwFifoControl);

    if (pDev->pDeltaDev->ChipID == DELTA_ID)
        pDev->wBoardID = BOARD_PERMEDIA_DELTA;	        // Probably a Permedia reference board with Delta.
    else
        pDev->wBoardID = BOARD_PERMEDIA;		// Probably a Permedia reference board.

    // Set up memory apperture config
    LOAD_GLINT_CTRL_REG(ApertureOne,        0x0);              // Normal Aperture
    LOAD_GLINT_CTRL_REG(ApertureTwo,        0x8);              // 5551 Aperture

    LOAD_GLINT_CTRL_REG(BypassWriteMask,    0xFFFFFFFF);   // Setup Bypass write mask

    if (pDev->dwFlags & GMVF_DELTA) 
    {
        // Turn on Disconnect for the GLINT when its behind a delta
        ulValue = DISCONNECT_INPUT_FIFO_ENABLE;
        LOAD_GLINT_CTRL_REG(FIFODiscon,     ulValue);
    }

    // Set up the Disconnect control correctly
    if (pDev->dwFlags & GMVF_DELTA) 
    {
	LOAD_GLINT_CTRL_REG(FIFODiscon,     DISCONNECT_INPUT_FIFO_ENABLE);
	LOAD_GLINT_CTRL_REG(DeltaDisconnectControl, DISCONNECT_INPUT_FIFO_ENABLE);
    }
    else 
    {
	LOAD_GLINT_CTRL_REG(FIFODiscon,     DISCONNECT_INPUT_FIFO_ENABLE);
    }

    return(1);
}

VOID 
ResetChip(PDEVICETABLE pDev)
{
    // First ensure chip is inactive.
    SYNC_WITH_GLINT;
    InitialiseChipAndMemoryPreSizing(pDev);

    // FBWindowBase can be destroyed on a chip reset. Fix up our cache for it here
    if (pDev->pGLInfo)
        pDev->pGLInfo->dwCurrentOffscreen = 0;

}

// Map in and initialise memory regions.
DWORD 
MapInitAndSizeMemoryRegions(PDEVICETABLE    pDev, 
                            PDEVICETABLE    pDeltaDev, 
                            BOOL            Destructive)
{
    DWORD	dwRegBase;
    DWORD	*pR, *pFB, *pLB;
    DWORD	sFB, sLB, dLB;
    DWORD	ulValue;
    BOOL	writeCombine;
    HKEY	hKey;

    DISPDBG(("MapInitAndSizeMemoryRegions"));
    DISPCHIP((pDev));

    pDev->wBoardID  = BOARD_UNKNOWN;
    dwRegBase       = pDev->CMConfig.dMemBase[0];
 
    if (pDev->dwFlags & GMVF_DELTA) 
    {
        // Check for bit 17 bug in delta if we have a delta
	if ((dwRegBase & (1<<17)) == (pDeltaDev->CMConfig.dMemBase[0] & (1<<17))) 
        {
	    // bug doesnt apply. Map in delta region unchanged
	    dwRegBase = pDeltaDev->CMConfig.dMemBase[0];
	    DISPDBG(("Mapping in Delta Address Region at 0x%x", dwRegBase));
	}
	else 
        {
            ULONG	deltaRegion;
            // We have to remap the delta to be inside the delta region of the glint
            // address space. The delta region is PCI region 4 on Permedia,
            // 3 on TX and SX
            // If PCI region 3 of the PCI has not been validated correctly, then
            // the actual delta region would be region 3 of the CM region list.
            // check to see if region 4 is 256K in size, if not, check region 3
            if (pDev->CMConfig.dMemLength[4] == 256*1024) 
                deltaRegion = 4;
            else if (pDev->CMConfig.dMemLength[3] == 256*1024) 
                deltaRegion = 3;
            else 
            {
                DISPDBG(("Delta bit 17 bug: Neither PCI region 3 or 4 is 256K in size"));
                return(NULL);
            }

            dwRegBase = pDev->CMConfig.dMemBase[deltaRegion];
            if ((dwRegBase & (1<<17)) != (pDev->CMConfig.dMemBase[0] & (1<<17))) 
            {
	        dwRegBase += (1<<17);
            }

            DISPDBG(("Remapping Delta Address Region due to Bit 17 Bug"));
            DISPDBG(("Delta Base0:0x%x Glint Delta Region:0x%x Mapping at 0x%x",
	                pDeltaDev->CMConfig.dMemBase[0], 
                        pDev->CMConfig.dMemBase[deltaRegion], 
                        dwRegBase));

            // Physically move the delta base address
            pDeltaDev->PCIConfig.u.type0.BaseAddresses[0] = dwRegBase;
            CM_Call_Enumerator_Function(pDeltaDev->DevNode, 
                                        PCI_ENUM_FUNC_SET_DEVICE_INFO,
				        0x10, 
                                        &(pDeltaDev->PCIConfig.u.type0.BaseAddresses[0]), 
                                        4, 
                                        0);
	}
    }

    // point the registers at the correct area of mapped space
    pR = (DWORD *)(((char *)pDev->DataSegmentBase) + GLINT_REGISTERS_OFFSET);
    {
        DWORD   nPages, Page, PhysPage;

        // Map in the physical pages
        Page = ((DWORD)pR) >> 12;
        nPages = (pDev->CMConfig.dMemLength[0] + 4095) >> 12;
        PhysPage = dwRegBase >> 12;
#ifndef WIN98
        __asm {
            pushad
            push    PC_INCR+PC_USER+PC_WRITEABLE
            push    PhysPage
            push    nPages
            push    Page
            VxDCall(_PageCommitPhys)

            add     esp, 0x10
            popad
        }
#else
		_PageCommitPhys(Page, nPages, PhysPage, (PC_INCR+PC_USER+PC_WRITEABLE));
#endif //#ifndef WIN98

    }

    pDev->pRegisters = (PREGISTERS)pR;

    DISPDBG(("Mapped Control Registers. Phys:0x%x Virt:0x%x Length:0x%x", dwRegBase, pR, pDev->CMConfig.dMemLength[0])); 

    InitialiseCachedMemoryTimings(pDev);

    if (!FindRamDacType(pDev)) 
    {
	DISPDBG(("FindRamDacType Failed"));
	return(NULL);
    }

    InitialiseChipAndMemoryPreSizing(pDev);


    // Map in the First Memory Region. It is the Local Buffer on SX and TX,
    // and both LB and FB on GIGI and PERMEDIA
    sFB = pDev->CMConfig.dMemLength[1];
    pFB = _MapPhysToLinear(pDev->CMConfig.dMemBase[1], sFB, 0);
    if (pFB == (DWORD *)0xFFFFFFFF) 
    {
        DISPDBG(( "Cannot map Region 1"));
        return (NULL);
    }

    pLB = 0;
    sLB = 0;
    // The second aperture points to the same memory as the first aperture, 
    // but we will possibly have a different format setting for this aperture.
    // Store its pointer in the Local Buffer pointer.
    sLB = pDev->CMConfig.dMemLength[2];
    pLB = _MapPhysToLinear( pDev->CMConfig.dMemBase[2],
			    pDev->CMConfig.dMemLength[2], 
                            0);			
    if (pLB == (DWORD *)0xFFFFFFFF) 
    {
	DISPDBG(( "Cannot map Region 2"));
	return(0);
    }

	pDev->pFrameBuffer = pFB;
	pDev->pLocalBuffer = pLB;

    if (Destructive) 
    {
        // Size the Frame Buffer in words
        sFB >>= 2;

        for (ulValue = 0; ulValue < sFB; ulValue += FB_PROBE_SIZE) 
        {
            pFB[ulValue] = FB_PROBE_PATTERN;
            pFB[1] = 0;
            if (pFB[ulValue] != FB_PROBE_PATTERN) 
            {
                DISPDBG(("Memory Sizing failed at 0x%x words", ulValue));
                sFB = ulValue;
                break;
            }
        }

        // Restore size to bytes
        sFB <<= 2;
    }

    {
        ULONG   nBanks;

        if (sFB & 0x1fffff)
        DISPDBG(("ERROR: Non 2Mb sizing of Permedia Framebuffer 0x%x", sFB));

        nBanks = sFB >> 21;

        if ((nBanks < 1) || (nBanks > 4)) 
        {
            DISPDBG(("ERROR: Number of memory banks is %d", nBanks));
            nBanks = 1;
        }

        DISPDBG(("Configuring %d memory banks", nBanks));

        // Update memconfig appropriatly
        pDev->dwMemConfig = (pDev->dwMemConfig & 0x9FFFFFFF) | ((nBanks - 1) << 29);
        LOAD_GLINT_CTRL_REG(MemConfig, pDev->dwMemConfig);

        // LB depth is always 16 bits. sLB and pLB refer to the 5551 mapped region
        dLB = 16;
        sLB = sFB >> 1;
    }

    if (pLB) 
    {
	// SX and TX have a separate Local Buffer. We need to work out the
	// Depth of the buffer, then the size of it.
															    
	// XXXXX For now, default to the region size and 32 bit depth
	dLB = 32;
    }
    else 
    {
	// Permedia and GIGI have a single memory region which is Local Buffer
	// and Frame Buffer. For Gigi we have to split into	2, as first half is
	// VRAM and second half is DRAM. For Permedia the entire buffer is SGRAM,
	// so we declare no local buffer at this stage.
	sFB = sLB = sFB >> 1;
	pLB = pFB + (sFB >> 2);
	dLB = 16;
    }

    pDev->dwFrameBufferSize  = sFB;
    pDev->dwLocalBufferSize  = sLB;
    pDev->dwLocalBufferDepth = dLB;

    if (_RegOpenKey(HKEY_LOCAL_MACHINE, 
                    pDev->szInformationKey, 
                    &hKey) != ERROR_SUCCESS) 
    {
        DISPDBG(("Couldnt open HKLM\\%s", pDev->szInformationKey));
    }

    ulValue = sizeof( writeCombine );
    if (_RegQueryValueEx(   hKey, 
                            "DisableWC", 
                            NULL, 
                            NULL, 
			    &writeCombine, 
                            &ulValue) != ERROR_SUCCESS)
    {	    
	writeCombine = TRUE;// Key doesn't exist - default WC on
    }
    else
    {
	writeCombine = !writeCombine;// Key exists - invert sense from disable to enable
    }

    if( writeCombine && 
        TestWCCapability() &&
	SetVariableMTRRToWC( pDev->CMConfig.dMemBase[1], sFB ))
    {
	pDev->dwFlags |= GMVF_FRAME_BUFFER_IS_WC;
    }

    DISPDBG(("Memory Sizes Detected: FB:0x%x, LB:0x%x LB Depth is %d bits", sFB, sLB, dLB));

    if (Destructive) 
    {
        ZeroMemoryRegion(pDev->pFrameBuffer, pDev->dwFrameBufferSize);
    }

    return(1);
}

DWORD 
ZeroMemoryRegion(VOID *pMemory, 
                 ULONG sMem)
{
    ULONG   *pMem = (ULONG *) pMemory;
    sMem >>= 2;

    if (sMem) 
    {
        do  
        {
            *pMem = 0;
            pMem++;
	} while ((--sMem) != 0);
    }	
    return(1);
}

 
DWORD
InitialiseChip(PDEVICETABLE pDev)
{
    // Finish initialising the chip and put it into a working state

    DISPDBG(("InitialiseChip"));
    DISPCHIP((pDev));

    WAIT_GLINT_FIFO(16);
    LD_GLINT_REG(RasterizerMode,        0x18d);
    LD_GLINT_REG(ColorDDAMode,          0);
    LD_GLINT_REG(TextureColorMode,      0);
    LD_GLINT_REG(FogMode,               0);
    LD_GLINT_REG(AlphaTestMode,         0);
    LD_GLINT_REG(AntialiasMode,         0);
    LD_GLINT_REG(Window,                0);
    LD_GLINT_REG(StencilMode,           0);
    LD_GLINT_REG(DepthMode,             0);
    LD_GLINT_REG(AlphaBlendMode,        0);
    LD_GLINT_REG(DitherMode,            0);
    LD_GLINT_REG(LBReadMode,            0);
    LD_GLINT_REG(LBWriteMode,           0);
    LD_GLINT_REG(StatisticMode,         0);
    LD_GLINT_REG(FBSoftwareWriteMask,   0xffffffff);
    LD_GLINT_REG(FBHardwareWriteMask,   0xffffffff);

    WAIT_GLINT_FIFO(15);
    LD_GLINT_REG(LogicalOpMode,         0x20);
    LD_GLINT_REG(dXDom,                 0);
    LD_GLINT_REG(dXSub,                 0);
    LD_GLINT_REG(dY,                    0x10000);
    LD_GLINT_REG(WindowOrigin,          0);
    LD_GLINT_REG(ScissorMode,           2);
    LD_GLINT_REG(LineStippleMode,       0);
    LD_GLINT_REG(AreaStippleMode,       0);
    LD_GLINT_REG(FilterMode,            0);
    LD_GLINT_REG(FBWindowBase,          0);
    LD_GLINT_REG(FBPixelOffset,         0);
    LD_GLINT_REG(FBWriteMode,	        5);
    LD_GLINT_REG(FBReadMode,            0);
    LD_GLINT_REG(ScreenSize,            (pDev->pGLInfo->DesktopDisplayInfo.ddiXRes + 
                                         (pDev->pGLInfo->DesktopDisplayInfo.ddiYRes << 16)));

    // Ensure we don't do an AGP access on the LUT
    WAIT_GLINT_FIFO(3);
    LD_GLINT_REG(TextureBaseAddress,    0);
    LD_GLINT_REG(TexelLUTTransfer,      0);
    LD_GLINT_REG(DMAControl,            0);

    // FBWindowBase has been zero'ed. Fix up our cache for it here
    if (pDev->pGLInfo)
        pDev->pGLInfo->dwCurrentOffscreen = 0;

    WAIT_GLINT_FIFO(8);
    LD_GLINT_REG(FxTextureReadMode  , 0); 
    LD_GLINT_REG(TextureLUTMode     , 0);    
    LD_GLINT_REG(FBReadPixel        , 0);
    LD_GLINT_REG(YUVMode            , 0);
    LD_GLINT_REG(XLimits            , 0);
    LD_GLINT_REG(YLimits            , 0);
    LD_GLINT_REG(TextureAddressMode , 0);
    // Setup texture unit ready for Text from Local Buffer
    WAIT_GLINT_FIFO(10);
    LD_GLINT_REG(SStart,                0);
    LD_GLINT_REG(dSdx,                  0x100000);
    LD_GLINT_REG(dSdyDom,               0);
    LD_GLINT_REG(TStart,                0);
    LD_GLINT_REG(dTdx,                  0);
    LD_GLINT_REG(dTdyDom,               0);
    LD_GLINT_REG(TextureDataFormat,     DEFAULT_P2TEXTUREDATAFORMAT);
    LD_GLINT_REG(TextureLUTMode,        0);
    LD_GLINT_REG(TextureAddressMode,    DEFAULT_P2TEXTUREADDRESSMODE); 
    LD_GLINT_REG(FxTextureReadMode,     DEFAULT_P2TEXTUREREADMODE); 

    if (pDev->dwFlags & GMVF_DELTA) 
    {
        LD_GLINT_REG(DeltaMode, 0);
    }

    DISPDBG(("InitialiseChip Done"));
    return(1);
}

DWORD _cdecl 
InitialiseSharedData(PDEVICETABLE pDev)
{
    DWORD   i;
    LPGLINTINFO pGLInfo = pDev->pGLInfo;
    DISPDBG(("InitialiseSharedData for 0x%x", pDev));
    DISPCHIP((pDev));

    pDev->pGLInfo               = pGLInfo;
    pGLInfo->dwRenderChipID 	= pDev->ChipID;
    pGLInfo->dwRenderChipRev 	= pDev->RevisionID;
    pGLInfo->dwRenderFamily   	= pDev->FamilyID;
    pGLInfo->dwBoardID 		= pDev->wBoardID;
    pGLInfo->dwRamDacType 	= pDev->DacId;
    pGLInfo->dwBoardRev 	= 1;
    if (pDev->dwFlags & GMVF_DELTA)
    {// We have a delta
        pGLInfo->dwSupportChipID 	= pDev->pDeltaDev->ChipID;
        pGLInfo->dwSupportChipRev 	= pDev->pDeltaDev->RevisionID;
    }
    else 
    {// No delta present
        pGLInfo->dwSupportChipID 	= NOCHIP_ID;
        pGLInfo->dwSupportChipRev 	= 1;
    }

    if (pDev->dwFlags & GMVF_DELTA) 
    {
        pGLInfo->dwDisconCtrl   = (unsigned long) OFFSET_GLINT_REG(DeltaDisconnectControl) + GLINT_REGISTERS_OFFSET;
        pGLInfo->pIntEnable     = (unsigned long) &(pDev->pRegisters->Glint.DeltaIntEnable);
        pGLInfo->pIntFlags      = (unsigned long) &(pDev->pRegisters->Glint.DeltaIntFlags);
    } else 
    {
        pGLInfo->dwDisconCtrl   = (unsigned long) OFFSET_GLINT_REG(FIFODiscon) + GLINT_REGISTERS_OFFSET;
        pGLInfo->pIntEnable     = (unsigned long) &(pDev->pRegisters->Glint.IntEnable);
        pGLInfo->pIntFlags      = (unsigned long) &(pDev->pRegisters->Glint.IntFlags);
    }

    pGLInfo->dwFlags            = pDev->dwFlags;
    pGLInfo->ddFBSize           = pDev->dwFrameBufferSize;
    pGLInfo->ddLBSize           = pDev->dwLocalBufferSize;
    pGLInfo->cLBDepth           = (char) pDev->dwLocalBufferDepth;
    pGLInfo->GlintBoardStatus   = GLINT_DMA_COMPLETE;

    pGLInfo->dwFontCacheSize    = DMABuffers.dwFontCacheSize;
    pGLInfo->dwFontCacheVirt    = DMABuffers.dwFontCacheVirt;
    pGLInfo->dwFontCache16      = DMABuffers.dwFontCache16;
    pGLInfo->dw2D_DMA_Phys      = pDev->dw2D_DMA_Phys;

    // update the base of the glint configuration key. Shouldbe pDev->szInformationKey - "\Display"
    i = SIZE_CONFIGURATIONBASE;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
                                szInformationKeyName,
				REG_SZ, 
                                pGLInfo->RegistryConfigBase, 
                                &i,
				CM_REGISTRY_SOFTWARE) != CR_SUCCESS)
    {
        strcpy(pGLInfo->RegistryConfigBase, "3Dlabs");
    }

    // initialize the circular DMA queue
    pGLInfo->frontIndex = pGLInfo->backIndex  = 0;
    pGLInfo->maximumIndex = MAX_QUEUE_SIZE - 1;

    // the size of the queue we actually use is dynamically configurable but
    // initialize it to be as small as possible. This default size will work
    // for all interrupt driven DMA buffers regardless of how many buffers
    // are actually available.
    //
    pGLInfo->endIndex = 2;

    // initially no interrupts are available. Later we try to enable the
    // interrupts and if they happen the interrupt handler will set the
    // available bits in this word. So it's a sort of auto-sensing mechanism.
    //
    pGLInfo->InterruptPending = 0;

    /* Finally bring the DMA buffer records up to date */
    pGLInfo->wDMABufferCount = DMABuffers.wDMABufferCount;
    pGLInfo->dwDMABufferSize = DMABuffers.dwDMABufferSize;
    for (i = 0; i < DMABuffers.wDMABufferCount; i++) 
    {
	pGLInfo->dwDMABufferVirt[i] = DMABuffers.dwDMABufferVirt[i];
	pGLInfo->dwDMABufferPhys[i] = DMABuffers.dwDMABufferPhys[i];
    }

    // Allocate selectors for 16 bit code
    pGLInfo->dwRegSel       = AllocateSelector(pDev->pRegisters, pDev->CMConfig.dMemLength[0] >> 12);
    pGLInfo->dwFBSel        = AllocateSelector(pDev->pFrameBuffer, pDev->dwFrameBufferSize >> 12);
    pGLInfo->dwLBSel        = AllocateSelector(pDev->pLocalBuffer, pDev->dwLocalBufferSize >> 12);

    pGLInfo->dwpRegisters   = (DWORD)pDev->pRegisters;
    pGLInfo->dwpFrameBuffer = (DWORD)pDev->pFrameBuffer;
    pGLInfo->dwpLocalBuffer = (DWORD)pDev->pLocalBuffer;

    DISPDBG(("InitialiseSharedData Done"));

    return (1);
}

DWORD 
InitialiseInterrupts(PDEVICETABLE pDev)
{
    DWORD   ulValue, ulDisableIRQ;
    HKEY    hKey;

    // Decide if we should use interrupts and set them up if so.
    DISPDBG(("InitiailiseInterrupts"));
    if (pDev->IRQHandle != 0) 
    {
	DISPDBG(("InitialiseInterupts called when already enabled. Redundant call"));
	return(1);
    }

    // Initially disable interrupts
    pDev->pGLInfo->dwFlags  |= GMVF_NOIRQ;
    pDev->IRQHandle         = 0;

    if (pDev->pGLInfo->dwFlags & GMVF_DELTA) 
    {
	if (pDev->pDeltaDev->CMConfig.wNumIRQs == 0) 
        {
	    DISPDBG(("IRQ not allocated to device by OS"));
	    return(1);
	}
	IRQDesc.VID_IRQ_Number = pDev->pDeltaDev->CMConfig.bIRQRegisters[0];
    }
    else 
    {
	if (pDev->CMConfig.wNumIRQs == 0) 
        {
	    DISPDBG(("IRQ not allocated to device by OS"));
	    return(1);
	}
	IRQDesc.VID_IRQ_Number = pDev->CMConfig.bIRQRegisters[0];
    }


    if (_RegOpenKey(HKEY_LOCAL_MACHINE, 
                    pDev->szInformationKey, 
                    &hKey) != ERROR_SUCCESS) 
    {
        DISPDBG(("Couldnt open HKLM\\%s", pDev->szInformationKey));
    }

    ulValue = 4;
    if (_RegQueryValueEx(   hKey, 
                            szDisableIRQ, 
                            NULL, 
                            NULL, 
                            &ulDisableIRQ,	
                            &ulValue) == ERROR_SUCCESS) 
    {
        if (!ulDisableIRQ)
            pDev->pGLInfo->dwFlags &= ~GMVF_NOIRQ;// Enable them if in the registry
    }

    if (_RegCloseKey(hKey) != ERROR_SUCCESS)
        DISPDBG(("Error closing registry key"));

#ifndef DDFLIP_INTERVALN
    if (pDev->pGLInfo->dwFlags & GMVF_NOIRQ)
    return(1);
#endif // #ifndef DDFLIP_INTERVALN

    // Set up the interrupt handler
    IRQDesc.VID_Options 	        = VPICD_OPT_CAN_SHARE | VPICD_OPT_REF_DATA;
    IRQDesc.VID_Hw_Int_Proc 		= (ULONG) MiniVDD_Hw_Int;
    IRQDesc.VID_Virt_Int_Proc 		= 0;
    IRQDesc.VID_EOI_Proc 	        = 0;
    IRQDesc.VID_Mask_Change_Proc 	= 0;
    IRQDesc.VID_IRET_Proc	 	    = 0;
    IRQDesc.VID_IRET_Time_Out 		= 500;
    IRQDesc.VID_Hw_Int_Ref          = (DWORD)pDev;  // so int handler knows which
                                                    // device the int is for

    pDev->IRQHandle = VPICD_Virtualize_IRQ(&IRQDesc);

    if (pDev->IRQHandle) 
    {
	DISPDBG(( "An Interrupt has been established. Routine at 0x%x", MiniVDD_Hw_Int));
	VPICD_Physically_Unmask(pDev->IRQHandle);
    }
    else 
    {	
	pDev->pGLInfo->dwFlags |= GMVF_NOIRQ;// Disable interrupts
    }

    return(1);
}


DWORD 
InitialiseDMA(PDEVICETABLE pDev) 
{
    DWORD	ulValue, ulRegistry;
    HKEY    hKey;
    PVOID	Phys, Virt;
    DWORD   Base, Result;
    int	nPages;

    DISPDBG(("InitialiseDMA"));
    DISPCHIP((pDev));

    if (_RegOpenKey(HKEY_LOCAL_MACHINE, 
                    pDev->szInformationKey, 
                    &hKey) != ERROR_SUCCESS) 
    {
        DISPDBG(("Couldnt open HKLM\\%s", pDev->szInformationKey));
    }

    // Try to allocate the desired amount of font cache
    DMABuffers.dwFontCacheSize = 0;
    ulValue = 4;
    if (_RegQueryValueEx(   hKey, 
                            szFontCacheSize, 
                            NULL, 
                            NULL, 
                            &ulRegistry, 
                            &ulValue) == ERROR_SUCCESS) 
    {
        // Registry is in number of K. Convert to number of pages.
        DMABuffers.dwFontCacheSize = (ulRegistry + 3) >> 2;
    }
    else 
    {
        DISPDBG(("Using default number of 50 pages of Font Cache"));
        DMABuffers.dwFontCacheSize = 50;    // Default of 50 pages - 200K
    }

    // Ensure we dont exceed size of defined cache limits
    if (DMABuffers.dwFontCacheSize > (FONT_CACHE_SIZE >> 12))
        DMABuffers.dwFontCacheSize = FONT_CACHE_SIZE >> 12;

    // No font caching on an SX
    if (pDev->ChipID == GLINT300SX_ID)
        DMABuffers.dwFontCacheSize = 0;

    if (DMABuffers.dwFontCacheSize != 0) 
    {
        // Now allocate the Font Cache

        Virt = (PVOID) (pDev->DataSegmentBase + FONT_CACHE_OFFSET);
        Base = (pDev->DataSegmentBase + FONT_CACHE_OFFSET) >> 12;
        nPages = DMABuffers.dwFontCacheSize;
#ifndef WIN98
        __asm {
            pushad  
            push    (PC_USER + PC_WRITEABLE)
            push    0
            push    PD_ZEROINIT
            push    nPages
            push    Base
            VxDCall(_PageCommit)
            add     esp, 0x14
            mov     Result, eax
            popad
        }
#else
		Result = _PageCommit(Base, nPages, PD_ZEROINIT, 0, (PC_USER + PC_WRITEABLE));
#endif //#ifndef WIN98


        if (!Result) 
        {
            DISPDBG(("PageCommit Font Cache Failed"));
		        DMABuffers.dwFontCacheSize = 0;
        }
        else 
        {
            DMABuffers.dwFontCacheVirt = (ULONG) Virt;
            DMABuffers.dwFontCache16 = AllocateSelector(Virt, DMABuffers.dwFontCacheSize);
            DISPDBG(("Font Cache allocated: V:0x%x nPages:0x%x Segment:0x%x",
                        Virt, 
                        DMABuffers.dwFontCacheSize, 
                        DMABuffers.dwFontCache16));
        }
    }

    if (DMABuffers.dwFontCacheSize == 0) 
    {
	DMABuffers.dwFontCacheVirt = 0;
	DMABuffers.dwFontCache16 = 0;
    }

    // Check to see if DMA has been enabled in the registry.
    // If variable isnt present, assume enabled DMA.
    ulValue = 4;
    if ((_RegQueryValueEx(  hKey, 
                            szEnableDMA,
                            NULL, 
                            NULL, 
                            &ulRegistry, 
                            &ulValue) == ERROR_SUCCESS) && !ulRegistry)
    {
        DISPDBG(("DMA Disabled in the registry"));
        return(1);
    }

    pDev->dw2D_DMA_Phys = 0;
    if (pDev->ChipID == PERMEDIA2_ID || pDev->ChipID == TIPERMEDIA2_ID)
    {
        Base = (pDev->DataSegmentBase + DMA_BUFFER_2D) >> 12;
        nPages = DMA_BUFFER_2D_SIZE >> 12;
        __asm {
	        pushad
	        push    0xffffffff
	        push    0
	        push    0
	        push    (PCC_ZEROINIT + PC_USER + PC_WRITEABLE)
	        push    nPages
	        push    Base
	        VxDCall(_PageCommitContig)
	        add     esp, 0x18
	        mov     Result, eax
	        popad
        }
        if (!Result) 
        {
	    DISPDBG(("PageCommitContig 2D DMA Buffer Failed"));
        }
        else {
                pDev->dw2D_DMA_Phys = Result;
                DISPDBG(("2D DMA buffer allocated: V:0x%x P:0x%x nPages:0x%x",
                            pDev->DataSegmentBase + DMA_BUFFER_2D, 
                            Result, 
                            nPages));
        }
    }

    // Do not reallocate DMA pages if we are called multiple times.
    if (DMABuffersAllocated)
        return(1);

    DMABuffersAllocated = 1;

    DMABuffers.wDMABufferCount = 0;
    DMABuffers.dwDMABufferSize = 0;

    // Read DMA buffer count from registry (take 32K of buffers as default)
    ulValue = 4;
    if (_RegQueryValueEx(   hKey, 
                            szDMABufferCount, 
                            NULL, 
                            NULL, 
                            &ulRegistry, 
                            &ulValue) == ERROR_SUCCESS) 
    {
	DMABuffers.wDMABufferCount = (unsigned short) ulRegistry;
    }
    else 
    {
        DISPDBG(("Using default number of 4 DMA buffers"));
        DMABuffers.wDMABufferCount = 4;    // Default of 4 buffers
    }

    // Read DMA buffer size from registry
    ulValue = 4;
    if (_RegQueryValueEx(   hKey, 
                            szDMABufferSize, 
                            NULL, 
                            NULL, 
                            &nPages, 
                            &ulValue) == ERROR_SUCCESS) 
    {
        // nPages is currently in K. Convert to 4K pages
        nPages = (nPages + 3) >> 2;
    }
    else 
    {
        DISPDBG(("Using default number of 8 4K pages for each DMA buffer"));
        nPages = 8; 
    }

    DMABuffers.dwDMABufferSize = nPages << 12;

    if (DMABuffers.wDMABufferCount > MAXDMABUFF)
	DMABuffers.wDMABufferCount = MAXDMABUFF;

    if (DMABuffers.wDMABufferCount && DMABuffers.dwDMABufferSize)
    {
	DWORD buffNum;
	BOOL writeCombine;

	ulValue = sizeof( writeCombine );
	if (_RegQueryValueEx(   hKey, 
                                "DisableWC", 
                                NULL, 
                                NULL, 
				&writeCombine, 
                                &ulValue) != ERROR_SUCCESS)
	{
	    writeCombine = TRUE;// Key doesn't exist - default WC on
	}
	else
	{
	    writeCombine = !writeCombine;// Key exists - invert sense from disable to enable
	}

	// Disable write-combining if the processor doesn't
	// have the necessary HW support
	writeCombine = writeCombine && TestWCCapability();

#define PAGES_IN_64K	16

	buffNum = 0;
	if( writeCombine )
	{
	    int totalPages, allocPages;

	    // Try to allocate as many pages for DMA buffers in the low
	    // 512 Kb ( 128 4K pages ) of memory as possible. These pages
	    // can then be marked as write-combined.
	    // Allocate in multiples of 64 K
	    totalPages = DMABuffers.wDMABufferCount * nPages;
	    allocPages = (totalPages + PAGES_IN_64K - 1) & ~(PAGES_IN_64K - 1);

	    DISPASSERT( nPages > 0 );
	    DISPASSERT( allocPages > 0 );
	    do
	    {
		Virt = _PageAllocate(   allocPages, 
                                        PG_SYS, 
                                        0, 
                                        0xf, 
                                        0, 
                                        128,
					&Phys, 
                                        PAGECONTIG | PAGEFIXED | PAGEUSEALIGN);

		allocPages -= PAGES_IN_64K;
	    }
	    while( !Virt && ( allocPages >= nPages ));

	    // Got the memory - set it to WriteCombine

	    if( Virt )
	    {
		allocPages += PAGES_IN_64K;

		if( !SetFixedMTRRToWC( (unsigned long) Phys, allocPages ))
		{
		    DISPDBG(( "Failed to set %d pages @ 0x%08x to WC", allocPages, Phys ));
		}

		while(( buffNum < DMABuffers.wDMABufferCount ) && ( allocPages >= nPages ))
		{
		    DMABuffers.dwDMABufferVirt[buffNum] = (ULONG) Virt;
		    DMABuffers.dwDMABufferPhys[buffNum] = (ULONG) Phys;

		    DISPDBG(("DMA buffer allocated: V:0x%x P:0x%x nPages:0x%x",Virt, Phys, nPages));

		    (int)Virt += DMABuffers.dwDMABufferSize;
		    (int)Phys += DMABuffers.dwDMABufferSize;

		    allocPages -= nPages;
		    buffNum++;
		}
	    }
	}
	else
	{
	    DISPDBG(( "Write-combining disabled" ));
	}

        // Now allocate any remaining DMA buffers that couldn't be
        // allocated in low memory, in general memory.
        while( buffNum < DMABuffers.wDMABufferCount )
        {
	    Virt = _PageAllocate(   nPages, 
                                    PG_SYS, 
                                    0, 
                                    0, 
                                    0, 
                                    0x100000,
                                    &Phys, 
                                    PAGECONTIG | PAGEFIXED | PAGEUSEALIGN);
	    if( Virt )
	    {
		DMABuffers.dwDMABufferVirt[buffNum] = (ULONG) Virt;
		DMABuffers.dwDMABufferPhys[buffNum] = (ULONG) Phys;
		DISPDBG(("DMA buffer allocated: V:0x%x P:0x%x nPages:0x%x",Virt, Phys, nPages));
	    }
	    else
	    {
		// Can't allocate all the requested buffers
		DMABuffers.wDMABufferCount = (unsigned short) buffNum;
		break;
	    }
	    buffNum++;
        }
    }

    if (_RegCloseKey(hKey) != ERROR_SUCCESS)
        DISPDBG(("Error closing registry key"));

    return(1);
}

DWORD
FindPartialProducts(PDEVICETABLE pDev, 
                    ULONG pitch)
{
    ULONG   MaxPP, MinPP;
    ULONG   ppn, j, maxppn;
    ULONG   pp[4];

    maxppn  = 3; // Number of partial products allowed
    MaxPP   = 10; // P2 partial product
    MinPP   = 5;

    // Generic partial product calculator for GLINT and PERMEDIA
    ppn = pp[0] = pp[1] = pp[2] = pp[3] = 0;
    
    if (pitch >= MaxPP) 
    {
        ppn = pitch >> (MaxPP);
        if (ppn <= maxppn) 
        {
            for (j = 0; j < ppn; j++) 
            {
                pp[j] = 1 + MaxPP - MinPP;
            }
        }
    }
    for(j = MinPP ; j < MaxPP ; j++) 
    {
        if (pitch & (1 << j)) 
        {
            if (ppn < maxppn)
                pp[ppn] = j + 1 - MinPP;
            ppn++;
        }
    }

    if (ppn > maxppn) 
    {
        DISPDBG(("Cannot locate partial products for %d", pitch));
        return(0);
    }
    else
    {
        return(pp[0] | (pp[1] << 3) | (pp[2] << 6) | (pp[3] << 25));
    }
}

ULONG 
AutodetectBlockFillSize(PDEVICETABLE pDev)
{
    ULONG	ulValue, ulRegValue;
    LPGLINTINFO pGLInfo = pDev->pGLInfo;

    // Autodetect on a TX or Permedia.
    DISPDBG(("Autodetecting Block Fill Size"));

    // Auto detect the Block Fill Size. First zero first 32 pixels
    for (ulValue = 0; ulValue < 32; ulValue++) 
    {
        pDev->pFrameBuffer[ulValue] = 0;
    }

    WAIT_GLINT_FIFO(10);
    LD_GLINT_REG(FBReadPixel,       2);// 32 bit pixels on PERMEDIA

    // setup for a 1024 wide framebuffer
    LD_GLINT_REG(FBReadMode, FindPartialProducts(pDev, 1024)); 

    // set FBWrite for 32 pixel block fill
    LD_GLINT_REG(FBWriteMode,       5); // Block width of 32 and enable

    // generate a single block fill message to write to this area of the screen
    LD_GLINT_REG(FBBlockColor,      0x010101);
    LD_GLINT_REG(StartXDom,    	    0);
    LD_GLINT_REG(StartXSub,    	    32 << 16);
    LD_GLINT_REG(StartY,       	    0);
    LD_GLINT_REG(Count,        	    1);
    LD_GLINT_REG(Render,       	    0x68);// FF Inc 32, Trap, FF Enable

    // must sync to ensure fill has completed
    SYNC_WITH_GLINT;

    ulValue = 0;
    while (pDev->pFrameBuffer[ulValue] == 0x010101)
	ulValue++;

    // If we have either of the supported block fill sizes, set them in FBModeSel
    if (ulValue == 16 || ulValue == 32) 
    {
        pDev->dwFlags |= GMVF_FFON;
        ulRegValue = (ulRegValue & ~(3<<10)) | (ulValue << 6);
        LD_GLINT_REG(FBWriteMode, 1);   
        pDev->pGLInfo->cBlockFillSize = (unsigned char) ulValue;
    }
    else 
    {
	// No block fills available.	
        pDev->dwFlags &= ~GMVF_FFON;
        ulRegValue = (ulRegValue & ~(3<<10));
        LD_GLINT_REG(FBWriteMode, 1);
        ulValue = 0;
        pDev->pGLInfo->cBlockFillSize = 0;
    }

    DISPDBG(("Autodetect Block Fill Size = %d pixels", ulValue));

    return(ulValue);
}


DWORD _cdecl 
InitialiseDisplay(PDEVICETABLE pDev)
{
    ULONG   ulValue;

    DISPDBG(("InitialiseDisplay"));
    DISPCHIP((pDev));

    // Initialise all the registers that matter 
    InitialiseChip(pDev);

    // Work out the block fill size
    AutodetectBlockFillSize(pDev);

    // We should check to see if the chip has hardware write masks,
    // but for now assume that it has.
    pDev->pGLInfo->dwFlags |= GMVF_HWWRITEMASK;

    // Calculate partial products.
    // Note: some of the device driver routines use optimizations which require the
    // partial products for half and quarter of the current resolution.  They are
    // calculated here at startup instead of each time they are required.
    // NOTE ALSO this assumes partial products exist for half and quarter values.
    // this is not the case for example for 800x600 mode.
    ulValue = FindPartialProducts(pDev, pDev->pGLInfo->DesktopDisplayInfo.ddiXRes/2);
    pDev->pGLInfo->dwPartialProducts2 = ulValue;

    ulValue = FindPartialProducts(pDev, pDev->pGLInfo->DesktopDisplayInfo.ddiXRes/4);
    pDev->pGLInfo->dwPartialProducts4 = ulValue;

    ulValue = FindPartialProducts(pDev, pDev->pGLInfo->DesktopDisplayInfo.ddiXRes);
    pDev->pGLInfo->dwPartialProducts = ulValue;

    WAIT_GLINT_FIFO(2);
    LD_GLINT_REG(FBReadMode, ulValue);

    // Work out cPelSize (0 - 8bpp, 1 - 16bpp, 2 - 32bpp);
    ulValue = pDev->pGLInfo->DesktopDisplayInfo.ddiBpp >> 4;
    if (pDev->pGLInfo->DesktopDisplayInfo.ddiBpp == 24) 
    {
        LD_GLINT_REG(FBReadPixel, 4);
    }
    else 
    {
        LD_GLINT_REG(FBReadPixel, ulValue);
    }

    return(1);
}

DWORD _cdecl 
InitialiseMode(PDEVICETABLE pDev)
{
    DISPDBG(("InitialiseMode"));
    DISPCHIP((pDev));

    if (!InitialisedIntDMA)
    {
        InitialisedIntDMA = 1;

        // Initialise the interrupts
        if (!InitialiseInterrupts(pDev)) 
        {
            DISPDBG(("InitialiseInterrupts Failed"));
            return(NULL);
        }
    } 

    if (!InitialiseVideo(pDev)) 
    {
        DISPDBG(("InitialiseVideo Failed"));
        return(0);
    }
   
    if (!InitialiseDisplay(pDev)) 
    {
        DISPDBG(("InitialiseDisplay Failed"));
        return(0);
    }

    return(1);
}


// Procedure to completly reinitialise the display after it has been corrupted in
// some way.
DWORD _cdecl 
ReinitialiseMode() 
{
    PDEVICETABLE pDev = FindPrimarypDev();

    DISPDBG(("ReinitialiseMode"));
    DISPCHIP((pDev));

    if (!InitialiseVideo(pDev)) 
    {
        DISPDBG(("InitialiseVideo Failed"));
        return(0);
    }
    
    InitialiseChipAndMemoryPreSizing(pDev);

    if (!InitialiseDisplay(pDev)) 
    {
        DISPDBG(("InitialiseDisplay Failed"));
        return(0);
    }
    return(1);
}

// Allocates a block of memory
extern DWORD _cdecl 
AllocateGlobalMemory(DWORD Size)
{
    DWORD   *pMemory;
    WORD    pMemory16;

    // Convert size into pages
    Size        = (Size + 4095) >> 12;
    pMemory     = _PageAllocate(Size, PG_SYS, 0, 0, 0, 0, 0, PAGEZEROINIT);
    pMemory16   = AllocateSelector(pMemory, Size);

    DISPDBG(("Global Alloc %d pages at 0x%x %x:0", Size, pMemory, pMemory16));
    return(pMemory16);
}

// Frees a block of memory
extern DWORD _cdecl 
FreeGlobalMemory(DWORD Selector)
{
    DWORD   *pMemory;

    pMemory = SelectorMapFlat(Selector);
    FreeSelector(Selector);

    DISPDBG(("Global Free 0x%x %x:0", pMemory, Selector));
    return(_PageFree(pMemory, 0));
}

// This rather nasty function needs to translate the current driver data
// segment into a far larger sparse segment - where the data segment is a
// copy if the real data segment and at specific offsets various areas of
// memory (eg Glint Registers) can be found
DWORD   _cdecl  
RemapDataSegment(PDEVICETABLE pDev, 
                 DWORD DataSegment, 
                 DWORD OffsetGLInfo, 
                 DWORD VirtualMachine)
{
    DWORD       HWord, LWord, Result, i;
    DWORD       DSBase, DSLimit, Base, nPages, NewDSBase;
    DWORD       *Src, *Dst;
    DWORD       Segm, Off;
    LPGLINTINFO pGLInfo;

    __asm {
        pushad
        push    0
        push    VirtualMachine
        push    DataSegment
        VxDCall(_GetDescriptor)
        add     esp, 0xc
        mov     HWord, edx
        mov     LWord, eax
        popad
    }

    // Work out the base and the limit of the selector
    DSBase  = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
    DSLimit = (LWord & 0xffff) | (HWord &0xf0000);
    DISPDBG(("Old Data Segment Base: 0x%x Limit:0x%x", DSBase, DSLimit));

    nPages = FINAL_DATA_SEGMENT_SIZE >> 12;
#ifndef WIN98
    __asm {
        pushad
        push    PR_FIXED
        push    nPages
        push    PR_SHARED
        VxDCall(_PageReserve)
        add     esp, 0xc
        mov     NewDSBase, eax
        popad
    }
#else
	NewDSBase = (DWORD)_PageReserve(PR_SHARED, nPages, PR_FIXED);
#endif //#ifndef WIN98

    if (NewDSBase == 0xffffffff) 
    {
        DISPDBG(("PageReserve Failed"));
        return(0);
    }
    DISPDBG(("Sparse base reserved: 0x%x nPages: %d", NewDSBase, nPages));

    pDev->DataSegmentBase = NewDSBase;

    Base = NewDSBase >> 12;
    nPages = (DSLimit + 4095) >> 12;
#ifndef WIN98
    __asm {
        pushad
        push    (PC_FIXED + PC_USER + PC_WRITEABLE)
        push    0
        push    PD_FIXEDZERO
        push    nPages
        push    Base
        VxDCall(_PageCommit)
        add     esp, 0x14
        mov     Result, eax
        popad
    }
#else
	Result = (DWORD)_PageCommit(Base, nPages, PD_FIXEDZERO, 0, (PC_FIXED + PC_USER + PC_WRITEABLE));
#endif //#ifndef WIN98


    if (!Result) 
    {
        DISPDBG(("PageCommit Data Segment Alias Failed"));
        return(0);
    }

    // Copy the initialised data in the data segment into the alias segment
    Src = (DWORD *) DSBase;
    Dst = (DWORD *) NewDSBase;
    DSLimit = (DSLimit + 3) >> 2;
    for (i = 0; i < DSLimit; i++) 
    {
        Dst[i] = Src[i];
    }

    pDev->DataSegment = AllocateSelector((DWORD *)NewDSBase, FINAL_DATA_SEGMENT_SIZE>>12);

    DISPDBG(("Sparse Selector: 0x%x", pDev->DataSegment));

    // Now load the new data segment into all the registers
    pGLInfo = (LPGLINTINFO) ((char *) NewDSBase + OffsetGLInfo);
    pDev->pGLInfo = pGLInfo;

    // dwDMABufferPhys is a temporary list of the locations to store the new segment
    i = 0;
    while (pGLInfo->dwDMABufferPhys[i]) 
    {
        Segm = pGLInfo->dwDMABufferPhys[i] >> 16;
        Off = pGLInfo->dwDMABufferPhys[i] & 0xffff;
        __asm {
            pushad
            push    0
            push    VirtualMachine
            push    Segm
            VxDCall(_GetDescriptor)
            add     esp, 0xc
            mov     HWord, edx
            mov     LWord, eax
            popad
        }

        // Work out the base of the selector
        Base = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);

        *(DWORD *)(Base + Off) = pDev->DataSegment;

        i++;
    }

    return(NewDSBase);
}


DWORD   _cdecl  
ReinitialiseDataSegment(PDEVICETABLE pDev, DWORD DataSegment, DWORD VirtualMachine)
{
    DWORD   HWord, LWord, i;
    DWORD   DSBase, DSLimit, Base;
    DWORD   *Src, *Dst;
    DWORD   Segm, Off;

    DISPDBG(("ReinitialiseDataSegment"));

    __asm {
        pushad
        push    0
        push    VirtualMachine
        push    DataSegment
        VxDCall(_GetDescriptor)
        add     esp, 0xc
        mov     HWord, edx
        mov     LWord, eax
        popad
    }

    // Work out the base and the limit of the selector
    DSBase = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
    DSLimit = (LWord & 0xffff) | (HWord &0xf0000);
    DISPDBG(("Old Data Segment Base: 0x%x Limit:0x%x", DSBase, DSLimit));

    // Copy the initialised data in the data segment into the alias segment
    Src = (DWORD *) DSBase;
    Dst = (DWORD *) pDev->DataSegmentBase;
    DSLimit = (DSLimit + 3) >> 2;
    for (i = 0; i < DSLimit; i++) 
    {
        Dst[i] = Src[i];
    }

    DISPDBG(("Sparse Selector: 0x%x", pDev->DataSegment));

    // dwDMABufferPhys is a temporary list of the locations to store the new segment
    i = 0;
    while (pDev->pGLInfo->dwDMABufferPhys[i]) 
    {
        Segm = pDev->pGLInfo->dwDMABufferPhys[i] >> 16;
        Off = pDev->pGLInfo->dwDMABufferPhys[i] & 0xffff;
        __asm {
            pushad
            push    0
            push    VirtualMachine
            push    Segm
            VxDCall(_GetDescriptor)
            add     esp, 0xc
            mov     HWord, edx
            mov     LWord, eax
            popad
        }

        // Work out the base of the selector
        Base = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);

        *(DWORD *)(Base + Off) = pDev->DataSegment;

        i++;
    }

    return(pDev->DataSegmentBase);
}

PDEVICETABLE 
FindpDevFromDevNode(DEVNODE DevNode)
{
    ULONG       i;
    char        DevNodeIDBuffer[MAX_DEVICE_ID_LEN + 1];
    ULONG       Bus, Dev, Fn;
    CONFIGRET   cRet;

    // Wish for the trivial case where the device ID is in the list of ID's we know about.
    for (i = 0; i < MAX_GLINT_DEVICES; i++) 
    {

        if (DevTable[i].ChipID == NOCHIP_ID)
            break;

        if ((DevTable[i].DevNode == DevNode) || (DevTable[i].ActualDevNode == DevNode)) 
        {
            return(&(DevTable[i]));
        }
    }
	
    cRet = CM_Get_Device_ID(DevNode, DevNodeIDBuffer, MAX_DEVICE_ID_LEN + 1, 0);

    if (cRet != CR_SUCCESS)
    {
        DISPDBG(("FindpDevFromDevNode: Getting ID of Devnode failed - Error %d", cRet));
        return (NULL);
    }

    // We dont know which glint device we are running on. As a second attempt
    // at IDing the device we require, search for a non delta device on the same
    // Bus and Device ID as the current node

    GetBusDevFnNumbers(DevNodeIDBuffer, &Bus, &Dev, &Fn);

    DISPDBG(("FindpDevFromDevNode: %s Bs%x Dv%x Fn%x", DevNodeIDBuffer, Bus, Dev, Fn));
    for (i = 0; i < MAX_GLINT_DEVICES; i++) 
    {
        if (DevTable[i].ChipID == NOCHIP_ID)
            break;

        if ((DevTable[i].ChipID != DELTA_ID) &&
            (DevTable[i].PCIBus == (WORD)Bus) &&
            (DevTable[i].PCIDevice == (WORD)Dev)) 
        {
            // We have found a 3dlabs device on the same bus and function
            DISPDBG(("FindpDevFromDevNode: Found 3dlabs device on same bus and function"));
            DISPCHIP((&(DevTable[i])));
            // Update the devnode of the chip to be the devnode for the device
            // we are running on. This speeds up the next calls to this function
            // and gets the correct device settings from the registry.
            DevTable[i].DevNode = DevNode;
            return(&(DevTable[i]));
        }        
    }

    // We havnt found it. As a third pass,
    // find the first non delta device that hasnt been initialised
    for (i = 0; i < MAX_GLINT_DEVICES; i++) 
    {
	if (DevTable[i].ChipID == NOCHIP_ID)
            break;

        if ((DevTable[i].ChipID != DELTA_ID) && (DevTable[i].pRegisters == 0)) 
        {
            // Point the DevNode to the correct place to get registry information.
            DISPDBG(("FindpDevFromDevNode: Found first uninitialised 3dlabs device"));
            DISPCHIP((&(DevTable[i])));
            DevTable[i].DevNode = DevNode;
            return(&(DevTable[i]));
        }
    }

    // We still havnt found it. 
    // As a final pass, find the first non delta device
    for (i = 0; i < MAX_GLINT_DEVICES; i++) 
    {
        if (DevTable[i].ChipID == NOCHIP_ID)
            break;

 	if (DevTable[i].ChipID != DELTA_ID) 
        {
            // Point the DevNode to the correct place to get registry information.
            DISPDBG(("FindpDevFromDevNode: Found first 3dlabs device"));
            DISPCHIP((&(DevTable[i])));
            DevTable[i].DevNode = DevNode;
            return(&(DevTable[i]));
        }
    }

    DISPDBG(("FindpDevFromDevNode: Failed to find 3dlabs device"));
    return(NULL);
}

PDEVICETABLE _cdecl
FindpDevFromDevNodeTable(DEVNODE DevNode)
{
    ULONG   i;

    for (i = 0; i < MAX_GLINT_DEVICES; i++) 
    {
        if (DevTable[i].ChipID == NOCHIP_ID)
            return(NULL);
        if ((DevTable[i].DevNode == DevNode) || (DevTable[i].ActualDevNode == DevNode)) 
        {
            return(&(DevTable[i]));
        }
    }
    return(NULL);
}

PDEVICETABLE _cdecl 
FindPrimarypDev()
{
    ULONG   i;

    // We dont know which glint device we are running on. 
    // Find the first non delta device
    for (i = 0; i < MAX_GLINT_DEVICES; i++) 
    {
        if (DevTable[i].ChipID == NOCHIP_ID)
            return(NULL);
        return(&(DevTable[i]));
    }
    return(NULL);
}

DWORD _cdecl 
FindDevNodeForCurrentlyActiveVGA()
{
    ULONG   i;

    // I am sure there is a better way to do this, but this seems the easiest.
    // We need to find the pDev for the currently active VGA. Run through each
    // of the chips reading their command registers. Return the first one which
    // has its io switched on.

    // We dont know which glint device we are running on. 
    // Find the first non delta device
    for (i = 0; i < MAX_GLINT_DEVICES; i++) 
    {

        if (DevTable[i].ChipID == NOCHIP_ID)
            return(0);

        // Check the io bit
        CM_Call_Enumerator_Function(DevTable[i].DevNode, 
                                    PCI_ENUM_FUNC_GET_DEVICE_INFO,
			            0x4, 
                                    &(DevTable[i].PCIConfig.Command), 
                                    0x2, 
                                    0);

        // If the io bit is set in the command register, this must be the VGA.
        if (DevTable[i].PCIConfig.Command & PCI_ENABLE_IO_SPACE)
            return(DevTable[i].DevNode);
    }
    return(0);
}


DWORD   _cdecl  
RegisterDisplayDriver(CLIENT_STRUCT *Regs)
{
    PDEVICETABLE    pDev;
    DIOCPARAMETERS  dioc;

    DISPDBG(("RegisterDisplayDriver called"));

    switch (Regs->CRS.Client_ECX) 
    {
        case MINIVDD_SHAREGLINFO:
        case MINIVDD_GETGLINFO:
        {
            DWORD   dsBase;

            // LocateHardwareNode should look at all the devices on the system
            // and fill in a table containing a list of all GLINT compatible devices.
            if (LocateHardwareNode(DevTable) == 0) 
            {
                DISPDBG(( "No Glint Devices found"));
                break;
            }

            // Get the pDev for the requested device.
            if ((pDev = FindpDevFromDevNode(Regs->CRS.Client_EDX)) == NULL) 
            {
                DISPDBG(("Can not locate Glint Device"));
                break;
            }
            DISPDBG(("MINIVDD_GETGLINFO"));
            DISPCHIP((pDev));

            // Has HW node been initialised yet?
            if (!pDev->pGLInfo) 
            {   // Client_SI is offset within data segment of GLInfo.
                dsBase = RemapDataSegment(pDev, Regs->CRS.Client_DS, Regs->CWRS.Client_SI, WindowsVMHandle);

                pDev->pGLInfo = (LPGLINTINFO) (dsBase + Regs->CWRS.Client_SI);
                pDev->pGLInfo->dwDSBase = dsBase;

                if (Regs->CRS.Client_ECX == MINIVDD_SHAREGLINFO) 
                {   // Destructive initialisation.
                    InitialiseHardware(pDev, 1);
                }
                else 
                {   // Nondestructive initialisation.
                    InitialiseHardware(pDev, 0);
                }
            }
            else 
            {
                dsBase = ReinitialiseDataSegment(pDev, Regs->CRS.Client_DS, WindowsVMHandle);
                pDev->pGLInfo = (LPGLINTINFO) (dsBase + Regs->CWRS.Client_SI);
                pDev->pGLInfo->dwDSBase = dsBase;
            }

	    InitialiseSharedData(pDev);

            Regs->CRS.Client_EAX = 0;
            return(1);
        } //End of case MINIVDD_GETGLINFO: case MINIVDD_SHAREGLINFO:

        case MINIVDD_INITIALISEMODE:
        // Get the pDev for the requested device.
        if ((pDev = FindpDevFromDevNode(Regs->CRS.Client_EDX)) == 0) {
            DISPDBG(("Can not locate Glint Device"));
            break;
        }

        DISPDBG(("MINIVDD_INITIALISEMODE"));
        DISPCHIP((pDev));

        if (!pDev->pGLInfo) {
            DISPDBG(("No pGLInfo"));
            break;
        }

        if (!InitialiseMode(pDev)) {
            DISPDBG(("InitialiseMode failed"));
            break;
        }

        Regs->CRS.Client_EAX = 0;
        return(1); // End of case MINIVDD_INITIALISEMODE:

        case MINIVDD_ALLOCATEMEMORY:
        Regs->CRS.Client_EAX = AllocateGlobalMemory(Regs->CRS.Client_EDX);
        return(1); // End of case MINIVDD_ALLOCATEMEMORY:

        case MINIVDD_FREEMEMORY:
        // As we are freeing a selector, make sure none of the client selectors are actually
        // set to this value 
        if (Regs->CWRS.Client_DX == Regs->CRS.Client_DS) Regs->CRS.Client_DS = 0;
        if (Regs->CWRS.Client_DX == Regs->CRS.Client_ES) Regs->CRS.Client_ES = 0;
        if (Regs->CWRS.Client_DX == Regs->CRS.Client_FS) Regs->CRS.Client_FS = 0;
        if (Regs->CWRS.Client_DX == Regs->CRS.Client_GS) Regs->CRS.Client_GS = 0;
        Regs->CRS.Client_EAX = FreeGlobalMemory(Regs->CWRS.Client_DX);
        return(1); //End of case MINIVDD_FREEMEMORY:
	
	case MINIVDD_VMIREQUEST:
        {
	    extern  BOOL DoVMIRequest(  PDEVICETABLE pDev, 
                                        LPVMIREQUEST pI2CIn, 
                                        LPVMIREQUEST pI2COut);
	    DWORD   pInput;
	    DWORD   pOutput;
	    DWORD   HWord;
	    DWORD   LWord;

	    // We must first translate the 16 bit memory passed in into a 32 bit pointer...
	    // Get the selectors for the input and output buffers.
	    DWORD ClientIn  = (Regs->CRS.Client_ESI >> 16);
	    DWORD ClientOut = (Regs->CRS.Client_EDX >> 16);
	    DWORD OffsetIn  = (Regs->CRS.Client_ESI & 0xFFFF);
	    DWORD OffsetOut = (Regs->CRS.Client_EDX & 0xFFFF);
	    
	    DISPDBG(("** In MINIVDD_VMIREQUEST"));
	    DISPDBG(("  ClientIn: 0x%x", ClientIn));
	    DISPDBG(("  ClientOut: 0x%x", ClientOut));
	    
	    // Get the input selector
	    __asm 
	    {
		pushad
		push    0
		push    WindowsVMHandle
		push    ClientIn
		VxDCall(_GetDescriptor)
		add     esp, 0xc
		mov     HWord, edx
		mov     LWord, eax
		popad
	    }
	    // Work out the base and the limit of the selector
	    pInput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
	    pInput += OffsetIn;
	    DISPDBG(("Input pointer: 0x%x", pInput));
	        
	    // If the input selector is the same as the output, then don't bother
	    // calling VMM to do the conversion again.  Simply apply the new offset
	    if (ClientIn == ClientOut)
	    {
		pOutput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
		pOutput += OffsetOut;
	    }
	    else
	    {
		// Get the output selector
		__asm 
		{
		    pushad
		    push    0
		    push    WindowsVMHandle
		    push    ClientOut
		    VxDCall(_GetDescriptor)
		    add     esp, 0xc
		    mov     HWord, edx
		    mov     LWord, eax
		    popad
		}

		// Work out the base and the limit of the selector
		pOutput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
		pOutput += OffsetOut;
	    }
	    DISPDBG(("Output pointer: 0x%x", pOutput));
	        
            // Now we can point at the input structures, find the PDev
            if ((pDev = FindpDevFromDevNode(Regs->CRS.Client_EDI)) == 0) 
            {
	        DISPDBG(("Cannot locate Glint Device"));
	        break;
            }

	    // Will fall out and return an error if not sucesfull
	    if (DoVMIRequest(pDev, (LPVMIREQUEST)pInput, (LPVMIREQUEST)pOutput)) return (1);
        }
        break; //End of case MINIVDD_VMIREQUEST:

	case MINIVDD_I2CREQUEST:
	{
	    extern BOOL DoI2C(PDEVICETABLE pDev, LPI2CREQUEST pI2CIn, LPI2CREQUEST pI2COut);
	    DWORD pInput;
	    DWORD pOutput;
	    DWORD HWord;
	    DWORD LWord;

	    // We must first translate the 16 bit memory passed in into a 32 bit pointer...
	    // Get the selectors for the input and output buffers.
	    DWORD ClientIn = (Regs->CRS.Client_ESI >> 16);
	    DWORD ClientOut = (Regs->CRS.Client_EDX >> 16);
	    DWORD OffsetIn = (Regs->CRS.Client_ESI & 0xFFFF);
	    DWORD OffsetOut = (Regs->CRS.Client_EDX & 0xFFFF);
	    
	    DISPDBG(("** In MINIVDD_I2CREQUEST"));
	    DISPDBG(("  ClientIn: 0x%x", ClientIn));
	    DISPDBG(("  ClientOut: 0x%x", ClientOut));
	    
	    // Get the input selector
	    __asm 
	    {
		pushad
		push    0
		push    WindowsVMHandle
		push    ClientIn
		VxDCall(_GetDescriptor)
		add     esp, 0xc
		mov     HWord, edx
		mov     LWord, eax
		popad
	    }
	    // Work out the base and the limit of the selector
	    pInput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
	    pInput += OffsetIn;
	    DISPDBG(("Input pointer: 0x%x", pInput));
	    
	    // If the input selector is the same as the output, then don't bother
	    // calling VMM to do the conversion again.  Simply apply the new offset
	    if (ClientIn == ClientOut)
	    {
		    pOutput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
		    pOutput += OffsetOut;
	    }
	    else
	    {
		// Get the output selector
		__asm 
		{
		    pushad
		    push    0
		    push    WindowsVMHandle
		    push    ClientOut
		    VxDCall(_GetDescriptor)
		    add     esp, 0xc
		    mov     HWord, edx
		    mov     LWord, eax
		    popad
		}

		// Work out the base and the limit of the selector
		pOutput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
		pOutput += OffsetOut;
	    }
	    DISPDBG(("Output pointer: 0x%x", pOutput));
	
	    // Now we can point at the input structures, find the PDev
	    if ((pDev = FindpDevFromDevNode(Regs->CRS.Client_EDI)) == 0) 
	    {
		DISPDBG(("Cannot locate Glint Device"));
		break;
            }

	    // Will fall out and return an error if not sucesfull
	    if (DoI2C(pDev, (LPI2CREQUEST)pInput, (LPI2CREQUEST)pOutput)) return (1);
	}
	break; //End of case MINIVDD_I2CREQUEST:

	case MINIVDD_MEMORYREQUEST:
	{
	    DWORD pInput;
	    DWORD pOutput;
	    DWORD HWord;
	    DWORD LWord;
	    // Get the selectors for the input and output buffers.
	    DWORD ClientIn = (Regs->CRS.Client_ESI >> 16);
	    DWORD ClientOut = (Regs->CRS.Client_EDX >> 16);
	    DWORD OffsetIn = (Regs->CRS.Client_ESI & 0xFFFF);
	    DWORD OffsetOut = (Regs->CRS.Client_EDX & 0xFFFF);
	    extern DWORD _stdcall VXD3DL_MemoryRequest(DWORD dwDDB, DWORD hDevice, DIOCPARAMETERS* lpDIOCParms);

	    DISPDBG(("ClientIn: 0x%x", ClientIn));
	    DISPDBG(("ClientOut: 0x%x", ClientOut));
	    // Get the input selector
	    __asm 
	    {
		pushad
		push    0
		push    WindowsVMHandle
		push    ClientIn
		VxDCall(_GetDescriptor)
		add     esp, 0xc
		mov     HWord, edx
		mov     LWord, eax
		popad
	    }
	    // Work out the base and the limit of the selector
	    pInput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
	    pInput += OffsetIn;
	    DISPDBG(("Input pointer: 0x%x", pInput));
		
            // If the input selector is the same as the output, then don't bother
            // calling VMM to do the conversion again.  Simply apply the new offset
            if (ClientIn == ClientOut)
            {
                pOutput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
                pOutput += OffsetOut;
            }
            else
	    {
		// Get the output selector
		__asm 
		{
		    pushad
		    push    0
		    push    WindowsVMHandle
		    push    ClientOut
		    VxDCall(_GetDescriptor)
		    add     esp, 0xc
		    mov     HWord, edx
		    mov     LWord, eax
		    popad
		}

		// Work out the base and the limit of the selector
		pOutput = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
		pOutput += OffsetOut;
	    }
            DISPDBG(("Output pointer: 0x%x", pOutput));

            dioc.lpvInBuffer = pInput;
            dioc.lpvOutBuffer = pOutput;
            if (!VXD3DL_MemoryRequest(0, 0, &dioc))
            {
                Regs->CRS.Client_EAX = 0x1;
            }
            else
            {
                Regs->CRS.Client_EAX = 0;
            }

            // Ensure that the selector isn't in a client register.
            if (ClientIn == Regs->CRS.Client_DS) Regs->CRS.Client_DS = 0;
	    if (ClientIn == Regs->CRS.Client_ES) Regs->CRS.Client_ES = 0;
	    if (ClientIn == Regs->CRS.Client_FS) Regs->CRS.Client_FS = 0;
	    if (ClientIn == Regs->CRS.Client_GS) Regs->CRS.Client_GS = 0;

            if (ClientOut != ClientIn)
            {
                if (ClientOut == Regs->CRS.Client_DS) Regs->CRS.Client_DS = 0;
                if (ClientOut == Regs->CRS.Client_ES) Regs->CRS.Client_ES = 0;
                if (ClientOut == Regs->CRS.Client_FS) Regs->CRS.Client_FS = 0;
                if (ClientOut == Regs->CRS.Client_GS) Regs->CRS.Client_GS = 0;
            }

	    return 1;
        } //End of case MINIVDD_MEMORYREQUEST:

    case MINIVDD_ENABLEVGA:     // called by driver's Disable function
                                // thru VDD_REGISTER_DISPLAY_DRIVER_INFO
                                // instead of using MiniVDD_DiplayDriverDisabling
                                // because MiniVDD_DiplayDriverDisabling is not
                                // called fro secondary devices ???!!!!#*&^&@

        if (Regs->CRS.Client_EBX == 1)
        {
            pDev = FindPrimarypDev();
        }
        else
        {
            pDev = FindpDevFromDevNodeTable(Regs->CRS.Client_EBX);
        }
        EnableVGA(pDev);    // disable interrupt on device, unvirtualize interrupt,
                            // set chip into VGA mode
	    return 1;
    

    }

    // A general failure condition
    Regs->CRS.Client_EAX = 0xffffffff;
    return(0);
}

DWORD   _cdecl  
PNP_NewDevnode(DWORD Command, DWORD DevNode, DWORD LoadType)
{
    PDEVICETABLE pDev;

    DISPDBG(("PNP_NewDevnode: 0x%x 0x%x 0x%x", Command, DevNode, LoadType));

    pDev = InitialiseDeviceNode(DevNode);

    if (!pDev) 
    {   // This may be because it has already been enumerated. Check to see if its in the table.
        pDev = FindpDevFromDevNodeTable(DevNode);
    }

    if (!pDev) 
    {
        DWORD ulSize = 1, ulValue = 0;

        // Check to see if this is a vga host chip (eg avance VGA on Racer)
        if (CM_Read_Registry_Value( DevNode, 
                                    szDefaultKey, 
				    szVGAHost, 
                                    REG_BINARY, 
                                    &ulValue, 
                                    &ulSize,
				    CM_REGISTRY_SOFTWARE) != CR_SUCCESS) 
        {
            return(0);// PNP_NewDevNode for device which isnt a 3dlabs host vga chip
        }
        DISPDBG(("PNP_NewDevnode on a 3Dlabs VGA Host"));
    }
    else 
    {
        // Final check to ensure something strange hasnt happened.
        if (pDev->DevNode != DevNode)
            return(0);
    }

    // Initialise this device as a secondary. We may not have a valid pDev though.
    DISPDBG(("Initialising as secondary device"));
    InitialiseSecondaryDispatchTable();
    return(1);
}


DWORD _cdecl 
HandleInterrupt(PDEVICETABLE pDev)
{
    ULONG           intrFlags;
    ULONG           enableFlags;
    ULONG           backIndex;
    ULONG           Temp;

//  removed loop thru DevTable because pDev is now passed in

        // find out what caused the interrupt. We AND with the enabled interrupts
        // since the flags are set if the event occurred even though no interrupt was
        // enabled.
        //
    READ_GLINT_CTRL_REG(IntFlags, intrFlags);
    READ_GLINT_CTRL_REG(IntEnable, enableFlags);
    if (pDev->dwFlags & GMVF_DELTA)
    {
        READ_GLINT_CTRL_REG(DeltaIntFlags, Temp);
        intrFlags |= Temp;
        READ_GLINT_CTRL_REG(DeltaIntEnable, Temp);
        enableFlags |= Temp;
    }
    intrFlags &= enableFlags;   // non-zero if this chip has an interrupt active

//
    if (intrFlags != 0)         // is the int from this chip?
    {
        //DISPDBG(( "Got Interrupt flags: 0x%x", intrFlags));

        // clear the interrupts we detected. Read to flush the write.
        LOAD_GLINT_CTRL_REG(IntFlags, intrFlags);
        if (pDev->dwFlags & GMVF_DELTA)
        {
	        LOAD_GLINT_CTRL_REG(DeltaIntFlags, intrFlags);
        }
        READ_GLINT_CTRL_REG(IntFlags, Temp);

	    // Check for VBlank. On a P2 we want to change the the cursor under a VBLANK
	    if (intrFlags & INTR_VBLANK_SET) 
        {
	        pDev->pGLInfo->dwFlags |= GMVF_VBLANK_OCCURED; 

#ifdef USE_HARDWARECURSOR
            // Top bit of Cursor control will be set if we have to update cursors.
            if (pDev->pGLInfo->wCursorControl & 0x8000) 
            {
                pTVP4020RAMDAC   pTVP4020Regs = (pTVP4020RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);					

                pDev->pGLInfo->wCursorControl &= 0xFF;		// Remove update bit
                TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, pDev->pGLInfo->wCursorControl);
            }
#endif // #ifdef USE_HARDWARECURSOR

            // Support for DDFLIP_INTERVALN
#ifdef DDFLIP_INTERVALN
            pDev->pGLInfo->InterruptCount++;
            if (pDev->pGLInfo->InterruptCount > MAX_P2INTERRUPT_COUNT)
                pDev->pGLInfo->InterruptCount = MAX_P2INTERRUPT_COUNT;
#endif //#ifdef DDFLIP_INTERVALN
	    }

		// the error interrupt occurs each time the display driver adds an entry
        // to the queue. We treat it exactly as though we had received a DMA
        // interrupt.
        //
        if (intrFlags & (INTR_DMA_SET|INTR_ERROR_SET))
        {   // If the previous DMA has not completed we can't do anything. We've
            // cleared the interrupt flag for this interrupt so even if the DMA
            // completes before we return, we'll immediately get another
            // interrupt. Since we will be getting another interrupt we do not
            // have to clear the InterruptPending flag.
            //
	        READ_GLINT_CTRL_REG(DMACount, Temp);
	        if (Temp != 0) 
            {
                VPICD_Phys_EOI(pDev->IRQHandle);
                return 1;
            }

            // We may return without starting any DMA and hence not expecting any
            // interrupt so clear the InterruptPending flag. This will force the
            // display driver to wake us. MUST DO THIS BEFORE CHECKING THE QUEUE.
            // Since the display driver adds entries and then checks the flag, we
            // must do it in the reverse order.
            //
            pDev->pGLInfo->InterruptPending = 0;

            // if the DMA queue is empty then we have nothing to do
            backIndex = pDev->pGLInfo->backIndex;
            if (pDev->pGLInfo->frontIndex != backIndex)
            {
		        // Since we know we'll get a DMA interrupt, we don't need a wakeup so
		        // set the InterruptPending flag to true.
		        //
		        pDev->pGLInfo->InterruptPending = 1;

		        LOAD_GLINT_CTRL_REG(DMAAddress, pDev->pGLInfo->QAddress[backIndex]);
		        LOAD_GLINT_CTRL_REG(DMACount,   pDev->pGLInfo->QCount[backIndex]);

		        // now remove the entry from the queue
		        if (++backIndex == pDev->pGLInfo->endIndex)
		            backIndex = 0;
		        pDev->pGLInfo->backIndex = backIndex;
	        }            
	    }

	    // Record the polarity of the VideoStream.
	    if (intrFlags & INTR_VIDSTREAM_A_SET)
        {
            DWORD   dwStatus;
            DWORD   dwAddrIndex;
            DWORD   dwMask;

            // Signal that a VBLANK interrupt of VideoStream A has occured.
            pDev->pGLInfo->dwFlags |= GMVF_VSA_INTERRUPT_OCCURED;

            // Get the current video index.
            READ_GLINT_CTRL_REG(VSAVideoAddressIndex, dwAddrIndex);

            if (pDev->pGLInfo->dwVSAPrevIndex != dwAddrIndex)
            {   // Record the last index we saw
                pDev->pGLInfo->dwVSAPrevIndex = dwAddrIndex;

                dwMask = (1 << dwAddrIndex);
                READ_GLINT_CTRL_REG(VSStatus, dwStatus);

                // Fill 3 polarity bits in dwVSAPolarity for later extraction
                // of the current video field
                dwStatus >>= (9 - dwAddrIndex);
                dwStatus &= dwMask;

                pDev->pGLInfo->dwVSAPolarity &= ~(dwMask);
                pDev->pGLInfo->dwVSAPolarity |= dwStatus;
            }
        }

        VPICD_Phys_EOI(pDev->IRQHandle);
        return(1);

    } // end interrupt is from this chip

    return 0;   // interrupt is not from this chip

} /* HandleInterrupt */


