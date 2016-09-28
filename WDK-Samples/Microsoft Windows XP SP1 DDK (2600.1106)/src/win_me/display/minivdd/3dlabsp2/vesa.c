/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: vesa.c
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include <basedef.h>
#include "glint.h"
#include <vmm.h>
#include <pbt.h>
#include <dbt.h>
#include <pci.h>


typedef struct 
{
    DWORD   VbeSignature;
    WORD    VbeVersion;
    DWORD   OemStringPtr;
    DWORD   Capabilities;
    DWORD   VideoModePtr;
    DWORD   TotalMemory;
    WORD    OemSoftwareRev;
    DWORD   OemVendorNamePtr;
    DWORD   OemProductNamePtr;
    DWORD   OemProductRevPtr;
    BYTE    Reserved[222];
    BYTE    OemData[256];
}   VbeInfoBlock;

#define DPMS_TestPresence   0
#define DPMS_Set            1
#define DPMS_Get            2

// SetDPMS. Modes are:
// 00    On
// 01    Standby
// 02    Suspend
// 04    Off
#define DPMS_On			0
#define DPMS_Standby	1
#define DPMS_Suspend	2
#define DPMS_Off		4

// Prototype
DWORD Vesa_DDC(PDEVICETABLE pDev, CLIENT_STRUCT *Regs, DWORD VirtualMachine);


DWORD   
Vesa_DPMS(PDEVICETABLE pDev, 
          CLIENT_STRUCT *Regs)
{
    DWORD           Blank, ulValue;
    DWORD           HSync, VSync;
    pTVP3026RAMDAC  pTVP3026Regs = (pTVP3026RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);
    pRGB525RAMDAC   pRGB525Regs;
    
    if (pDev->ChipID == PERMEDIA2_ID || pDev->ChipID == TIPERMEDIA2_ID) 
    {   
        pRGB525Regs = (pRGB525RAMDAC)&(pDev->pRegisters->Glint.P2ExtVCReg); 
    } 
    else 
    {                                                                
        pRGB525Regs = (pRGB525RAMDAC)&(pDev->pRegisters->Glint.ExtVCReg);   
    }

    // First check to see if we are actually running accelerated.
    if (!pDev->pGLInfo->dwHiResMode) 
    {
        DISPDBG(("Vesa_DPMS call rejuected as we are in VGA mode."));
        return(0);
    }


    switch(Regs->CBRS.Client_BL) 
    {
        case    DPMS_TestPresence:
        Regs->CWRS.Client_AX = 0x004F;        // show vesa ok
        Regs->CWRS.Client_BX = 0x0710;       // all modes available
        return(1); // End of case    DPMS_TestPresence:

        case    DPMS_Set:
    	switch(Regs->CBRS.Client_BH) 
        {
            case DPMS_Standby:
            HSync = 0;      // No H
            VSync = 1;      // V Pulse
            Blank = 1;
            break;

            case DPMS_Suspend:
            HSync = 1;      // H Pulse
            VSync = 0;      // No V
            Blank = 1;
            break;

            case DPMS_Off:
            HSync = 0;      // No H
            VSync = 0;      // No V
            Blank = 1;
            break;

	    case DPMS_On:
            LOAD_GLINT_CTRL_REG(VideoControl, pDev->pGLInfo->dwVideoControl);
            Blank = 0;
            break;
        }

        if (Blank) 
        {
            LOAD_GLINT_CTRL_REG(VideoControl, 0x1 | ((HSync ? 1 : 2) << 3) | ((VSync ? 1 : 2) << 5));
        }
        Regs->CWRS.Client_AX = 0x4F;        // show vesa ok
        return(1); //End of  case    DPMS_Set:

        case    DPMS_Get:
        READ_GLINT_CTRL_REG(VideoControl, ulValue);
	switch ((ulValue >> 3) & 3) 
        {
	    case 1:
	    case 3:
	    HSync = 1;
	    break;

	    case 2:
	    case 0:
	    HSync = 0;
	}
	switch ((ulValue >> 5) & 3) 
        {
            case 1:
            case 3:
            VSync = 1;
            break;

            case 2:
            case 0:
            VSync = 0;
	}
        LOAD_GLINT_CTRL_REG(VideoControl, 0x1 | ((HSync ? 1 : 2) << 3) | ((VSync ? 1 : 2) << 5));

        if (HSync) 
        {
            if (VSync)
	        Regs->CBRS.Client_BH = DPMS_On;
            else
	        Regs->CBRS.Client_BH = DPMS_Suspend;
        }
        else 
        {
            if (VSync)
                Regs->CBRS.Client_BH = DPMS_Standby;
            else
                Regs->CBRS.Client_BH = DPMS_Off;
        }

        Regs->CWRS.Client_AX = 0x4F;        // show vesa ok
        return(1); //End of case    DPMS_Get:
    }
    Regs->CBRS.Client_AH=1;
    return(1);
}

