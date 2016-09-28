/***************************************************************************\
*
*                        ***********************************
*                        * Permedia 3: miniVDD SAMPLE CODE *
*                        ***********************************
*
* Module Name:
*
*   perm3mem.c
*
* Abstract:
*
*   This module contains code to allocate physically contiguous memory
*   on behalf of the 2D acceleration DLL.
*
* Environment:
*
*   Kernel mode
*
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
* Copyright (c) 1995-2001 Microsoft Corporation.  All Rights Reserved.
*
\***************************************************************************/

#include <basedef.h>
#include <vmm.h>
#include <vmmreg.h>
#include <vwin32.h>
#include <winerror.h>
#include "perm3.h"
#include <regdef.h>


VOID 
CopyROMInitializationTable(
    PDEVICETABLE            hwDeviceExtension,
    PVOID                   pvROMAddress)

/*+++

Routine Description:

    This function should be called for devices that have an expansion ROM
    which contains a register initialisation table. The function assumes
    he ROM is present and enabled.

Arguments:

    HwDeviceExtension -
        Points to the device extension of the device whose ROM is to be read

    pvROMAddress - 
        Base address of the expansion ROM. This function assumes that the 
        offset to the initialisation table is defined at 0x1c from the 
        beginning of ROM


Return Value:

   void

---*/

