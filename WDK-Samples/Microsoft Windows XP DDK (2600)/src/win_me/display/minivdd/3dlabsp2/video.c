/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: video.c
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


#include "minivdd.h"

#define	MaxChipClockRate 100

VOID DisableVGALow(PDEVICETABLE pDev);

typedef struct 
{
    UCHAR       Min;    // Min Refresh Rate
    UCHAR       Max;    // Max Refresh Rate
    USHORT      HTot;	// Hor Total Time
    UCHAR       HFP;	// Hor Front Porch
    UCHAR       HST;	// Hor Sync Time
    UCHAR       HBP;	// Hor Back Porch
    UCHAR       HSP;	// Hor Sync Polarity
    USHORT      VTot;	// Ver Total Time   
    UCHAR       VFP;	// Ver Front Porch  
    UCHAR       VST;	// Ver Sync Time    
    UCHAR       VBP;	// Ver Back Porch   
    UCHAR       VSP;	// Ver Sync Polarity
} VESA_TIMING_STANDARD;

// Entries for 3 bpp colors
//		Index(0-7) -> Color(0-255)
ULONG   bPal8[] = 
{
    0x00, 0x24, 0x48, 0x6D,
    0x91, 0xB6, 0xDA, 0xFF
};

// Entries for 2 bpp colors
//		Index(0-3) -> Color(0-255)
ULONG   bPal4[] = 
{
    0x00, 0x6D, 0xB6, 0xFF
};

#define SET_P2_VS_MODE(mode)                                                \
    if (pDev->ChipID == PERMEDIA2_ID || pDev->ChipID == TIPERMEDIA2_ID) {   \
        ULONG ulValue1;                                                     \
        READ_GLINT_CTRL_REG(VSConfiguration, ulValue1);                     \
        ulValue1 = (ulValue1 & ~0xf) | (mode);                              \
        LOAD_GLINT_CTRL_REG(VSConfiguration, ulValue1);                     \
        DISPDBG(("Video Streams Interface set to mode %d", (mode)));      \
    }
#define VS_ROM_MODE         0
#define VS_RAMDAC_MODE      1
#define VS_VIDEO_MODE       7


void 
PermediaCalculateScreenBase(PDEVICETABLE pDev)
{
    DWORD   ScreenSize, ScreenBase;

    ScreenSize = pDev->pGLInfo->dwScreenWidthBytes * pDev->pGLInfo->dwScreenHeight;
    ScreenBase = 0;

    if (pDev->dwFlags & GMVF_TRYTOVIRTUALISE4PLANEVGA) 
    {
        if (pDev->pGLInfo->dwBpp == 24) 
        {   // Screen needs to start on a 12 byte boundary (for pixel alignment)
            // and a 8 byte boundary (for ScreenBase alignment)
            ScreenBase = 0x20010;
        }
        else 
        {   // Screen starts on 8 byte boundary.
            ScreenBase = 0x20000;
        }

        // Check to see if there is size for the VGA virtualisation region.
        if (ScreenSize + ScreenBase > pDev->pGLInfo->ddFBSize) 
        {
            ScreenBase = 0;
            pDev->pGLInfo->dwFlags &= ~GMVF_VIRTUALISE4PLANEVGA;
        }
        else 
        {
            pDev->pGLInfo->dwFlags |= GMVF_VIRTUALISE4PLANEVGA;
        }
    }

    pDev->pGLInfo->dwScreenBase         = ScreenBase;
    pDev->pGLInfo->dwOffscreenBase      = ScreenBase + ScreenSize;
    pDev->pGLInfo->dwOSFontCacheStart   = pDev->pGLInfo->ddFBSize;

    return;
}

//
// find out what type of RAMDAC we have
//
DWORD 
FindRamDacType(PDEVICETABLE pDev)
{
        pDev->DacId = TVP4020_RAMDAC;
        pDev->ActualDacId = TVP4020_RAMDAC;

	DISPDBG(("PERMEDIA2: using INTERNAL TVP4020 RAMDAC"));
        return(1);
}

#define INITIALFREQERR 100000

