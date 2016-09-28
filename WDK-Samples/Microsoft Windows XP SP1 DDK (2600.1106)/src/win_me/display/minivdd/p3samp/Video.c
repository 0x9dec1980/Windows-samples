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
#include "minivdd.h"
#include "perm3.h"
#include <regdef.h>


#define MaxChipClockRate 100


//
// Entries for 3 bpp colors
//      Index(0-7) -> Color(0-255)

ULONG   bPal8[] = {

    0x00, 0x24, 0x48, 0x6D,
    0x91, 0xB6, 0xDA, 0xFF
};


//
// Entries for 2 bpp colors
//      Index(0-3) -> Color(0-255)

ULONG   bPal4[] = {

    0x00, 0x6D, 0xB6, 0xFF
};


#define SET_P2_VS_MODE(mode)                                                \
    if (pDev->ChipID == PERMEDIA2_ID || pDev->ChipID == TIPERMEDIA2_ID) {   \
        ULONG ulValue1;                                                     \
        READ_GLINT_CTRL_REG(VSConfiguration, ulValue1);                     \
        ulValue1 = (ulValue1 & ~0xf) | (mode);                              \
        LOAD_GLINT_CTRL_REG(VSConfiguration, ulValue1);                     \
        DISPDBG(("Video Streams Interface set to mode %d", (mode)));        \
    }


#define VS_ROM_MODE         0
#define VS_RAMDAC_MODE      1
#define VS_VIDEO_MODE       7

    
//
// Local helper functions
//

void 
SwitchToHiResMode(
    PDEVICETABLE, BOOLEAN);

BOOLEAN 
Program_P3RD(
    PDEVICETABLE, VDDDISPLAYINFO *, ULONG, ULONG, ULONG, 
    PULONG, PULONG, PULONG);


#define ROTATE_LEFT_DWORD(dWord,cnt) (((cnt) < 0) ? (dWord) >> ((-cnt)) : (dWord) << (cnt))
#define ROTATE_RTIGHT_DWORD(dWord,cnt) (((cnt) < 0) ? (dWord) << ((-cnt)) : (dWord) >> (cnt))


//
// Engine function which actually sets up the video signal generator
//