{
    PULONG                  pulROMTable;
    ULONG                   cEntries;
    PULONG                  pul;
    ULONG                   ul, regHdr;

    hwDeviceExtension->culTableEntries = 0;

    //
    // the 2-byte offset to the initialization table is given at 0x1c
    // from the start of ROM
    //

    ul = VideoPortReadRegisterUshort((USHORT *)(0x1c + (PCHAR)pvROMAddress));
    pulROMTable = (PULONG)(ul + (PCHAR)pvROMAddress);
    
    //
    // the table header (32 bits) has an identification code and a count
    // of the number of entries in the table
    //

    regHdr = VideoPortReadRegisterUlong(pulROMTable++);

    while ((regHdr >> 16) == 0x3d3d) {
   
        ULONG BiosID = (regHdr >> 8) & 0xFF;

        switch (BiosID){
        
            case 0:

                //
                //    BIOS partition 0
                //    ----------------
                //    This BIOS region holds the memory timings for P3 chip.
                //    We also look up the version number.
                //

                DISPDBG(("CopyROMInitialisationTable: Got memory timings\n"));

                //
                // the 2-byte BIOS version number in in the form of
                // <MajorNum>.<MinorNum>

                hwDeviceExtension->BiosVersionMajorNumber =
                        (ULONG)VideoPortReadRegisterUchar(
                                        (PCHAR)(0x7 + (PCHAR)pvROMAddress) ); 
                                       
                                                     
                hwDeviceExtension->BiosVersionMinorNumber = (ULONG)VideoPortReadRegisterUchar((PCHAR)(0x8 + (PCHAR)pvROMAddress)); 

                //
                // number of register address & data pairs
                //

                cEntries = regHdr & 0xffff;

                if(cEntries > 0) {
               
                    //
                    // this assert, and the one after the copy should ensure
                    // we don't write past the end of the table
                    //

                    DISPASSERT((cEntries * sizeof(ULONG) * 2 <= sizeof(hwDeviceExtension->aulInitializationTable))); 
                    // "ERROR: too many initialization entries\n");

                    //
                    // each entry contains two 32-bit words
                    //

                    pul = hwDeviceExtension->aulInitializationTable;
                    ul = cEntries << 1;

                    while(ul--) {
                   
                        *pul++ = VideoPortReadRegisterUlong(pulROMTable);
                        ++pulROMTable;
                    }

                    hwDeviceExtension->culTableEntries = 
                                      (ULONG)(pul - 
                                             (ULONG *)hwDeviceExtension->aulInitializationTable) >> 1;

                    DISPASSERT((cEntries == hwDeviceExtension->culTableEntries)); 
                    // "ERROR: generated different size init table to that expected\n");

#if DBG
                    //
                    // output the initialization table
                    //

                    pul = hwDeviceExtension->aulInitializationTable;
                    ul = hwDeviceExtension->culTableEntries;

                    while(ul--) {
                   
                        ULONG ulReg;
                        ULONG ulRegData;

                        ulReg = *pul++;
                        ulRegData = *pul++;
                        DISPDBG((
                            "CopyROMInitializationTable: initialising register %08.8Xh with %08.8Xh\n", 
                            ulReg, ulRegData));
                    }
#endif //DBG
                }

                break;

            case 1:

                //
                //    BIOS partition 1
                //    ----------------
                //    This BIOS region holds the extended clock settings
                //    for PXRX chips.
                //

                DISPASSERT(((regHdr & 0xffff) == 0x0103))
                // "ERROR: Extended table doesn't have right cnt/ID\n");

                //
                // Some P3 boards have a whole set of clocks defined in
                // the BIOS. The high nibble defines the source for the
                // clock, this isn't relevant for anything but MCLK and
                // SCLK on PXRX.
                //

                hwDeviceExtension->bHaveExtendedClocks  = TRUE;

                hwDeviceExtension->ulPXRXCoreClock = 
                    ( VideoPortReadRegisterUlong(pulROMTable++) & 0xFFFFFF )
                                                               * 1000 * 1000;

                hwDeviceExtension->ulPXRXMemoryClock = 
                    VideoPortReadRegisterUlong(pulROMTable++);

                hwDeviceExtension->ulPXRXMemoryClockSrc = 
                    (hwDeviceExtension->ulPXRXMemoryClock >> 28) << 4;

                hwDeviceExtension->ulPXRXMemoryClock = 
                    (hwDeviceExtension->ulPXRXMemoryClock & 0xFFFFFF)
                                                        * 1000 * 1000;

                hwDeviceExtension->ulPXRXSetupClock = 
                    VideoPortReadRegisterUlong(pulROMTable++);

                hwDeviceExtension->ulPXRXSetupClockSrc = 
                    (hwDeviceExtension->ulPXRXSetupClock >> 28) << 4;

                hwDeviceExtension->ulPXRXSetupClock = 
                    (hwDeviceExtension->ulPXRXSetupClock & 0xFFFFFF)
                                                      * 1000 * 1000;

                hwDeviceExtension->ulPXRXGammaClock = 
                    (VideoPortReadRegisterUlong(pulROMTable++) & 0xFFFFFF)
                                                            * 1000 * 1000;

                hwDeviceExtension->ulPXRXCoreClockAlt = 
                    (VideoPortReadRegisterUlong(pulROMTable++) & 0xFFFFFF)
                                                            * 1000 * 1000;

                //
                // Oops the GVX1 2.09 BIOS has the wrong values in it, remove 
                // 10Mhz from coreclock and coreclockALT
                //

                if (GLINT_GAMMA_PRESENT && 
                    hwDeviceExtension->BiosVersionMajorNumber == 2 && 
                    hwDeviceExtension->BiosVersionMinorNumber == 9 ) {
               
                    hwDeviceExtension->ulPXRXCoreClockAlt -= 10 * 1000 * 1000;
                    hwDeviceExtension->ulPXRXCoreClock    -= 10 * 1000 * 1000;
                }
#if DBG
                DISPDBG((
                    "CopyROMInitializationTable: core clock %d, core clock alt %d\n", 
                    hwDeviceExtension->ulPXRXCoreClock, 
                    hwDeviceExtension->ulPXRXCoreClockAlt));

                DISPDBG((
                    "CopyROMInitializationTable: Mem clock %d, mem clock src 0x%x\n", 
                    hwDeviceExtension->ulPXRXMemoryClock, 
                    hwDeviceExtension->ulPXRXMemoryClockSrc));

                DISPDBG((
                    "CopyROMInitializationTable: setup clock %d, setup clock src 0x%x\n", 
                    hwDeviceExtension->ulPXRXSetupClock, 
                    hwDeviceExtension->ulPXRXSetupClockSrc));

                DISPDBG((
                    "CopyROMInitializationTable: Gamma clock %d\n", 
                    hwDeviceExtension->ulPXRXGammaClock));
#endif    // DBG

                break;
        
            default:
                DISPDBG(( 
                     "CopyROMInitializationTable: Extended table doesn't have right cnt/ID !\n"));
        }
    
        regHdr = VideoPortReadRegisterUlong(pulROMTable++);
    }
}