// TVP4020 PLLs set up differently than TVP3026
ULONG
TVP4020_CalculateMNPForClock(   ULONG RefClock,		// In 100Hz units
                                ULONG ReqClock,		// In 100Hz units
                                ULONG MinClock,		// Min VCO rating
                                ULONG MaxClock,		// Max VCO rating
                                ULONG *rM, 			// M Out
                                ULONG *rN, 			// N Out
                                ULONG *rP			// P Out
                                )
{
    ULONG       M, N, P;
    ULONG       VCO, Clock;
    LONG	freqErr, lowestFreqErr = INITIALFREQERR;
    LONG        AcceptableError = 0;                    // initially accept no error
    LONG        AcceptableErrorDelta = ReqClock / 800;  // delta of .125% of error for each N
    ULONG       ActualClock = 0;

    DISPDBG(("TVP4020 Locating MNP for %d00Hz with RefClock %d00Hz", ReqClock, RefClock));

    for (N = 2; N <= 14; N++) 
    {
	for (M = 2; M <= 255; M++) 
        {
	    VCO = ((RefClock * M) / N);
	    if ((VCO < MinClock) || (VCO > MaxClock))
		continue;
	    for (P = 0; P <= 4; P++) 
            {
		Clock = VCO >> P;
		freqErr = (Clock - ReqClock);
                if (freqErr	< 0)
                    freqErr = -freqErr;
		if (freqErr < lowestFreqErr) 
                {  // Only replace if error is less; keep N small!
                    *rM = M;
                    *rN = N;
                    *rP = P;
                    ActualClock = Clock;
                    lowestFreqErr = freqErr;
                    DISPDBG(("Ref: %d Req: %d Found Freq %d, M%d N%d P%d", RefClock, ReqClock, Clock, M, N, P));
		}
	    }
	}
        // Return if we found a match within current accepted error range. This will bias our
        // decision for a lower N
        if (lowestFreqErr < AcceptableError) 
        {
            DISPDBG(("Accepted clock %d. Error %d, Accepting %d at N=%d", ActualClock, lowestFreqErr, AcceptableError, N));
            return(ActualClock);
        }

        AcceptableError += AcceptableErrorDelta;
    }

    DISPDBG(("Accepted clock %d. Error %d, Accepting %d at N=%d", ActualClock, lowestFreqErr, AcceptableError, N));
    return(ActualClock);
}

DWORD TVP4020_ProgramSystemClock(PDEVICETABLE pDev, ULONG RefClkSpeed, ULONG SystemClock)
{
    ULONG	    M, N, P;
    ULONG           ReadResult;
    pTVP4020RAMDAC  pTVP4020Regs = (pTVP4020RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);

    // Program system clock. This controls the speed of the Glint or Permedia.
    SystemClock = TVP4020_CalculateMNPForClock( RefClkSpeed,    // In 100Hz units
                                                SystemClock,    // In 100Hz units
                                                1100000,        // Min VCO rating
                                                2500000,	// Max VCO rating
                                                &M, 		// M Out
                                                &N, 		// N Out
                                                &P);  		// P Out
    if (SystemClock == 0)
    {
        DISPDBG(("TVP4020_CalculateMNPForClock failed"));
        return(FALSE);
    }
    // Can change P2 MCLK directly without switching to PCLK
    // Program the Mclk PLL
    TVP4020_WRITE_INDEX_REG(__TVP4020_MEMCLK_REG_3, 0x6);       // Stop the system clock
    TVP4020_WRITE_INDEX_REG(__TVP4020_MEMCLK_REG_2, N);         // N
    TVP4020_WRITE_INDEX_REG(__TVP4020_MEMCLK_REG_1, M);		// M
    TVP4020_WRITE_INDEX_REG(__TVP4020_MEMCLK_REG_3, P | 0x08);  // P / Enable

    M = 1000;
    ReadResult = 0;
    do 
    {
        N = 10000;
        do 
        {
	    TVP4020_READ_INDEX_REG(__TVP4020_MEMCLK_STATUS, ReadResult);
        } while ((!(ReadResult & (1 << 4))) && (--N));
    } while ((!(ReadResult & (1 << 4))) && (--M));

    DISPDBG(("MemClock Synced M:%d N:%d", M, N));

    return(SystemClock);
}

DWORD 
TVP4020_WaitForPixelClockToSync(PDEVICETABLE pDev)
{
    ULONG	    M, N;
    ULONG           ulValue;
    pTVP4020RAMDAC  pTVP4020Regs = (pTVP4020RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);

    M = 1000;
    ulValue = 0;

    do 
    {
        N = 10000;
        do 
        {
	    TVP4020_READ_INDEX_REG(__TVP4020_PIXCLK_STATUS, ulValue);
        } while ((!(ulValue & (1 << 4))) && (--N));
    } while ((!(ulValue & (1 << 4))) && (--M));

    DISPDBG(("PixelClock Synced M:%d N:%d", M, N));
    if (M == 0) 
    {
	return(0);
    }
    return(1);
}