DWORD _cdecl
InitializeVideo(
    PDEVICETABLE            pDev)
{
    VDDDISPLAYINFO         *pDispInfo;
    ULONG                   ulValue;
    LONG                    dShift, dStrideShift;
    VESA_TIMING_STANDARD    VESATimings;
    ULONG                   Htot, Hss, Hse, Hbe, Hsp;
    ULONG                   Vtot, Vss, Vse, Vbe, Vsp;
    ULONG                   PixelClock, Freq, MemClock = 0;
    ULONG                   RefClkSpeed, SystemClock; // , GlintDeltaClock; 
    ULONG                   VTGPolarity;
    ULONG                   DacDepth, depth, xRes, yRes;
    ULONG                   xStride;
    ULONG                   pixelData; // , ulVGateStart;

    //
    // Get the helper pointer for faster access
    //

    pDispInfo = &pDev->pGLInfo->DesktopDisplayInfo;

    //
    // Retrieve display mode related information
    //

    depth = pDispInfo->ddiBpp;
    xRes  = pDispInfo->ddiXRes;
    yRes  = pDispInfo->ddiYRes;

    //
    // Just in case we get weird values in the DesktopDisplayInfo
    //

    if (xRes < 320) {
        xRes = 640;
    }
    if (yRes < 200) {
        yRes = 480;
    }
    if (depth < 8) {
        depth = 16;
    }

    //
    // Check there is enough video memory to support the mode
    //
 
    if ((xRes * yRes * (depth >> 3)) > pDev->dwFrameBufferSize) {
    
        DISPDBG((
            "Not enough memory to initiate mode %dx%d %dbpp",
             xRes, yRes, depth));

        return (0);
    }

    //
    // Choose suitable refresh rate
    //
 
    if ((pDispInfo->ddiInfoFlags & REFRESH_RATE_MAX_ONLY) &&
        (pDispInfo->ddiRefreshRateMax)) {

        DISPDBG((
            "REFRESH_RATE_MAX_ONLY: Using %d as refresh rate",
            pDispInfo->ddiRefreshRateMax));

        Freq = pDispInfo->ddiRefreshRateMax;

    } else {

        //
        // Just use the default in all cases 
        //

        Freq = 60;
    }

    //
    // For timing calculations need full depth in bits
    //

    if ((DacDepth = depth) == 15) {
    
        DacDepth = 16;
    } else {
        
        if (depth == 12) {
        
            DacDepth = 32;
        }
    }


    //
    // Get screen stride in pixels
    //

    xStride = pDispInfo->ddiXRes;

    DISPDBG((
        "InitializeVideo called: \n",
        "depth %d, xres %d, yres %d, freq %d, xStride %d\n",
        depth, xRes, yRes, Freq, xStride));

    //
    // Get the video timing, from the registry, if an entry exists, or from
    // the list of defaults, if it doesn't.
    //

    if (! GetVideoTiming(pDev, xRes, yRes, Freq, DacDepth, &VESATimings)) {

        DISPDBG((0, "GetVideoTiming failed."));

        return (FALSE);
    }

    //
    // We have got a valid set of VESA timigs
    // Extract timings from VESA list in a form that can be programmed into
    // the P2/P3 timing generator.
    //

    Htot = GetHtotFromVESA (&VESATimings);
    Hss  = GetHssFromVESA  (&VESATimings);
    Hse  = GetHseFromVESA  (&VESATimings);
    Hbe  = GetHbeFromVESA  (&VESATimings);
    Hsp  = GetHspFromVESA  (&VESATimings);

    Vtot = GetVtotFromVESA (&VESATimings);
    Vss  = GetVssFromVESA  (&VESATimings);
    Vse  = GetVseFromVESA  (&VESATimings);
    Vbe  = GetVbeFromVESA  (&VESATimings);
    Vsp  = GetVspFromVESA  (&VESATimings);

    PixelClock = VESATimings.pClk;

    //
    // If we're zooming by 2 in Y then double the vertical timing values.
    //

    // if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_Y_BY2) {

    //     Vtot *= 2;
    //     Vss  *= 2;
    //     Vse  *= 2;
    //     Vbe  *= 2;
    //     PixelClock *= 2;
    // }

    DISPDBG(("VESA Timing Values are :-\n"));
    DISPDBG(("Htot %4d \n", Htot));
    DISPDBG(("Hss  %2d \n", Hss ));
    DISPDBG(("Hse  %2d \n", Hse));
    DISPDBG(("Hbe  %2d \n", Hbe));
    DISPDBG(("Hsp  %2d \n", Hsp));
    DISPDBG(("Vtot %4d \n", Vtot));
    DISPDBG(("Vss  %2d \n", Vss ));
    DISPDBG(("Vse  %2d \n", Vse));
    DISPDBG(("Vbe  %2d \n", Vbe));
    DISPDBG(("Vsp  %2d \n", Vsp));
    DISPDBG(("PixelClock %d00 Hz\n", PixelClock));
    DISPDBG((
        "fH = %d.%03dkHz\n", 
        PixelClock / (Htot * 8 * 10), (PixelClock * 100) / (Htot * 8) % 1000));
    DISPDBG(("fV = %dHz\n", (PixelClock * 100) / (Htot * 8 * Vtot)));

    //
    // Note that the pixelData limits for P1 are, currently, about
    // 440 MByte/sec and are 500 MByte/sec. I have changed the P1 limit
    // to allow it to run 1280x1024x60Hz, truecolour.
    //

    pixelData = PixelClock * (DacDepth / 8);

    if ((pDev->ChipID == PERMEDIA3_ID) && (pixelData > P3_MAX_PIXELDATA)) { 

        //
        // Failed pixelData validation
        //

        return(FALSE);
    }


    RefClkSpeed = pDev->RefClockSpeed  / 100;   // 100Hz units
    SystemClock = pDev->ChipClockSpeed / 100;   // 100Hz units

    //
    //  We do some basic initialisation before setting up MCLK.
    //

    if (pDev->ChipID == PERMEDIA3_ID) {

        //
        //  disable the video control register
        //

        pDev->pGLInfo->dwVideoControl = 0;
        LOAD_GLINT_CTRL_REG(VideoControl, pDev->pGLInfo->dwVideoControl);

        SwitchToHiResMode(pDev, TRUE);
    }

    //
    // Reset the clock information
    //

    pDev->pGLInfo->PixelClockFrequency = 0;
    pDev->pGLInfo->MClkFrequency       = 0;

    //
    // Setup Ramdac.
    //

    if (pDev->DacId == P3RD_RAMDAC) {

        if (! Program_P3RD( 
                  pDev, 
                  &pDev->pGLInfo->DesktopDisplayInfo, 
                  Hsp, Vsp, 
                  RefClkSpeed, 
                  &SystemClock, 
                  &PixelClock, 
                  &MemClock)) {

            return(FALSE);
        }
    }

#if  GAMMA_CORRECTION
    //
    // Set the LUT cache size to 254 and the first entry to zero, 
    // then write the  LUT cache to the LUT
    //

    LUT_CACHE_SETSIZE (256);
    LUT_CACHE_SETFIRST (0);

    (VOID)Perm3SetColorLookup(
              pDev,
              &(pDev->LUTCache.LUTCache),
              sizeof(pDev->LUTCache),
              TRUE,     // Always update RAMDAC (i.e. don't check cache)
              FALSE);   // Don't Update cache entries
#endif

    //
    // Setup VTG
    //

    switch (pDev->ChipID) {

        case PERMEDIA3_ID:
        case GLINT_R3_ID:

            //
            // If we have a P3 then we have to set or clear byte doubling, 
            // depending on the whether the byte doubling capabilities flag
            // is set.
            //
    
            if (pDev->ChipID == PERMEDIA3_ID) {

                ULONG ulMiscCtl;

                READ_GLINT_CTRL_REG(MiscControl, ulMiscCtl);
    
                ulMiscCtl &= ~PXRX_MISC_CONTROL_BYTE_DBL_ENABLE;

                if (pDev->PXRXCapabilities & PXRX_USE_BYTE_DOUBLING) {

                    ulMiscCtl |= PXRX_MISC_CONTROL_BYTE_DBL_ENABLE;
                } 
    
                LOAD_GLINT_CTRL_REG(MiscControl, ulMiscCtl);
            }
    
    
            ulValue = 3;    // RAMDAC pll pins for VClkCtl
    
            //
            // dShift is now used as a rotate count (it can be negative), 
            // instead of a shift count. This means it on't work with 24-bit
            // packed framebuffer layouts.
            //
    
            if (pDev->DacId == P3RD_RAMDAC) {
    
                if (pDev->PXRXCapabilities & PXRX_USE_BYTE_DOUBLING) {
    
                    // Pretend we have a 64-bit pixel bus
                    dShift = DacDepth >> 4; 
    
                } else if (DacDepth > 8) {
    
                    dShift = DacDepth >> 5; // 128-bit pixel bus
    
                } else {
    
                    dShift = -1;    // We need a shift right not left
                }
    
                dStrideShift = 4;   // Stride & screenBase in 128-bit units
    
            }
    
            //
            // Must load HgEnd before ScreenBase
            //
    
            LOAD_GLINT_CTRL_REG(HgEnd,   ROTATE_LEFT_DWORD (Hbe, dShift));
            // Need to set up RAMDAC pll pins
            LOAD_GLINT_CTRL_REG(VClkCtl, ulValue); 
    
            //
            // We just load the right screenbase with zero (the same as the 
            // left). The display driver will update this when stereo buffers
            // have been allocated and stereo apps start running.
            //
    
            LOAD_GLINT_CTRL_REG(ScreenBaseRight, 0);
            LOAD_GLINT_CTRL_REG(ScreenBase,      0);
    
            LOAD_GLINT_CTRL_REG(
                ScreenStride, 
                (xStride >> dStrideShift) * (DacDepth >> 3)); // 64-bit units
            LOAD_GLINT_CTRL_REG(
                HTotal,  
                (ROTATE_LEFT_DWORD (Htot, dShift)) - 1);
            LOAD_GLINT_CTRL_REG(HsStart,     ROTATE_LEFT_DWORD (Hss, dShift));
            LOAD_GLINT_CTRL_REG(HsEnd,       ROTATE_LEFT_DWORD (Hse, dShift));
            LOAD_GLINT_CTRL_REG(HbEnd,       ROTATE_LEFT_DWORD (Hbe, dShift));
    
            LOAD_GLINT_CTRL_REG(VTotal,      Vtot - 1);
            LOAD_GLINT_CTRL_REG(VsStart,     Vss - 1);
            LOAD_GLINT_CTRL_REG(VsEnd,       Vse - 1);
            LOAD_GLINT_CTRL_REG(VbEnd,       Vbe);
    
            //
            //  We need this to ensure that we get interrupts at the right time
            //
    
            LOAD_GLINT_CTRL_REG(InterruptLine, 0);
    
            //
            // Set up video fifo stuff for P3
            //
    
            if (pDev->ChipID == PERMEDIA3_ID) {

                ULONG    highWater, loWater;
    
                if (pDev->Capabilities & CAPS_INTERRUPTS) {

                    //
                    // we can use our reiterative formula. We start by setting
                    // the thresholds to sensible values for a lo-res mode
                    // (640x480x8) then turn on the FIFO  underrun error 
                    // interrupt (we do this after the mode change to avoid
                    // spurious interrupts). In the interrupt routine we 
                    // adjust the thresholds whenever we get an underrun error
                    //
    
                    loWater = 8;
                    highWater = 28;
    
                    pDev->VideoFifoControl = 
                        (1 << 16) | (highWater << 8) | loWater;
    
                    //
                    // we'll want check for video FIFO errors via the error
                    // interrupt only for a short time as P3/R3 generates a
                    // lot of spurious interrupts too. Use the VBLANK interrupt
                    // to time the period that we keep error interrupts enabled
                    //
    
                    pDev->VideoFifoControlCountdown = 20 * Freq;
    
                    //
                    // don't actually update this register until we've left 
                    // InitializeVideo - we don't want to enable error 
                    // interrupts until the mode change has settled
                    //
    
                    pDev->IntEnable |= 
                        INTR_ERROR_SET | INTR_VBLANK_SET;
    
                    //
                    // we want VBLANK interrupts permanently enabled so that we
                    // can monitor error flags for video FIFO underruns
                    //

                } else {

                    //
                    // We don't have interrupts calculate safe thresholds for
                    // this mode. The high threshold can be determined using
                    // the following formula.
                    //
    
                    highWater = 
                        ((PixelClock / 80) * (33 * DacDepth)) / MemClock;
    
                    if (highWater < 28) {

                        highWater = 28 - highWater;
    
                        //
                        // Low threshhold should be the lower of 
                        // highWater/2 or 8.
                        //
    
                        loWater = (highWater + 1) / 2;
                        if (loWater > 8) {
                            loWater = 8;
                        }

                    } else {

                        //
                        // we don't have an algorithm for this so choose
                        // a safe value 
                        //
    
                        highWater = 0x01;
                        loWater = 0x01;
                    }
    
                    pDev->VideoFifoControl = (highWater << 8) | loWater;
                }
    
                LOAD_GLINT_CTRL_REG(FifoControl, pDev->VideoFifoControl);
            }
    
            //
            // On a P3 set up the memory refresh counter
            //
    
            if (pDev->ChipID == PERMEDIA3_ID) {

                //
                // Memory refresh needs to be 64000 times per second.
                //
    
                ulValue = ((MemClock / 640) - 16) / 32;
                LOAD_GLINT_CTRL_REG(LocalMemRefresh, (ulValue << 1) | 1);
                DISPDBG((
                    "Setting LocalMemRefresh to 0x%x\n", (ulValue << 1) | 1));
            }
    
    
            if (pDev->DacId == P3RD_RAMDAC ) {

                //
                // Enable H & V syncs to active high (the RAMDAC will invert 
                // these as necessary)
                // Enable video out.
                //
    
                VTGPolarity = (1 << 5) | (1 << 3) | 1;
    
                // if ((VideoPortGetRegistryParameters(
                //          HwDeviceExtension,
                //          L"OpenGL.WaitForVBlank",
                //          FALSE,
                //          Perm3RegistryCallback,
                //          &ulValue) == NO_ERROR) && ulValue) {
    
                    //
                    // Set BufferSwapCtl to LimitToFrameRate
                    //
    
                    // VTGPolarity |= (2 << 9);
                // } else {
    
                    //
                    // Set BufferSwapCtl to FreeRunning
                    //
    
                    VTGPolarity |= (1 << 9);
                // }
    
                if (pDev->DacId == P3RD_RAMDAC ) {
    
                    //
                    // P2ST always uses 64-bit pixel bus
                    // P2 uses 64-bit pixel bus for modes > 8BPP
                    //
    
                    VTGPolarity |= (1 << 16);
                }
    
                //
                // Set up pixel size, this register is only on PXRX.
                //
    
                if (pDev->DacId == P3RD_RAMDAC) {

                    if (DacDepth == 8) {
                        VTGPolarity |= (0 << 19);    // 8BPP
                    } else {

                        if (DacDepth == 16) {
                            VTGPolarity |= (1 << 19);    // 16BPP
                        } else {
                            if (DacDepth == 32) {
                                VTGPolarity |= (2 << 19);    // 32BPP
                            }
                        }
                    }
    
                    ulValue = 0;
    
                    // VideoPortGetRegistryParameters(
                    //     HwDeviceExtension, 
                    //     L"FrameBufferPatching", 
                    //     FALSE, 
                    //     Perm3RegistryCallback, 
                    //     &ulValue );
    
                    //
                    // If we only want to patch on a Gamma-board and we are not
                    // on a Gamma-board then clear the variable read from the
                    // registry. Duplicated from perm3init.c in the display 
                    // driver.
                    //
    
                    if ((ulValue & (1 << 2)) && ! GLINT_GAMMA_PRESENT) {
                        ulValue = 0;
                    } 
 
                    //
                    // Can't handle non-patch aligned strides:
                    //
    
                    if (xStride % (64 << (2 - (DacDepth >> 4)))) {
                        ulValue = 0;
                    }
 
                    //
                    // Can't handle non-patch aligned heights:
                    //
    
                    if (yRes % 16) {
                        ulValue = 0;
                    }
    
                    if (ulValue & 1) {
                        VTGPolarity |= (1 << 18);
                    }
                }
    
                //
                // Set the stereo bit if it's enabled.
                //
    
                if (pDev->Capabilities & CAPS_STEREO) {
                    VTGPolarity |= (1 << 11);
    
                    //
                    // Check the registry to see if we should set the 
                    // RightEyeCtl bit. This controls the polarity of the
                    // left/right eye pin. Defaults to on.
                    //
    
                    // if ((VideoPortGetRegistryParameters(
                    //          HwDeviceExtension,
                    //          L"OpenGL.StereoRightEyeControl",
                    //          FALSE,
                    //          Perm3RegistryCallback,
                    //          &ulValue) == NO_ERROR)) {

                    //     ulValue &= 0x1;
                    //     VTGPolarity |= (ulValue << 12);
                    // } else {
    
                        VTGPolarity |= (1 << 12);
                    // }
                }
            }
    
            // if (VideoModeInfo->DriverSpecificAttributeFlags & 
            //     CAPS_ZOOM_Y_BY2)
            //    VTGPolarity |= (1 << 2);
    
    
            DISPDBG(("Writing 0x%x to VideoControl register\n", VTGPolarity));
            pDev->VideoControlMonitorON = VTGPolarity & VC_DPMS_MASK;

            pDev->pGLInfo->dwVideoControl = VTGPolarity;

            //
            // TMM
            //

            LOAD_GLINT_CTRL_REG(VideoControl, pDev->pGLInfo->dwVideoControl);
    
            DISPDBG(("Loaded Permedia timing registers:\n"));
            DISPDBG(("\tScreenBase: 0x%x\n", 0));
            DISPDBG((
                "\tScreenStride: 0x%x\n", 
                (xStride >> dStrideShift) * (DacDepth >> 3)));
            DISPDBG((
                "\tHTotal: 0x%x\n", (ROTATE_LEFT_DWORD (Htot, dShift)) - 1));
            DISPDBG(("\tHsStart: 0x%x\n", ROTATE_LEFT_DWORD (Hss, dShift)));
            DISPDBG(("\tHsEnd: 0x%x\n", ROTATE_LEFT_DWORD (Hse, dShift)));
            DISPDBG(("\tHbEnd: 0x%x\n", ROTATE_LEFT_DWORD (Hbe, dShift)));
            DISPDBG(("\tHgEnd: 0x%x\n", ROTATE_LEFT_DWORD (Hbe, dShift)));
            DISPDBG(("\tVTotal: 0x%x\n", Vtot - 1));
            DISPDBG(("\tVsStart: 0x%x\n", Vss - 1));
            DISPDBG(("\tVsEnd: 0x%x\n", Vse - 1));
            DISPDBG(("\tVbEnd: 0x%x\n", Vbe));
            DISPDBG(("\tVideoControl: 0x%x\n", VTGPolarity));
    
            break;
    } // End of switch (ChipID)


    //
    // record the final chip clock in the registry
    //

    SystemClock *= 100;

    MemClock *= 100;
    PixelClock *= 100;

    //
    // Save the clock information to support OVERLAY surface
    //

    pDev->pGLInfo->PixelClockFrequency = PixelClock;
    pDev->pGLInfo->MClkFrequency       = MemClock;

    pDev->bVTGRunning = TRUE;

    //
    // Write information into registry which is required by the display applet
    //

    RegistryLog(pDev);

    DISPDBG(("InitializeVideo Finished\n"));

    return (TRUE);
}