VOID 
GenerateInitializationTable(
    PDEVICETABLE            hwDeviceExtension)

/*+++

Routine Description:

    Creates a register initialization table (called if we can't read one
    from ROM). If VGA is enabled the registers are already initialized so
    we just read them back, otherwise we have to use default values

---*/

{
    ULONG                   cEntries;
    PULONG                  pul;
    // ULONG                   ul;
    // int                     i, j;
    GLREG                  *pCtrlRegs;
    
    // pGlintControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];
    // GLINT_REG *aCoreRegs = &(pCtrlRegs->coreRegs[0]);
    
    pCtrlRegs = &hwDeviceExtension->pRegisters->Glint;

    hwDeviceExtension->culTableEntries = 0;

    if (hwDeviceExtension->ChipID == PERMEDIA3_ID) {
   
        cEntries = 4;

        //
        // this assert, and the one after the copy should ensure we don't
        // write past the end of the table
        //

        DISPASSERT((cEntries * sizeof(ULONG) * 2 <= sizeof(hwDeviceExtension->aulInitializationTable))) 
        // "ERROR: too many initialization entries\n");

        //
        // each entry contains two 32-bit words
        //

        pul = hwDeviceExtension->aulInitializationTable;

        if (hwDeviceExtension->bVGAEnabled) {
       
            //
            // No initialisation table but VGA is running so our key
            // registers have been initialized to sensible values
            //

            DISPDBG(("GenerateInitialisationTable: VGA enabled: reading registers\n"));

            //
            // key entries are: ROM control, Boot Address, Memory Config
            // and VStream Config
            //

            *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
            *pul++ = VideoPortReadRegisterUlong(PXRX_LOCAL_MEM_CAPS);

            *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
            *pul++ = VideoPortReadRegisterUlong(PXRX_LOCAL_MEM_CONTROL);

            *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
            *pul++ = VideoPortReadRegisterUlong(PXRX_LOCAL_MEM_REFRESH);

            *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
            *pul++ = VideoPortReadRegisterUlong(PXRX_LOCAL_MEM_TIMING);

        } else {

            if (hwDeviceExtension->PXRXDefaultMemoryType == 
                                   PXRX_MEMORY_TYPE_DONTPROGRAM) {

                //           
                // Don't set up the memory timing registers.
                //           

                cEntries = 0;

            } else if (hwDeviceExtension->PXRXDefaultMemoryType == 
                       PXRX_MEMORY_TYPE_SGRAM_32MB) {
           
                //
                // No initialization table and no VGA. Use default SGRAM 
                // values for  32MB of SAMSUNG SEC KM4132G512A, with a 
                // 125Mhz MClk.
                //

                DISPDBG(("PERM3: GenerateInitializationTable() VGA disabled - using default SGRAM values\n"));

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
                *pul++ = 0x30E311B8; // make use of the Interleave and CombineBanks option on the memory controller

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
                *pul++ = 0x08000002; // figures for 80 MHz

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
                *pul++ = 0x08501204;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
                *pul++ = 0x0000006B;

            } else if (hwDeviceExtension->PXRXDefaultMemoryType == 
                       PXRX_MEMORY_TYPE_SDRAM_16MB) {
           
                //
                // No initialization table and no VGA. Use default SDRAM values.
                // 16MB of MICRON MT48LC1M16A1-8A with a 125Mhz MClk, we are 
                // apparently using SIEMENS memories but the numbers will 
                // hopefully be the same.
                //

                DISPDBG(("PERM3: GenerateInitializationTable() VGA disabled - using default SDRAM values\n"));

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
                *pul++ = (8  << PXRX_LM_CAPS_COLUMN_SHIFT       ) |
                         (11 << PXRX_LM_CAPS_ROW_SHIFT          ) |    
                         (1  << PXRX_LM_CAPS_BANK_SHIFT         ) |
                         (1  << PXRX_LM_CAPS_CHIPSEL_SHIFT      ) |    
                         (3  << PXRX_LM_CAPS_PAGESIZE_SHIFT     ) |    
                         (14 << PXRX_LM_CAPS_REGIONSIZE_SHIFT   ) |
                         (0 <<  PXRX_LM_CAPS_NOPRECHARGE_SHIFT  ) |
                         (0 <<  PXRX_LM_CAPS_SPECIALMODE_SHIFT  ) |
                         (0  << PXRX_LM_CAPS_TWOCOLOR_SHIFT     ) |
                         (0  << PXRX_LM_CAPS_COMBINEBANKS_SHIFT ) |
                         (1  << PXRX_LM_CAPS_NOWRITEMASK_SHIFT  ) |
                         (1  << PXRX_LM_CAPS_NOBLOCKFILL_SHIFT  ) |
                         (0  << PXRX_LM_CAPS_HALFWIDTH_SHIFT    ) |
                         (0  << PXRX_LM_CAPS_NOLOOKAHEAD_SHIFT    );

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
                *pul++ = (2  << PXRX_LM_CTL_CASLATENCY_SHIFT     ) |
                         (0  << PXRX_LM_CTL_INTERLEAVE_SHIFT     ) |    
                         (32 << PXRX_LM_CTL_MODE_SHIFT             );

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
                *pul++ = (1  << PXRX_LM_REF_ENABLE_SHIFT         ) |
                         (57 << PXRX_LM_REF_DELAY_SHIFT            );

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
                *pul++ = (1  << PXRX_LM_TMG_TURNON_SHIFT         ) |
                         (1  << PXRX_LM_TMG_TURNOFF_SHIFT        ) |    
                         (1  << PXRX_LM_TMG_REGLOAD_SHIFT        ) |    
                         (0  << PXRX_LM_TMG_BLKWRITE_SHIFT       ) |    
                         (3  << PXRX_LM_TMG_ACTTOCMD_SHIFT       ) |    
                         (3  << PXRX_LM_TMG_PRETOACT_SHIFT       ) |    
                         (0  << PXRX_LM_TMG_BLKWRTTOPRE_SHIFT    ) |    
                         (0  << PXRX_LM_TMG_WRTTOPRE_SHIFT       ) |    
                         (6  << PXRX_LM_TMG_ACTTOPRE_SHIFT       ) |    
                         (9  << PXRX_LM_TMG_REFCYCLE_SHIFT         );

            } else if (hwDeviceExtension->PXRXDefaultMemoryType == 
                       PXRX_MEMORY_TYPE_SGDEMUL) {

                //
                // No initialization table and no VGA. Use default SGRAM values 
                // for 32MB of SAMSUNG SEC KM4132G512A, with a 125Mhz MClk.
                //

                DISPDBG(("PERM3: GenerateInitializationTable() VGA disabled - using SDRAM values on a SGRAM board\n"));

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
                *pul++ = 0x3AE321A8;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
                *pul++ = 0x08000002;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
                *pul++ = 0x0000004D;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
                *pul++ = 0x06300905;

            } else if (hwDeviceExtension->PXRXDefaultMemoryType == 
                       PXRX_MEMORY_TYPE_SGDEMUL64) {
                //
                // No initialization table and no VGA. Use default SGRAM values for 
                // 32MB of SAMSUNG SEC KM4132G512A, with a 125Mhz MClk.
                //

                DISPDBG(("PERM3: GenerateInitializationTable() VGA disabled - using SDRAM values on a SGRAM board\n"));

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
                *pul++ = 0x7AD221A8;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
                *pul++ = 0x08000002;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
                *pul++ = 0x0000004D;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
                *pul++ = 0x06300905;

            } else if (hwDeviceExtension->PXRXDefaultMemoryType == 
                       PXRX_MEMORY_TYPE_HANDMADE) {
           
                ULONG    memCaps     = 0x02E421A8;    // 80 Mhz SGRAM defaults
                ULONG    memControl  = 0x0800000A;
                ULONG    memTiming   = 0x06300905;
                ULONG    memRefresh  = 0x0000004D;

                DISPDBG(("PERM3: GenerateInitializationTable() VGA disabled - using hand-crafted values on any board\n"));

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
                *pul++ = memCaps;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
                *pul++ = memControl;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
                *pul++ = memRefresh;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
                *pul++ = memTiming;

            } else if (hwDeviceExtension->PXRXDefaultMemoryType == 
                       PXRX_MEMORY_TYPE_SGRAM_16MB) {
           
                //
                // no initialisation table and no VGA. Use default SGRAM values for 
                // 32MB of SAMSUNG SEC KM4132G512A, with a 125Mhz MClk.
                //

                DISPDBG(("PERM3: GenerateInitializationTable() VGA disabled - using default SGRAM values\n"));

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
                *pul++ = 0x0AE321A8; 

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
                *pul++ = 0x0C000003; 

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
                *pul++ = 0x0000002F;

                *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
                *pul++ = 0x08501105;
            }
        }
    }

    hwDeviceExtension->culTableEntries = (ULONG)(pul - (ULONG *)hwDeviceExtension->aulInitializationTable) >> 1;

#if DBG
    if (cEntries != hwDeviceExtension->culTableEntries) {
        DISPDBG(("ERROR: generated different size init table to that expected\n"));
    }

    //
    // output the initialization table
    //

    pul = hwDeviceExtension->aulInitializationTable;
    ul = hwDeviceExtension->culTableEntries;
    while(ul--) {
        
        ULONG ulReg;
        ULONG ulRegData;

        ulReg = *pul++;
        ulRegData = *pul++;
        DISPDBG((
            "GenerateInitializationTable: initialising register %08.8Xh with %08.8Xh\n", 
            ulReg, ulRegData));
    }
#endif //DBG
}