DWORD 
TVP4020_ProgramPixelClock(PDEVICETABLE  pDev, 
                          ULONG         RefClkSpeed, 
                          ULONG         PixelClock)
{
    ULONG	     M, N, P;
    pTVP4020RAMDAC   pTVP4020Regs = (pTVP4020RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);
    // Program Pixel Clock to the correct value for the required resolution
    PixelClock = TVP4020_CalculateMNPForClock(  RefClkSpeed,	// In 100Hz units
                                                PixelClock,	// In 100Hz units
                                                1500000,		// Min VCO rating
                                                3000000,		// Max VCO rating
                                                &M, 			// M Out
                                                &N, 			// N Out
                                                &P);			// P Out
    if (PixelClock == 0)
    {
        DISPDBG(("TVP4020_CalculateMNPForClock failed"));
        return(FALSE);
    }
    // Pixel Clock
    switch (pDev->dwVClkCtl & 3) 
    {
        case 0:
        DISPDBG(("TVP4020 Programming Pixel Clock Register set A"));
            TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_A3, 0x6);     // Disable the Pixel Clock
            LOAD_GLINT_CTRL_REG(VClkCtl, pDev->dwVClkCtl);
            TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_A2, N);       // N
            TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_A1, M);	   // M
            TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_A3, P | 0x8); // P / Enable
        break;

        case 1:
        DISPDBG(("TVP4020 Programming Pixel Clock Register set B"));
	        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_B3, 0x6);     // Disable the Pixel Clock
	        LOAD_GLINT_CTRL_REG(VClkCtl, pDev->dwVClkCtl);
	        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_B2, N);       // N
	        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_B1, M);	   // M
	        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_B3, P | 0x8); // P / Enable
        break;

        default:
        DISPDBG(("TVP4020 Programming Pixel Clock Register set C"));
	        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_C3, 0x6);     // Disable the Pixel Clock
	        LOAD_GLINT_CTRL_REG(VClkCtl, pDev->dwVClkCtl);
	        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_C2, N);       // N
	        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_C1, M);	   // M
	        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_C3, P | 0x8); // P / Enable
        break;
    }
    TVP4020_WaitForPixelClockToSync(pDev);    
    return(PixelClock);// No Loop Clock on P2
}