//
// Find out what type of RAMDAC we have
//

DWORD 
FindRamDacType(
    PDEVICETABLE    pDev)
{
    pDev->DacId       = P3RD_RAMDAC;    // This is the only RAMDAC for P3
    pDev->ActualDacId = P3RD_RAMDAC;

    DISPDBG(("PERMEDIA3: using INTERNAL P3RD RAMDAC"));

    return (1);
}

#define INITIALFREQERR 100000

DWORD
P3RD_WaitForPixelClockToSync(
    PDEVICETABLE    pDev)
{
    ULONG           M;
    ULONG           ulValue;
    pP3RDRAMDAC     pP3RDRegs;

    pP3RDRegs = (pP3RDRAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);
    M         = 0x100000;
    ulValue   = 0;

    do {

        P3RD_READ_INDEX_REG(P3RD_DCLK_CONTROL, ulValue);
    } while ((ulValue & P3RD_DCLK_CONTROL_LOCKED) == FALSE && --M);

    DISPDBG(("PixelClock Synced M:%d", M));

    if (M == 0) {

        return (0);
    }

    return (1);
}


//
// This routine is called to reenable VGA when the driver is disabling. 
// It does whatever is required for a Permedia to run in VGA mode. 
// It also disabes the Bypass on TX and SX. 
//