VOID 
ProcessInitializationTable(
    PDEVICETABLE            hwDeviceExtension)

/*+++

Routine Description:
    this function processes the register initialization table

---*/

{
    PULONG                  pul;
    ULONG                   cul;
    ULONG                   ulRegAddr, ulRegData;
    PULONG                  pulReg;
    ULONG                   BaseAddrSelect;
    GLREG                  *pCtrlRegs;

    // pGlintControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];
    // GLINT_REG *aCoreRegs = &(pCtrlRegs->coreRegs[0]);

    pCtrlRegs = &hwDeviceExtension->pRegisters->Glint;

    pul = (PULONG)hwDeviceExtension->aulInitializationTable;
    cul = hwDeviceExtension->culTableEntries;

    while(cul--) {
   
        ulRegAddr = *pul++;
        ulRegData = *pul++;

        pulReg = (PULONG)(ulRegAddr & 0x3FFFFF);    // offset from base address
        BaseAddrSelect = ulRegAddr >> 29;

        if(BaseAddrSelect == 0) {
       
            //
            // the offset is from the start of the control registers
            //

            pulReg = (PULONG)((ULONG_PTR)pCtrlRegs + (ULONG_PTR)pulReg);

        } else {
       
            DISPDBG((
                "ProcessInitializationTable: Invalid base address select %d regAddr = %d regData = %d\n", 
                BaseAddrSelect, ulRegAddr, ulRegData));
            
            continue;
        }

        DISPDBG((
            "ProcessInitializationTable: initialising (region %d) register %08.8Xh with %08.8Xh\n", 
            BaseAddrSelect, pulReg, ulRegData));
        
        VideoPortWriteRegisterUlong(pulReg, ulRegData);
    }
}