DWORD _cdecl 
InitialiseVideo(PDEVICETABLE pDev)
{
    ULONG       index;
    ULONG       ulValue, ulSize;
    ULONG       pixelCtrl;
    ULONG	prefRefreshRate;
    ULONG	dShift;
    VESA_TIMING_STANDARD	VESATimings;
    ULONG	Htot, Hss, Hse, Hbe, Hsp;
    ULONG	Vtot, Vss, Vse, Vbe, Vsp;
    ULONG	PixelClock, Freq;
    ULONG	RefClkSpeed, SystemClock;	// Speed of clocks in 100Hz units
    ULONG	M;
    ULONG       SystemClockInitialised=0;
    char	szModeKey[32];				// Template for the registry mode key
    char	szFreqValue[8];				// Template for frequency value string
    extern char szDefaultKey[], szRefClk[], szSysClk[], szPrefRefresh[];
    ULONG	depth, xRes, yRes;
    pTVP4020RAMDAC   pTVP4020Regs = (pTVP4020RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);

    depth = pDev->pGLInfo->DesktopDisplayInfo.ddiBpp;
    xRes  = pDev->pGLInfo->DesktopDisplayInfo.ddiXRes;
    yRes  = pDev->pGLInfo->DesktopDisplayInfo.ddiYRes;

    // Just in case we get called with really weird values in the pDev->pGLInfo->DesktopDisplayInfo
    if (xRes < 320)
        xRes = 640;
    if (yRes < 200)
        yRes = 480;
    if (depth < 8)
        depth = 16;

    // Check there is enough video memory to support the mode
    ulSize = xRes * yRes * (depth >> 3);
    if (ulSize > pDev->dwFrameBufferSize) 
    {
        DISPDBG(("Not enough memory to initiate mode %dx%d %dbpp", xRes, yRes, depth));
        return(0);
    }

    // Choose suitable refresh rate.
    Freq = 0;// Assume automatic value unless otherwise informed.
    {
        HKEY    hKey;
        if (_RegOpenKey(HKEY_LOCAL_MACHINE, 
                        pDev->szInformationKey, 
                        &hKey) != ERROR_SUCCESS) 
        {
            DISPDBG(("Couldnt open HKLM\\%s", pDev->szInformationKey));
        }

	ulSize = 4;
	if (_RegQueryValueEx(   hKey, 
                                szPrefRefresh, 
                                NULL, 
                                NULL, 
                                &prefRefreshRate, 
                                &ulSize) == ERROR_SUCCESS) 
        {
	    if (prefRefreshRate != 0xFF) 
            {
		Freq = prefRefreshRate;		// Use user's preferred value.
		DISPDBG(("PreferredRefreshRate: %d Hz", Freq));
	    }
	}

        if (_RegCloseKey(hKey) != ERROR_SUCCESS)
            DISPDBG(("Error closing registry key"));
    }

    if (!(pDev->pGLInfo->DesktopDisplayInfo.ddiInfoFlags &
	    (MONITOR_DEVNODE_NOT_ACTIVE | MONITOR_INFO_NOT_VALID | MONITOR_INFO_DISABLED_BY_USER)))
    {  
        ULONG   Max = pDev->pGLInfo->DesktopDisplayInfo.ddiRefreshRateMax;
        ULONG   Min = pDev->pGLInfo->DesktopDisplayInfo.ddiRefreshRateMin;


		// DisplayInfo struc contains valid info about max and min refresh rate allowed.
        if (xRes < 512) 
        {   // We will be using line doubling in these modes, which means our vertical res
            // will be twice what the OS thinks it should be. Therefore we should halve the
            // reported max/min refresh rates.
            Max >>= 1;
            Min >>= 1;
        }

        if (Max < 60 || Min > 200) 
        {   // Sometimes get this case when switching from a DDC to a non DDC monitor.
            Freq = 60;
        }
        else 
        {
            if (Freq == 0)// User has no preference so automatically choose highest refresh
	        Freq = Max;// rate possible for this combination of equipment.
	    else 
            {
	        if (Freq > Max)
		    Freq = Max;
	        if (Freq < Min)
		    Freq = Min;
	    }
        }
	DISPDBG(("Refresh Rate after DisplayInfo limits: %d Hz", Freq));
    }
    else if (pDev->pGLInfo->DesktopDisplayInfo.ddiInfoFlags & REFRESH_RATE_MAX_ONLY) 
    {
        DISPDBG(("REFRESH_RATE_MAX_ONLY: Using %d as refresh rate", pDev->pGLInfo->DesktopDisplayInfo.ddiRefreshRateMax));
        Freq = pDev->pGLInfo->DesktopDisplayInfo.ddiRefreshRateMax;
    }
    else 
    {   // If the monitor information is not valid, then default to 60Hz whatever
        // the user specifies. This helps when changing into Hi Res modes - but can
        // still cause problems if the monitor doent support the mode at all - eg many
        // monitors cannot support 1600x1200 at any refresh rate.
        Freq = 60;

        DISPDBG(("Using default refresh rate of %dHz", Freq));
    }

	_Sprintf(szModeKey, "DEFAULT\\TIMINGS\\%d,%d", xRes, yRes);

    // Repeatedly read Vesa information from the registry, and accept the first one that
    // matches our conditions
    PixelClock = 0;
    M = 0;      // First key to try is 1
    while (1) 
    {
        M++;

	_Sprintf(szFreqValue, "%d", M);

        DISPDBG(("Looking for registry key: %s value:%s", szModeKey, szFreqValue));

        // Check to see if we have an override for this particular refresh
	ulSize = sizeof(VESA_TIMING_STANDARD);
	if (CM_Read_Registry_Value( pDev->DevNode, 
                                    szModeKey, 
                                    szFreqValue,
				    REG_BINARY, 
                                    &VESATimings, 
                                    &ulSize,
				    CM_REGISTRY_SOFTWARE) != CR_SUCCESS) 
        {
            // We have no more mode information if this fails. Use last read value.
            Freq = VESATimings.Min;
            // Calculate Pixel Clock in 100 Hz units
	    PixelClock = (Htot * Vtot * Freq * 8) / 100;
            DISPDBG(("No more timing information. Use last known"));
            break;
        }

	Htot = VESATimings.HTot;
	Hss =  VESATimings.HFP ;
	Hse =  Hss + VESATimings.HST;
	Hbe =  Hse + VESATimings.HBP;
	Hsp = VESATimings.HSP;
	Vtot = VESATimings.VTot;
	Vss =  VESATimings.VFP ;
	Vse =  Vss + VESATimings.VST;
	Vbe =  Vse + VESATimings.VBP;
	Vsp = VESATimings.VSP;

	// Calculate Pixel Clock in 100 Hz units
	PixelClock = (Htot * Vtot * Freq * 8) / 100;

        // Ensure timings found will match timings required
        if (Freq < VESATimings.Min) 
        {
            //If desired frequency is below range supported, then go onto next entry.
            DISPDBG(("Discarding timings as desired frequency is below minimum"));
            continue;
        }
        if (Freq > VESATimings.Max) 
        {
            //If desired frequency is above range supported, set frequency to max.
            DISPDBG(("Limiting frequency to maximum"));
            Freq = VESATimings.Max;
    	    PixelClock = (Htot * Vtot * Freq * 8) / 100;
        }

        // On permedia, Limit maximum frequency of the Pixel Data Clock to 100MHz
        {
            DWORD MaxPixelClock;
            DWORD OldFreq = Freq;

            // Limit the maximum data transfer between the Chip and the DAC
            // P2 we can handle up to 125M DWORDS of data. We should be able to
            // handle up to 2*SystemClock, but we need to tweek the FifoControl
            // register a little more to achieve these rates.
            MaxPixelClock = 1250000;

            // Convert 
            MaxPixelClock = MaxPixelClock << (2 - (depth >> 4));

            // Ensure Pixel Clock doesnt exceed DAC limit of 230MHz
            if (MaxPixelClock > 2300000) MaxPixelClock = 2300000;

            if (PixelClock > MaxPixelClock) 
            {
                Freq = (MaxPixelClock * 100) / (Htot * Vtot * 8);
                DISPDBG(("Pixel clock of %d being limited to %d - %dHz", PixelClock, MaxPixelClock, Freq));
                PixelClock = MaxPixelClock;

                if (Freq < VESATimings.Min) 
                {
                    DISPDBG(("Discarding timings as frequency is now below minimum"));
                    Freq = OldFreq; // Restore old frequency - as we have modified it.
                    continue;
                }
            }
        }

        break;
    }

    if (!PixelClock) 
    {
        DISPDBG(("No timings found for %dx%dx%dbpp %dHz.", xRes, yRes, depth, Freq));
    }

    DISPDBG(("Resolution Found: %dx%dx%dbpp %dHz. Pixel Clock %d00Hz", xRes, yRes, depth, Freq, PixelClock));
    DISPDBG(("Htot %4d Hss  %2d Hse  %2d Hbe  %2d Hsp  %2d", Htot, Hss, Hse, Hbe, Hsp));
    DISPDBG(("Vtot %4d Vss  %2d Vse  %2d Vbe  %2d Vsp  %2d", Vtot, Vss, Vse, Vbe, Vsp));


    pDev->pGLInfo->wRefreshRate = (WORD)Freq;

    // Setup VTG
    RefClkSpeed = 143182;

    // P2
    SystemClock = 800000;

    ulSize = 1;
    ulValue = 0;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
			        szSysClk, 
                                REG_BINARY, 
                                &ulValue, 
                                &ulSize,
			        CM_REGISTRY_SOFTWARE) == CR_SUCCESS) 
    {
        if (ulValue > MaxChipClockRate)
	        ulValue = MaxChipClockRate;
        SystemClock = ulValue * 10000;
    }

    pDev->ClkSpeed = SystemClock;

    // Update the registry with Memphis's required strings. This uses SystemClock
    RegistryLog(pDev);

    PermediaCalculateScreenBase(pDev);

    // Reset the chip first of all. as long as we arnt a Permedia 1
    {
        ULONG highWater;

        // First work out the correct VideoFifoControl value. This gets
        // loaded at reset
        #define videoFIFOSize		32
        #define videoFIFOLoWater	 8
        #define videoFIFOLatency	25

        // Calculate the high-water, by taking into account
        // the pixel clock, the pxiel size, add 1 for luck
        highWater = (((videoFIFOLatency * PixelClock * depth) / (64 * SystemClock )) + 1);

        // Trim the highwater, make sure it's not bigger than the FIFO size
        if (highWater > videoFIFOSize)
	        highWater = videoFIFOSize;

        highWater = videoFIFOSize - highWater;

        // Make sure the highwater is greater than the low water mark.
        if (highWater <= videoFIFOLoWater)
	        highWater = videoFIFOLoWater + 1;

        pDev->dwFifoControl = (highWater << 8) | videoFIFOLoWater;
	        
        DISPDBG(("Setting VFifoCtl to 0x%x (pc 0x%x, sc 0x%x, tm %d)\n", pDev->dwFifoControl, PixelClock, SystemClock, videoFIFOLatency));
        ResetChip(pDev);
    }

    // Work out delay between RAMDAC accesses. This has to be 6 Video Clocks, but is 
    // measured in PCI clocks. Worst case we get 33MHz PCI Clock so number of clocks
    // to wait should be 6 * PCI / Video (rounded up), where PCI and Video are
    // measured in Hz
    ulValue = 16;						// 16 video clocks on P2
    pDev->dwVClkCtl = (ulValue * 330000 + 329999) / PixelClock;

    // Set a minimum of 5 PCI clocks of delay.
    if (pDev->dwVClkCtl < 5)
	pDev->dwVClkCtl = 5;

    READ_GLINT_CTRL_REG(ChipConfig, ulValue);
    if (ulValue & PM_CHIPCONFIG_AGPCAPABLE)
        pDev->dwVClkCtl <<= 1;                 // AGP is 66MHz -> twice as many clocks required

    DISPDBG(("Waiting for %d clocks between RAMDAC access's", pDev->dwVClkCtl));
    // Shift it to its correct position in the register
    pDev->dwVClkCtl <<= 2;
    READ_GLINT_CTRL_REG(VClkCtl, ulValue);
    // Dont want to change VCLKCtl PLL selector bits.
    pDev->dwVClkCtl |= (ulValue & 3);
    if (!(ulValue & ~3)) 
    {
        BYTE	MOR;
        // If no delay currently set, then we have come from VGA. In this case, we need to
        // check what the VGA has set the clock to and maintain this value. Also set a huge
        // ramdac access delay until we program the pixel clock.
        READ_GLINT_CTRL_REG(ReadMiscOutputReg, MOR);
        pDev->dwVClkCtl = (pDev->dwVClkCtl & ~3) | ((ULONG)(MOR & 0xC) >> 2);

        LOAD_GLINT_CTRL_REG(VClkCtl, pDev->dwVClkCtl | 0xfc);
    }

    // Work out the control information for VGA
    pDev->pGLInfo->dwVGADisable = 0x0005;
    pDev->pGLInfo->dwVGAEnable = 0x4b05;


    DisableVGALow(pDev);

    ZeroMemoryRegion(((char *)pDev->pFrameBuffer) + pDev->pGLInfo->dwScreenBase, 
                        pDev->pGLInfo->dwOffscreenBase - pDev->pGLInfo->dwScreenBase);

    // Check for registry overrides
    ulSize = 1;
    ulValue = 0;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
			        szRefClk, 
                                REG_BINARY, 
                                &ulValue, 
                                &ulSize,
				CM_REGISTRY_SOFTWARE) == CR_SUCCESS) 
    {
	if (ulValue > MaxChipClockRate)
	    ulValue = MaxChipClockRate;
	RefClkSpeed = ulValue * 10000;
    }

	// Setup Ramdac.
    DISPDBG(("GLINT: SetDacParameters to %d", depth));

    {
        // SetUpTVP4020(depth);
        // No separate S/W reset for P2 pixel unit

        TVP4020_SET_PIXEL_READMASK (0);
        TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, 0x00) // 4x32x32, cursor 1, ADDR[9:8] = 00, cursor off
        TVP4020_LOAD_CURSOR_CTRL(TVP4020_CURSOR_OFF);           // Redundant here; we just cleared the CCR above
        TVP4020_SET_CURSOR_COLOR0(0, 0, 0);
        TVP4020_SET_CURSOR_COLOR1(0xFF, 0xFF, 0xFF);

        // Set up syncs appropriately for ancient monitors
        ulValue = ((Hsp ? 0x0 : 0x1) << 2) | ((Vsp ? 0x0 : 0x1) << 3) | 0x12;

        TVP4020_WRITE_INDEX_REG(__TVP4020_MISC_CONTROL,    ulValue);   // 7.5 IRE, no sync invert, 8-bit data
        TVP4020_WRITE_INDEX_REG(__TVP4020_MODE_CONTROL,    0x00);   // Mode Control
        TVP4020_WRITE_INDEX_REG(__TVP4020_CK_CONTROL,      0x00);   // Color-Key Control
        TVP4020_WRITE_INDEX_REG(__TVP4020_PALETTE_PAGE,    0x00);   // Palette page

        // No zoom on P2 pixel unit
        // No separate multiplex control on P2 pixel unit
        //Start TI TVP4020 programming
        switch (depth) 
        {
            case 8:
                TVP4020_WRITE_INDEX_REG(__TVP4020_COLOR_MODE, 0x30);  // RGB, graphics, Color Index 8
                DISPDBG(("Loading 8bpp palette"));

                if (!pDev->pGLInfo->pColorTable) 
                {   // Setup pColorTable if we can 
                    if (pDev->pGLInfo->lpColorTableSeg) 
                    {
                        pDev->pGLInfo->pColorTable = pDev->pGLInfo->lpColorTableOff
                                        + (ULONG)SelectorMapFlat(pDev->pGLInfo->lpColorTableSeg);
                    }
                }
                if (pDev->pGLInfo->pColorTable) 
                {
                    UCHAR *pColorTable = (UCHAR *)pDev->pGLInfo->pColorTable;
                    DISPDBG(("RGB525 Load Current 8bpp colormap"));

                    TVP4020_PALETTE_START_WR (0);
                    for (index = 0; index <= 0xff; ++index) 
                    {
                        TVP4020_LOAD_PALETTE (pColorTable[2], pColorTable[1], pColorTable[0]);
                        pColorTable += 4;
                    }
                }
            break; //End of case 8:

            case 15:
            case 16:
                pixelCtrl = 0xB4;       // True color w/gamma, RGB, graphics, 5:5:5:1
                TVP4020_WRITE_INDEX_REG(__TVP4020_COLOR_MODE, pixelCtrl);
                // load linear ramp into LUT
                TVP4020_PALETTE_START_WR (0);
                for (index = 0; index <= 0xff; ++index) 
                {
                    ulValue = (index & 0xF8) | (index >> 5);
                    TVP4020_LOAD_PALETTE (ulValue, ulValue, ulValue);
                }
            break; //End of case 16: case 15:

            // 12-bit not supported on P2
            case 24:  // P2 supports 24-bit packed mode
            case 32:
                pixelCtrl = 0xB8;       // True color w/gamma, RGB, graphics, 8:8:8:8
                if (depth == 24)
                pixelCtrl |= 0x01;  // True color w/gamma, RGB, graphics, packed-24 (8:8:8)
                TVP4020_WRITE_INDEX_REG(__TVP4020_COLOR_MODE, pixelCtrl);
                // load linear ramp into LUT
                TVP4020_PALETTE_START_WR (0);
                // standard 888 ramps
                for (index = 0; index <= 0xff; ++index) 
                {
                    TVP4020_LOAD_PALETTE (index, index, index);
                }
            break; //End of case 24: case 32:

            default:
                DISPDBG(("Cannot set RAMDAC for bad depth %d", depth));
            break;

        }

        {
            ULONG	M, N, P;
            // Ensure frequencies do not interract
            P = 1;
            do 
            {
                M = P * SystemClock;
                if ((M > PixelClock - 10000) && (M < PixelClock + 10000)) 
                {
                    // Frequencies do interract. We can either change the
                    // PixelClock or change the System clock to avoid it.
                    SystemClock = (PixelClock - 10000) / P;
                    DISPDBG(("Clock Frequencies interract, Set SystemClock to %d00 Hz", SystemClock));
                }
                N = P * PixelClock;
                if ((N > SystemClock - 10000) && (N < SystemClock + 10000)) 
                {
                    // Frequencies do interract. We can either change the
                    // PixelClock or change the System clock to avoid it.
                    SystemClock = N - 10000;
                    DISPDBG(("Clock Frequencies interract, Set SystemClock to %d00 Hz", SystemClock));
                }
                P++;
            } while ((M < PixelClock + 30000) || (N < SystemClock + 30000));
        }

        if (!(SystemClock = TVP4020_ProgramSystemClock(pDev, RefClkSpeed, SystemClock)))
            return(0);
    
        if (!(PixelClock = TVP4020_ProgramPixelClock(pDev, RefClkSpeed, PixelClock)))
            return(0);
    
        SystemClockInitialised = SystemClock;

        TVP4020_SET_PIXEL_READMASK (0xff); //JME 17.Mar.97
    }

    // Setup VTG
    //dShift = (depth >> 4) + 1;
    // dShift is now used as a multiplier, instead of a shift count.
    // This is to support P2 packed-24 mode where the VESA horizontal timing values need to be
    // multiplied by a non-power-of-two multiplier.
    if (depth == 8)
        dShift = depth >> 2;  // 32-bit pixel bus
    else
        dShift = depth >> 3;  // 64-bit pixel bus

    LOAD_GLINT_CTRL_REG(HTotal,		(Htot * dShift) - 1);
    LOAD_GLINT_CTRL_REG(HsStart,	Hss * dShift);
    LOAD_GLINT_CTRL_REG(HsEnd,		Hse * dShift);
    LOAD_GLINT_CTRL_REG(HbEnd,		Hbe * dShift);
    LOAD_GLINT_CTRL_REG(HgEnd,		Hbe * dShift);    

    LOAD_GLINT_CTRL_REG(VTotal,		Vtot - 1);
    LOAD_GLINT_CTRL_REG(VsStart,	Vss - 1);
    LOAD_GLINT_CTRL_REG(VsEnd,		Vse - 1);
    LOAD_GLINT_CTRL_REG(VbEnd,		Vbe);

    LOAD_GLINT_CTRL_REG(ScreenBase, pDev->pGLInfo->dwScreenBase >> 3);
    LOAD_GLINT_CTRL_REG(ScreenStride, (xRes >> 3) * (depth >> 3));

    // Work out Video Control information
    ulValue = ((Hsp ? 0x1 : 0x3) << 3) | ((Vsp ? 0x1 : 0x3) << 5) | 1;

    {
	ULONG	enableFlags;

        // P2 specific initialisation
        ulValue = (0x1 << 3) | (0x1 << 5) | 1; // Active high syncs.
        if (depth != 8) 
            ulValue |= (1 << 16);  // P2 uses 64-bit pixel bus for modes > 8BPP

        // Enable VBLANK Interrupts on P2 for cursor control if interrupts enabled.
        if (!(pDev->pGLInfo->dwFlags & GMVF_NOIRQ)) 
        {
	    READ_GLINT_CTRL_REG(IntEnable, enableFlags);
	    enableFlags |= INTR_ENABLE_VBLANK;
            LOAD_GLINT_CTRL_REG(IntEnable, enableFlags);
	    pDev->pGLInfo->dwFlags |= GMVF_VBLANK_ENABLED;
        }
        else
        {
	    pDev->pGLInfo->dwFlags &= ~GMVF_VBLANK_ENABLED;
        }
    }

    if (xRes < 512)
        ulValue |= 4;

    pDev->pGLInfo->dwVideoControl = ulValue;

    ulSize = 4;
    if (CM_Read_Registry_Value( pDev->DevNode, 
                                szDefaultKey, 
                                "VideoControl",
			        REG_BINARY, 
                                &ulValue, 
                                &ulSize,
			        CM_REGISTRY_SOFTWARE) == CR_SUCCESS) 
    {
        DWORD ulMask;
        // VideoControl override
        ulSize = 4;
        if (CM_Read_Registry_Value( pDev->DevNode, 
                                    szDefaultKey, 
                                    "VideoControlMask",
			            REG_BINARY, 
                                    &ulMask, 
                                    &ulSize,
			            CM_REGISTRY_SOFTWARE) == CR_SUCCESS) 
        {
            pDev->pGLInfo->dwVideoControl = (pDev->pGLInfo->dwVideoControl & ~ulMask) | (ulValue & ulMask);
        }
    }
    LOAD_GLINT_CTRL_REG(VideoControl, pDev->pGLInfo->dwVideoControl);
    LOAD_GLINT_CTRL_REG(VClkCtl, pDev->dwVClkCtl);    // Need to set up RAMDAC pll pins

    DISPDBG(("InitialiseVideo Finished"));
    return(1);
}