DWORD _cdecl 
EnableVGA(
    PDEVICETABLE    pDev) 
{
    pP3RDRAMDAC     pP3RDRegs;

    //
    // Added pDev as calling parameter so secondary interrupt,
    // etc., can be turned off
    //

    if (! pDev) {

        //
        // called from glintmvd.asm with null pointer means primary device
        //

        pDev = FindPrimarypDev();
    }

    pP3RDRegs = (pP3RDRAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);

    //
    // Enable VGA on a permedia
    //

    LOAD_GLINT_CTRL_REG(VideoControl, 0);

    //
    // Need to set up RAMDAC pll pins.
    //

    LOAD_GLINT_CTRL_REG(VClkCtl, pDev->dwVClkCtl & 3);  

    //
    // Reset Pixel Clock
    //

    switch (pDev->dwVClkCtl & 3) {
    case 0:
        // Disable the Pixel Clock
        P3RD_LOAD_INDEX_REG(P3RD_DCLK2_POST_SCALE,     0x6);  
        P3RD_LOAD_INDEX_REG(P3RD_DCLK2_PRE_SCALE,      0x02); // N
        P3RD_LOAD_INDEX_REG(P3RD_DCLK2_FEEDBACK_SCALE, 0x1c); // M
        P3RD_LOAD_INDEX_REG(P3RD_DCLK2_POST_SCALE,     0x0A); // P / Enable
        break;

    case 1:
        // Disable the Pixel Clock
        P3RD_LOAD_INDEX_REG(P3RD_DCLK3_POST_SCALE,     0x6);  
        P3RD_LOAD_INDEX_REG(P3RD_DCLK3_PRE_SCALE,      0x02); // N
        P3RD_LOAD_INDEX_REG(P3RD_DCLK3_FEEDBACK_SCALE, 0x10); // M
        P3RD_LOAD_INDEX_REG(P3RD_DCLK3_POST_SCALE,     0x0A); // P / Enable
        break;

    default:

        //
        // Don't have to reset register set C
        //

        break;
    }

    P3RD_WaitForPixelClockToSync(pDev);

    //
    // Disable all interupts and remove the handler.
    //

    if (pDev->dwFlags & GMVF_DELTA) {
        LOAD_GLINT_CTRL_REG(DeltaIntEnable, 0);
    }

    LOAD_GLINT_CTRL_REG(IntEnable, 0);

    if (pDev->IRQHandle) {

        //
        // Remove cursor update bit
        //

        pDev->pGLInfo->wCursorMode   &= 255;     

        //
        // kill the virtual handler
        //

        VPICD_Force_Default_Behavior(pDev->IRQHandle);  

        pDev->IRQHandle                 = 0;
        pDev->pGLInfo->dwFlags          |= GMVF_NOIRQ;
    }

    //
    // Indicate that we are not in HiRes mode any more
    //

    pDev->pGLInfo->dwHiResMode = 0;

    return (1);
}


//
// This routine is called when the VXD terminates. It is after the VGA has 
// been reenabled. The idea is to give us a second chance to switch the bypass
// TX and SX. On a Permedia, we shouldt do anything at this stage as the 
// VGA should have taken over.
//

DWORD _cdecl 
DisableHardware() 
{
    PDEVICETABLE pDev = FindPrimarypDev();

    //
    // Indicate that we are not in HiRes mode any more
    //

    pDev->pGLInfo->dwHiResMode = 0; 

    return(1);
}