DWORD   _cdecl  
VesaSupport(DWORD DevNode, 
            CLIENT_STRUCT *Regs)
{
    PDEVICETABLE pDev = FindpDevFromDevNode(DevNode);

    DISPDBG(("Vesa Support function:0x%x DevNode:0x%x", Regs->CWRS.Client_AX, DevNode));

    if (pDev == 0) 
    {
        DISPDBG(("Failing VesaSupport: Cant find pDev"));
        return(0);
    }

    switch (Regs->CWRS.Client_AX) 
    {
        case 0x4f10:
        // DPMS call
        return Vesa_DPMS(pDev, Regs);

        case 0x4f15:
        // VBE/DDC support
        return Vesa_DDC(pDev, Regs, WindowsVMHandle);
    }
    return(0);  // Fail call
}

BOOL __cdecl 
ShellBroadcastCallback(DWORD uMsg, 
                       DWORD wParam, 
                       DWORD lParam, 
                       PDEVICETABLE pDevTable)
{
    PDEVICETABLE            pDev;
    PDEV_BROADCAST_DEVNODE  pBroadcast = (PDEV_BROADCAST_DEVNODE) lParam;
    long		    i;

    DISPDBG(("ShellBroadcastCallback: uMsg %x, wParam %x, lParam %x, pDev %x", uMsg, wParam, lParam, pDevTable));

    switch (uMsg) 
    {
        case WM_POWERBROADCAST:
	switch (wParam) 
        {
            case PBT_APMSUSPEND:             
            case PBT_APMSTANDBY:
            DISPDBG(("PBT_APMSTANDBY/PBT_APMSUSPEND - Powering Down:"));
            for (i = 0; i < MAX_GLINT_DEVICES; i++) 
            {
	        pDev = &(DevTable[i]);

	        if (pDev->ChipID == NOCHIP_ID)
		    return(0);

	        if (pDev->pRegisters == 0)
		    continue;
	        pDev->pGLInfo->dwFlags |= GMVF_POWERDOWN;
		LOAD_GLINT_CTRL_REG(VideoControl, 0);
            }
            break;

            case PBT_APMRESUMESUSPEND:       
            case PBT_APMRESUMESTANDBY:
            DISPDBG(("PBT_APMRESUMESTANDBY/PBT_APMRESUMESUSPEND - Powering Up:"));
            for (i = 0; i < MAX_GLINT_DEVICES; i++) 
            {
	        pDev = &(DevTable[i]);

	        if (pDev->ChipID == NOCHIP_ID)
		        return(0);
	        if (pDev->pRegisters == 0)
		        continue;
	        pDev->pGLInfo->dwFlags &= ~GMVF_POWERDOWN;
		LOAD_GLINT_CTRL_REG(VideoControl, pDev->pGLInfo->dwVideoControl);
            }
            break;

	    case PBT_APMQUERYSUSPENDFAILED:  
	    case PBT_APMQUERYSTANDBYFAILED:  
	    case PBT_APMRESUMECRITICAL:      
	    case PBT_APMBATTERYLOW:          
	    case PBT_APMPOWERSTATUSCHANGE:
	    case PBT_APMOEMEVENT:
	    case PBT_APMQUERYSUSPEND:        
	    case PBT_APMQUERYSTANDBY:        
	    break;
	}
        break; //End of case WM_POWERBROADCAST:

        case WM_DEVICECHANGE:
	switch (wParam) 
        {
            case DBT_DEVICEARRIVAL:
            {
                pDev = FindpDevFromDevNodeTable(pBroadcast->dbcd_devnode);

                if (pDev == NULL) 
                {
	            DISPDBG(("WM_DEVICECHANGE WARNING: pDev %x is not a 3Dlabs device", pBroadcast->dbcd_devnode));
	            break;
                }
                // Check to see if this is a recognised 3dlabs device
                if (!pDev || (pDev->DevNode != pBroadcast->dbcd_devnode))
                    break;
            }
            break;

            case DBT_DEVICEQUERYREMOVE:
            // Disallow the removal of our delta chip or our graphics processor.
            {
                PDEV_BROADCAST_DEVNODE pBroadcast = (PDEV_BROADCAST_DEVNODE) lParam;

                pDev = FindpDevFromDevNodeTable(pBroadcast->dbcd_devnode);
                if (pDev == NULL) 
                {
	                DISPDBG(("WM_DEVICECHANGE WARNING: pDev %x is not a 3Dlabs device", pBroadcast->dbcd_devnode));
	                break;
                }

                // Check to see if this is a recognised 3dlabs device
                if (!pDev || (pDev->DevNode != pBroadcast->dbcd_devnode))
                break;
                return(0);
            }
            break;
        }
        break; //End of case WM_DEVICECHANGE:
    } //    switch (uMsg) 

    return(1);
}