// This routine is called to reenable VGA when the driver is disabling. It does whatever
// is required for a Permedia to run in VGA mode. It also disabes the Bypass on TX and SX. 
DWORD _cdecl 
EnableVGA(PDEVICETABLE pDev) 
{
    pTVP4020RAMDAC  pTVP4020Regs;

    // added pDev a calling parameter so secondary interrupt ,etc., can be turned off

	if (!pDev)		// called from glintmvd.asm with null pointer means primary device
	{
		pDev = FindPrimarypDev();
	}

	pTVP4020Regs = (pTVP4020RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);

    // Enable VGA on a permedia
    LOAD_GLINT_CTRL_REG(VideoControl, 0);

    LOAD_GLINT_CTRL_REG(VClkCtl, pDev->dwVClkCtl & 3);      // Need to set up RAMDAC pll pins.
    // Reset Pixel Clock
    switch (pDev->dwVClkCtl & 3) 
    {
        case 0:
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_A3, 0x6);  // Disable the Pixel Clock
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_A2, 0x02); // N
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_A1, 0x1c); // M
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_A3, 0x0b); // P / Enable
        break;

        case 1:
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_B3, 0x6);  // Disable the Pixel Clock
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_B2, 0x02); // N
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_B1, 0x10); // M
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_B3, 0x0a); // P / Enable
        break;

        default:
	    // Dont have to reset register set C
	break;
    }
    TVP4020_WaitForPixelClockToSync(pDev);
    // Disable all interupts and remove the handler.
    if (pDev->dwFlags & GMVF_DELTA) 
    {
        LOAD_GLINT_CTRL_REG(DeltaIntEnable, 0);
    }
    LOAD_GLINT_CTRL_REG(IntEnable, 0);
    if (pDev->IRQHandle) 
    { 
        pDev->pGLInfo->wCursorControl   &= 255;         // Remove cursor update bit
        VPICD_Force_Default_Behavior(pDev->IRQHandle);  // kill the virtual handler
        pDev->IRQHandle                 = 0;
        pDev->pGLInfo->dwFlags          |= GMVF_NOIRQ;
    }

    // Indicate that we are not in HiRes mode any more
    pDev->pGLInfo->dwHiResMode = 0;

    return(1);
}