#ifdef POWER_STATE_MANAGE
DWORD _cdecl 
SetAdapterPowerState(
    DWORD           DevNode, 
    DWORD           PowerState)
{
    PDEVICETABLE    pDev = FindpDevFromDevNode(DevNode);
    DWORD           iRet = CR_SUCCESS;

    switch (PowerState) {
    
    case CM_POWERSTATE_D0 : // return to normal state

        //
        // Set return value to default if BIOS POST needs to be done to
        // init the chip after D3 powerdown (powerup is not a reboot)
        //

        if (pDev->dwPowerState == CM_POWERSTATE_D3) {
            iRet = CR_DEFAULT;
        }

        //
        // Reset the chip
        //

        ResetChip(pDev);

        //
        // On return to D0, INT2F DevToForeground is issued.
        // this will call ResetHiresMode and sets up the chip for hires mode.
        // Reenable the interrupt handler if it was disabled.

        if (pDev->IRQHandle == 0) {
            InitializeInterrupts(pDev);
        }

        LOAD_GLINT_CTRL_REG(VideoControl, pDev->pGLInfo->dwVideoControl);

        break;

    case CM_POWERSTATE_D1:          // reduced power
    case CM_POWERSTATE_D2:          // states

    case CM_POWERSTATE_D3:          // device powerdown
    case CM_POWERSTATE_HIBERNATE:   // system powerdown (resumable)

        //
        // In all reduced power states, PCI accesses are disallowed.
        //
        // All state needed to restore on return to D0 should be saved 
        // at this point.
        //
        // In this case, EnableVGA turns off the Permedia2 interrupt and 
        // sets the chip into VGA usable mode since the caller will be setting
        // VGA mode 3. INT2F DevToBackground is issued before
        // SetAdapterPowerState is called.  It causes device bitmaps to be
        // to host memory and invalidates the font cache. DirectDraw/D3D will
        // invalidate all device surfaces on powerdown. Therefore video memory
        // does not need to be saved.
        //

        EnableVGA(pDev);    
    }

    pDev->dwPowerState = PowerState;

    return iRet;
}
#endif // #ifdef POWER_STATE_MANAGE


void 
SwitchToHiResMode(
    PDEVICETABLE        pDev, 
    BOOLEAN             bHiRes)
{
    UCHAR               ucData;
    UCHAR              *pVgaIndexReg;
    UCHAR              *pVgaDataReg;

    //
    // Set up the VGA registers which will be used;
    //

    pVgaIndexReg = (UCHAR *)&pDev->pRegisters->Glint.VGAControlReg;
    pVgaDataReg  = pVgaIndexReg + 1;
    
    //
    // We have to unlock VGA registers on P3 before we can use them
    //

    if (pDev->ChipID == PERMEDIA3_ID) {

        *pVgaIndexReg = PERMEDIA_VGA_LOCK_INDEX1;        
        *pVgaDataReg  = PERMEDIA_VGA_UNLOCK_DATA1;       
        *pVgaIndexReg = PERMEDIA_VGA_LOCK_INDEX2;        
        *pVgaDataReg  = PERMEDIA_VGA_UNLOCK_DATA2;       
    }

    //
    // Enable / Disable the VGA
    //

    *pVgaIndexReg = PERMEDIA_VGA_CTRL_INDEX;
    ucData        = *pVgaDataReg;

    if (bHiRes) {
        ucData &= ~PERMEDIA_VGA_ENABLE;
    } else {
        ucData |= PERMEDIA_VGA_ENABLE;
    }

    *pVgaIndexReg = PERMEDIA_VGA_CTRL_INDEX;
    *pVgaDataReg  = ucData;

    //
    // Lock the VGA registers
    //

    if (pDev->ChipID == PERMEDIA3_ID) {

        *pVgaIndexReg = PERMEDIA_VGA_LOCK_INDEX1;
        *pVgaDataReg  = PERMEDIA_VGA_LOCK_DATA1;
    }
}


//***************************************************************************
// FUNC: P3RD_CalculateMNPForClock
// DESC: calculates prescaler, feedback scaler and postscaler values for the
//       STMACRO PLL71FS used by P3RD.
//***************************************************************************
ULONG 
P3RD_CalculateMNPForClock(
    PDEVICETABLE            pDev,
    ULONG                   RefClock,       // In 100Hz units
    ULONG                   ReqClock,       // In 100Hz units
    ULONG                  *rM,             // M Out (feedback scaler)
    ULONG                  *rN,             // N Out (prescaler)
    ULONG                  *rP)             // P Out (postscaler)
{
    // min fVCO is 200MHz (in 100Hz units)
    const ULONG             fMinVCO = 2000000;      
    // max fVCO is 622MHz (in 100Hz units)
    const ULONG             fMaxVCO = 6220000;      
    // min fINTREF is 1MHz (in 100Hz units)
    const ULONG             fMinINTREF = 10000;     
    // max fINTREF is 2MHz (in 100Hz units)
    const ULONG             fMaxINTREF = 20000;     
    ULONG                   M, N, P;
    ULONG                   fINTREF;
    ULONG                   fVCO;
    ULONG                   ActualClock;
    int                     Error;
    int                     LowestError = INITIALFREQERR;
    BOOLEAN                 bFoundFreq = FALSE;
    int                     cInnerLoopIterations = 0;
    int                     LoopCount;

    for(P = 0; P <= 5; ++P) {

        ULONG fVCOLowest, fVCOHighest;

        //
        // it's pointless going through the main loop if all 
        // values of N produce an fVCO outside the acceptable range
        //

        N = 1;
        M = (N * (1 << P) * ReqClock) / (2 * RefClock);
        fVCOLowest = (2 * RefClock * M) / N;

        N = 255;
        M = (N * (1 << P) * ReqClock) / (2 * RefClock);
        fVCOHighest = (2 * RefClock * M) / N;

        if(fVCOHighest < fMinVCO || fVCOLowest > fMaxVCO) {
      
            continue;
        }

        for (N = 1; N <= 255; ++N, ++cInnerLoopIterations) {

            fINTREF = RefClock / N;

            if(fINTREF < fMinINTREF || fINTREF > fMaxINTREF) {
           
                if(fINTREF > fMaxINTREF) {
                    
                    //
                    // hopefully we'll get into range as the prescale value
                    // increases
                    //

                    continue;

                } else {
                    
                    //
                    // already below minimum and it'll only get worse: move to
                    // the next postscale value
                    //

                    break;
                }
            }

            M = (N * (1 << P) * ReqClock) / (2 * RefClock);
            if(M > 255) {
                
                //
                // M, N & P registers are only 8 bits wide
                //

                break;
            }

            //
            // we can expect rounding errors in calculating M, which will
            // always be rounded down.  So we'll checkout our calculated
            // value of M along with (M+1)
            //

            for (LoopCount = (M == 255) ? 1 : 2; --LoopCount >= 0; ++M) {
                
                fVCO = (2 * RefClock * M) / N;

                if(fVCO >= fMinVCO && fVCO <= fMaxVCO) {
                    
                    ActualClock = fVCO / (1 << P);

                    Error = ActualClock - ReqClock;
                    if(Error < 0)
                        Error = -Error;

                    if(Error < LowestError) {
                        
                        bFoundFreq = TRUE;
                        LowestError = Error;
                        *rM = M;
                        *rN = N;
                        *rP = P;

                        if(Error == 0) {
                            goto Done;
                        }
                    }
                }
            } // End of for (M)
        } // End of for (N)
    } // End of for (P)

Done:
    
    if (bFoundFreq) {
        ActualClock = (2 * RefClock * (*rM)) / ((*rN) * (1 << (*rP)));
    } else {
        ActualClock = 0;
    }

    return (ActualClock);
}

//
// Helper function which programs the P3RD RAMDAC
//

BOOLEAN
Program_P3RD(
    PDEVICETABLE        pDev, 
    VDDDISPLAYINFO     *VideoMode, 
    ULONG               Hsp, 
    ULONG               Vsp, 
    ULONG               RefClkSpeed,
    PULONG              pSystemClock, 
    PULONG              pPixelClock, 
    PULONG              pMemClock)
{
    ULONG               pixelClock;
    ULONG               coreClock;
    P3RDRAMDAC         *pP3RDRegs;
    // ULONG               DacDepth, depth;
    ULONG               depth;
    ULONG               index;
    ULONG               color;
    ULONG               ulValue;
    UCHAR               pixelCtrl;
    ULONG               mClkSrc = 0, sClkSrc = 0;
    BOOLEAN             status = FALSE;
    ULONG               M, N, P;

    //
    // If we are using 64-bit mode then we need to double the pixel 
    // clock and set the pixel doubling bit in the RAMDAC.
    //

    if (pDev->PXRXCapabilities & PXRX_USE_BYTE_DOUBLING) {
        pixelClock = (*pPixelClock) << 1;
    } else {
        pixelClock = (*pPixelClock);        
    }

    //
    // Double the desired system clock as P3 has a divider, which divides 
    // this down again.
    //

    coreClock = (*pSystemClock << 1);
                                              
    pP3RDRegs = (P3RDRAMDAC *)&pDev->pRegisters->Glint.ExtVCReg;

    //
    // Get MClkCtl source field override from registry
    //

    // status = VideoPortGetRegistryParameters(
    //              HwDeviceExtension,
    //              GLINT_REG_STRING_PXRX_MEMCLKSRC,
    //              FALSE,
    //              Perm3RegistryCallback,
    //              &mClkSrc);
    if (status) {
        mClkSrc = mClkSrc << 4;
    } else {

        if (pDev->bHaveExtendedClocks) {
            mClkSrc = pDev->ulPXRXMemoryClockSrc; 
        } else {
            mClkSrc = P3RD_DEFAULT_MCLK_SRC;
        }
    } 

    //
    // Get SClkCtl source field override from registry
    //

    // status = VideoPortGetRegistryParameters(
    //              HwDeviceExtension,
    //              GLINT_REG_STRING_PXRX_SETUPCLKSRC,
    //              FALSE,
    //              Perm3RegistryCallback,
    //              &sClkSrc);
    if (status) {
        sClkSrc = sClkSrc << 4;
    } else { 

        if (pDev->bHaveExtendedClocks) {
            sClkSrc = pDev->ulPXRXSetupClockSrc;
        } else {
            sClkSrc = P3RD_DEFAULT_SCLK_SRC;
        }
    }

    depth = VideoMode->ddiBpp;

    //
    // For timing calculations need full depth in bits
    //

    // if ((DacDepth = depth) == 15) {
    //     DacDepth = 16;
    // } else {

    //    if (depth == 12) {
    //        DacDepth = 32;
    //    }
    //}

    //
    // Set up Misc Control
    //

    P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);

    ulValue &= ~(P3RD_MISC_CONTROL_HIGHCOLORRES | 
                 P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED |
                 P3RD_MISC_CONTROL_PIXEL_DOUBLE);

    // if ((VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_X_BY2)) {
   
        //
        // it's a really low resolution (e.g. 320x200) so enable pixel 
        // doubling in the RAMDAC (we'll enable line doubling in the pixel
        // unit too)
        //

        // P3RD_LOAD_INDEX_REG(
        //     P3RD_MISC_CONTROL, 
        //     ulValue | P3RD_MISC_CONTROL_HIGHCOLORRES | 
        //         P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED |
        //         P3RD_MISC_CONTROL_PIXEL_DOUBLE);
    // } else {
   
        P3RD_LOAD_INDEX_REG(
            P3RD_MISC_CONTROL, 
            ulValue | P3RD_MISC_CONTROL_HIGHCOLORRES | 
            P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED);
    // }

    VideoPortWriteRegisterUlong(
        P3RD_INDEX_CONTROL, P3RD_IDX_CTL_AUTOINCREMENT_ENABLED);

    if (Hsp) {
        ulValue = P3RD_SYNC_CONTROL_HSYNC_ACTIVE_HIGH;
    } else {
        ulValue = P3RD_SYNC_CONTROL_HSYNC_ACTIVE_LOW;
    }
    if (Vsp) {
        ulValue |= P3RD_SYNC_CONTROL_VSYNC_ACTIVE_HIGH;
    } else {
        ulValue |= P3RD_SYNC_CONTROL_VSYNC_ACTIVE_LOW;
    }

    P3RD_LOAD_INDEX_REG(P3RD_SYNC_CONTROL, ulValue);
    P3RD_LOAD_INDEX_REG(
        P3RD_DAC_CONTROL,  P3RD_DAC_CONTROL_BLANK_PEDESTAL_ENABLED);

    ulValue = 0;

#if __NOT_SUPPORTED_YET__
    if ((VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_X_BY2)||
        (pDev->PXRXCapabilities & PXRX_USE_BYTE_DOUBLING)) {
   
        ulValue |= P3RD_CURSOR_CONTROL_DOUBLE_X;
    }

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_Y_BY2) {
   
        ulValue |= P3RD_CURSOR_CONTROL_DOUBLE_Y;
    }