VOID
DisableVGALow(PDEVICETABLE pDev) 
{
    // disable VGA on a permedia
    LOAD_GLINT_CTRL_REG(VGAControlReg, (WORD)pDev->pGLInfo->dwVGADisable);   
    pDev->pGLInfo->dwHiResMode = 1;// Indicate that we are in HiRes mode now  
    if (pDev->IRQHandle == 0)// Reenable the interrupt handler if it was disabled.
	InitialiseInterrupts(pDev);
}

#if 0
// This routine is called to reenable VGA when the driver is disabling. It does whatever
// is required for a Permedia to run in VGA mode. It also disabes the Bypass on TX and SX. 
DWORD _cdecl
DisableVGA() 
{
    PDEVICETABLE pDev = FindPrimarypDev();

    DisableVGALow(pDev);
    return(1);
}
#endif

// This routine is called when the VXD terminates. It is after the VGA has been reenabled.
// The idea is to give us a second chance to switch the bypass on TX and SX. On a Permedia,
// we shouldt do anything at this stage as the VGA should have taken over.
DWORD _cdecl 
DisableHardware() 
{
    PDEVICETABLE pDev = FindPrimarypDev();
  
    pDev->pGLInfo->dwHiResMode = 0;// Indicate that we are not in HiRes mode any more
    return(1);
}