#endif

    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_CONTROL,   ulValue);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_MODE,      0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_X_LOW,     0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_X_HIGH,    0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_Y_LOW,     0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_Y_HIGH,    0xff);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_HOTSPOT_X, 0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_HOTSPOT_Y, 0);
    P3RD_LOAD_INDEX_REG(P3RD_PAN, 0);

    //
    // The first 3-color cursor is the mini cursor which is 
    // always black & white. Set it up here
    //

    P3RD_CURSOR_PALETTE_CURSOR_RGB(P3RD_CALCULATE_LUT_INDEX(0), 0x00,0x00,0x00);
    P3RD_CURSOR_PALETTE_CURSOR_RGB(P3RD_CALCULATE_LUT_INDEX(1), 0xff,0xff,0xff);

    //
    // Stop all clocks
    //

    P3RD_LOAD_INDEX_REG(P3RD_DCLK_CONTROL, 0);
    P3RD_LOAD_INDEX_REG(P3RD_KCLK_CONTROL, 0);
    P3RD_LOAD_INDEX_REG(P3RD_MCLK_CONTROL, 0);
    P3RD_LOAD_INDEX_REG(P3RD_SCLK_CONTROL, 0);

    //
    // If we are on an R3 board then we support external clocks.
    //

    if (pDev->PXRXCapabilities & PXRX_EXTERNAL_CLOCK) {

        //
        // Shall we configure MCLK to be driven externally
        //

        if (mClkSrc & 
                (P3RD_MCLK_CONTROL_EXTMCLK | P3RD_MCLK_CONTROL_HALF_EXTMCLK)) {

            ULONG mClkSpeed = 0;

            //
            // Get MCLK speed override from registry
            //

            // (void) VideoPortGetRegistryParameters(
            //             HwDeviceExtension,
            //             GLINT_REG_STRING_PXRX_MEMCLKSPEED,
            //             FALSE,
            //             Perm3RegistryCallback,
            //             &mClkSpeed);

            //
            // If mClkSpeed is zero (or the registry entry doesn't exist) then
            // set it to our built-in default.
            //

            if (mClkSpeed == 0) {

                //
                // Convert from Hz to 100 Hz
                //

                mClkSpeed = PERMEDIA3_DEFAULT_MCLK_SPEED / 100; 

            } else {

                //
                // Convert from MHz to 100 Hz.
                //

                mClkSpeed *= 10000;
            }

            *pMemClock = mClkSpeed;
        }

        //
        // Shall we configure SCLK to be driven externally
        //

        if (sClkSrc & 
                (P3RD_SCLK_CONTROL_EXTSCLK | P3RD_SCLK_CONTROL_HALF_EXTSCLK)) {

            ULONG sClkSpeed = 0;

            //
            // Get SCLK speed override from registry
            //

            // (void) VideoPortGetRegistryParameters(
            //             HwDeviceExtension,
            //             GLINT_REG_STRING_PXRX_SETUPCLKSPEED,
            //             FALSE,
            //             Perm3RegistryCallback,
            //             &sClkSpeed);

            //
            // If sClkSpeed is zero (or the registry entry doesn't exist) then
            // set it to our built-in default.
            //

            if (sClkSpeed == 0) {

                //
                // Convert from Hz to 100 Hz
                //

                sClkSpeed = PERMEDIA3_DEFAULT_SCLK_SPEED / 100;    

            } else {

                //
                // Convert from MHz to 100 Hz.
                //

                sClkSpeed *= 10000;
            }
        }
    }

    //
    // Belt and braces let's set MCLK to something just in case
    // Let's enable SCLK and MCLK.
    //

    if (*pMemClock == 0) {

        //
        // Convert from Hz to 100 Hz
        //

        *pMemClock = PERMEDIA3_DEFAULT_MCLK_SPEED / 100; 
    }

    DISPDBG((
        "Program_P3RD: mClkSrc 0x%x, sClkSrc 0x%x, mspeed %d00\n", 
        mClkSrc, sClkSrc, *pMemClock));

    P3RD_LOAD_INDEX_REG(
        P3RD_MCLK_CONTROL, 
        P3RD_MCLK_CONTROL_ENABLED | P3RD_MCLK_CONTROL_RUN | mClkSrc);

    P3RD_LOAD_INDEX_REG(
        P3RD_SCLK_CONTROL, 
        P3RD_SCLK_CONTROL_ENABLED | P3RD_SCLK_CONTROL_RUN | sClkSrc);

    // if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_X_BY2) {
   
        //
        // We're doubling each pixel so we double the pixel clock too
        // (I think this means that each pixel is drawn twice on the screen,
        //  but the second pixel is drawn over the first)
        // NB. the PixelDouble field of RDMiscControl needs to be set also)
        //

        // pixelClock *= 2;
    // }

    pixelClock = P3RD_CalculateMNPForClock(
                     pDev, RefClkSpeed, pixelClock, &M, &N, &P);

    if(pixelClock == 0) {

        DISPDBG((
            "Program_P3RD: P3RD_CalculateMNPForClock(PixelClock) failed\n"));

        return(FALSE);
    }

    //
    // Load both copies of the dot clock with our times 
    // (DCLK0 & DCLK1 reserved for VGA only)
    //

    P3RD_LOAD_INDEX_REG(P3RD_DCLK2_PRE_SCALE,      N);
    P3RD_LOAD_INDEX_REG(P3RD_DCLK2_FEEDBACK_SCALE, M);
    P3RD_LOAD_INDEX_REG(P3RD_DCLK2_POST_SCALE,     P);

    P3RD_LOAD_INDEX_REG(P3RD_DCLK3_PRE_SCALE,      N);
    P3RD_LOAD_INDEX_REG(P3RD_DCLK3_FEEDBACK_SCALE, M);
    P3RD_LOAD_INDEX_REG(P3RD_DCLK3_POST_SCALE,     P);

    coreClock = P3RD_CalculateMNPForClock(
                    pDev, RefClkSpeed, coreClock, &M, &N, &P);

    if(coreClock == 0) {
   
        DISPDBG((
            "Program_P3RD: P3RD_CalculateMNPForClock(SystemClock) failed\n"));

        return (FALSE);
    }

    //
    // Load the core clock
    //

    P3RD_LOAD_INDEX_REG(P3RD_KCLK_PRE_SCALE,      N);
    P3RD_LOAD_INDEX_REG(P3RD_KCLK_FEEDBACK_SCALE, M);
    P3RD_LOAD_INDEX_REG(P3RD_KCLK_POST_SCALE,     P);

    //
    // enable the dot clock
    //

    P3RD_LOAD_INDEX_REG(
        P3RD_DCLK_CONTROL, P3RD_DCLK_CONTROL_ENABLED | P3RD_DCLK_CONTROL_RUN);

    M = 0x100000;

    do {
   
        P3RD_READ_INDEX_REG(P3RD_DCLK_CONTROL, ulValue);
    } while((ulValue & P3RD_DCLK_CONTROL_LOCKED) == FALSE && --M);

    if ((ulValue & P3RD_DCLK_CONTROL_LOCKED) == FALSE) {
   
        DISPDBG(("Program_P3RD: PixelClock failed to lock\n"));

        return (FALSE);
    }

    //
    // Enable the core clock
    //

    P3RD_LOAD_INDEX_REG(
        P3RD_KCLK_CONTROL, 
        P3RD_KCLK_CONTROL_ENABLED | 
        P3RD_KCLK_CONTROL_RUN | P3RD_KCLK_CONTROL_PLL);

    M = 0x100000;
    do {
   
        P3RD_READ_INDEX_REG(P3RD_KCLK_CONTROL, ulValue);
    } while((ulValue & P3RD_KCLK_CONTROL_LOCKED) == FALSE && --M);

    if ((ulValue & P3RD_KCLK_CONTROL_LOCKED) == FALSE) {
   
        DISPDBG((0, "Program_P3RD: SystemClock failed to lock\n"));

        return (FALSE);
    }

    //
    // This register doesn't have a reasonable default after reboot,
    // and it is anded with pixel values before they are used. 0XF for
    // 12bit color, and 0XFF for all other color depthes.
    //

    P3RD_SET_PIXEL_READMASK(0xff);

    switch (depth) {

        case 8:
        
            P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
            ulValue &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
            P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
            P3RD_LOAD_INDEX_REG(P3RD_PIXEL_SIZE, P3RD_PIXEL_SIZE_8BPP);
    
            if (pDev->Capabilities & CAPS_8BPP_RGB) {
           
                ULONG    Red, Green, Blue ;
    
                DISPDBG(("Program_P3RD: Load 8bpp RGB colormap\n"));
                P3RD_LOAD_INDEX_REG(
                    P3RD_COLOR_FORMAT, 
                    P3RD_COLOR_FORMAT_8BPP | P3RD_COLOR_FORMAT_RGB);
    
                for (index = 0; index <= 0xff; ++index) {
    
                    Red   = bPal8[index & 0x07];
                    Green = bPal8[(index >> 3 ) & 0x07];
                    Blue  = bPal4[(index >> 6 ) & 0x03];
                    if(Red == Green) {  // Maybe it's a special case of gray ?
    
                        //
                        // Blue = 10, Green = 100 or 101,
                        // This is a tricky part:
                        // the Blue field in BGR 2:3:3 color goes thru steps 
                        // 00, 01, 10, 11 (Binary), the Red and Green go thru 
                        // 000, 001, 010, 011, 100, 101, 110, 111 (Binary)
                        // We load the special gray values ONLY when Blue color 
                        // is close in intensity to both Green and Red, i.e.
                        // Blue = 01, Green = 010 or 011,
                        // Blue = 10, Green = 100 or 101,
                        //
    
                        if (((index >> 1) & 0x03) == ((index >> 6 ) & 0x03 )) {
    
                            Blue = Red;
                        }
                    }
    
                    LUT_CACHE_SETRGB (index, Red, Green, Blue);
                } // End of for
    
            } else {
    
                //
                // Color indexed mode
                //
    
                P3RD_LOAD_INDEX_REG(
                    P3RD_COLOR_FORMAT, 
                    P3RD_COLOR_FORMAT_CI8 | P3RD_COLOR_FORMAT_RGB);
            }
    
            break;

        case 15:
        case 16:

            P3RD_LOAD_INDEX_REG(P3RD_PIXEL_SIZE, P3RD_PIXEL_SIZE_16BPP);

#if  GAMMA_CORRECTION
            P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
            ulValue &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
            P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
    
            //
            // load linear ramp into LUT as default
            //
    
            for (index = 0; index <= 0xff; ++index) {
                LUT_CACHE_SETRGB (index, index, index, index);
            }
    
            pixelCtrl = 0;
#else
            P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
            ulValue |= P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
            P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
    
            pixelCtrl = P3RD_COLOR_FORMAT_LINEAR_EXT;
#endif
            //
            // Win9x GDI doesn't dictate which 16 bit mode the driver uses,
            // the driver can freely choose to use 5:6:5 or 1:5:5:5 mode based
            // hardware support and efficiency, and the driver can communicate
            // this information back to the DIB Engine through the DIBENGINE
            // structure's deBitmapInfo. (specifically the biCompression and 
            // RGB bitmasks(Palette) of BITMAPINFO)
            //
            // P3 hardware can support both 1:5:5:5 and 5:6:5, but since the
            // display driver always use 5:6:5, so a fixed values is used.
            //

            pixelCtrl |= P3RD_COLOR_FORMAT_16BPP;
            // For 1:5:5:5 16 bit mode, use the following
            // pixelCtrl |= P3RD_COLOR_FORMAT_15BPP;
  
            pixelCtrl |= P3RD_COLOR_FORMAT_RGB;
    
            VideoDebugPrint((3, "P3RD_COLOR_FORMAT = 0x%x\n", pixelCtrl));
    
            P3RD_LOAD_INDEX_REG(P3RD_COLOR_FORMAT, pixelCtrl);
            break;

        case 12:
        case 24:
        case 32:

            P3RD_LOAD_INDEX_REG(P3RD_PIXEL_SIZE, P3RD_PIXEL_SIZE_32BPP);
            P3RD_LOAD_INDEX_REG(
                P3RD_COLOR_FORMAT, 
                P3RD_COLOR_FORMAT_32BPP | P3RD_COLOR_FORMAT_RGB);
    
            if (depth == 12) {
    
                USHORT cacheIndex;
    
                P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
                ulValue &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
                P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
    
                //
                // Use auto-increment to load a ramp into entries 0 to 15
                //
    
                DISPDBG(("12 BPP. loading palette\n"));
                for (index = 0, cacheIndex = 0; 
                     index <= 0xff; index += 0x11, cacheIndex++) {
    
                    LUT_CACHE_SETRGB (index, index, index, index);
                }
    
                //
                // Load ramp in every 16th entry from 16 to 240
                //
    
                color = 0x11;
    
                for (index = 0x10; 
                    index <= 0xf0; index += 0x10, color += 0x11) {
                    LUT_CACHE_SETRGB (index, color, color, color);
                }
    
                P3RD_SET_PIXEL_READMASK(0x0f);
     
            } else {

#if  GAMMA_CORRECTION
                P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
                ulValue &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
                P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
    
                //
                // load linear ramp into LUT as default
                //
    
                for (index = 0; index <= 0xff; ++index) {
                    LUT_CACHE_SETRGB (index, index, index, index);
                }
#else
                P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
                ulValue |= P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
                P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
    
#endif  // GAMMA_CORRECTION

            }
            break;

          default:

            DISPDBG(("Program_P3RD: bad depth %d \n", depth));

            return(FALSE);
    }

    //
    // Blank the analogue display if we are using a DFP, also re-program the 
    // DFPO because the BIOS may have trashed some of the registers
    // that we programmed at start of day.
    //

    if (pDev->PXRXCapabilities & PXRX_DFP_MON_ATTACHED) {

        // BYTE    val;

        //
        // Only blank the CRT if the mode is less than 60Hz refresh.
        //

        // if (VideoMode->ScreenFrequency < 60) {
            // P3RD_LOAD_INDEX_REG(
            //     P3RD_DAC_CONTROL, 
            //     (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) );
            // P3RD_LOAD_INDEX_REG(P3RD_DAC_CONTROL, (1 << 0));
            // val = 1;
            // P3RD_LOAD_INDEX_REG(P3RD_DAC_CONTROL, val);
        // }

        // PXRXProgramDFP (pDev);     // Program the DFP registers
    }

    // Return these values
    //*pPixelClock = pixelClock;
    //*pSystemClock = coreClock;

    switch (mClkSrc) {

        case P3RD_MCLK_CONTROL_HALF_PCLK:
            *pMemClock = 33 * 10000;
            break;

        case P3RD_MCLK_CONTROL_PCLK:
            *pMemClock = 66 * 10000;
            break;

        case P3RD_MCLK_CONTROL_HALF_EXTMCLK:
            *pMemClock = *pMemClock / 2;
            break;

        case P3RD_MCLK_CONTROL_EXTMCLK:
            //*pMemClock = *pMemClock;
            break;

        case P3RD_MCLK_CONTROL_HALF_KCLK:
            *pMemClock = (coreClock >> 1) / 2;
            break;

        case P3RD_MCLK_CONTROL_KCLK:
            *pMemClock = (coreClock >> 1);
            break;
    }

    return(TRUE);
}