#ifdef POWER_STATE_MANAGE
DWORD _cdecl 
SetAdapterPowerState(DWORD DevNode, DWORD PowerState)
{
    PDEVICETABLE    pDev = FindpDevFromDevNode(DevNode);
    DWORD	    iRet = CR_SUCCESS;
    switch (PowerState)
    {

	// return to normal state
	case CM_POWERSTATE_D0:
		// set return value to default if BIOS POST needs to be done to
		// init the chip after D3 powerdown (powerup is not a reboot)
        if (pDev->dwPowerState == CM_POWERSTATE_D3)
			iRet = CR_DEFAULT;

        // On return to D0, INT2F DevToForeground is issued.
        // this will call ResetHiresMode and sets up the chip for hires mode.

		// Reenable the interrupt handler if it was disabled.
	    if (pDev->IRQHandle == 0)
			InitialiseInterrupts(pDev);

	    LOAD_GLINT_CTRL_REG(VideoControl, pDev->pGLInfo->dwVideoControl);

	    break;

// In all reduced power states, PCI accesses are disallowed.

// All state needed to restore on return to D0 should be saved at this point.

// In this case, EnableVGA turns off the Permedia2 interrupt and sets the chip
// into VGA usable mode since the caller will be setting VGA mode 3.

// INT2F DevToBackground is issued before SetAdapterPowerState is called.  It
// causes device bitmaps to be copied to host memory and invalidates the font
// cache.  DirectDraw/D3D will invalidate all device surfaces on powerdown.
// Therefore video memory does not need to be saved.

	case CM_POWERSTATE_D1:		    // reduced power
	case CM_POWERSTATE_D2:		    // states

	case CM_POWERSTATE_D3:		    // device powerdown
	case CM_POWERSTATE_HIBERNATE:	// system powerdown (resumable)

        EnableVGA(pDev);	
    }

    pDev->dwPowerState = PowerState;

    return iRet;
}
#endif //#ifdef POWER_STATE_MANAGE
