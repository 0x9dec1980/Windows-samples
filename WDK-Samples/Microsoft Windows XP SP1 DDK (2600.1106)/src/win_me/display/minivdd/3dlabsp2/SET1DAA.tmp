/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: hardware.c
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include <basedef.h>
#include <vmm.h>
#include <vxdwraps.h>
#include <configmg.h>
#include <vwin32.h>
#include <pci.h>
#include "glint.h"


/****************************************************************************
    Global Data
****************************************************************************/
#pragma CM_PAGEABLE_DATA


/****************************************************************************
    Functions start here
****************************************************************************/
#pragma CM_PAGEABLE_CODE

static DWORD prefix(char *pre, char *str)
{
    while (*pre) 
    {
	if (*pre != *str)
	    return 0;
	pre++;
	str++;
    }
    return(1);
}

LONG GetNumberWithPrefix(char *pre, char *str)
{
    LONG Number = 0, index;
    while (*str != 0) 
    {
        index = 0;
        while (pre[index] == str[index])
            index++;

        if (pre[index] == 0) 
        {
            // We matched the entire prefix in the string. Extract the following number
            while (((str[index] >= '0') && (str[index]<='9')) ||
                   ((str[index] >= 'A') && (str[index]<='F'))) 
            {
                if (str[index]<='9')
                    Number = (Number << 4) + (str[index]-'0');
                else
                    Number = (Number << 4) + (str[index]-'A'+10);
                index++;
            }
            return(Number);
        }
        str++;
    }
    return(0xffffffff);
}

DWORD GetBusDevFnNumbers(PCHAR DevNodeIDBuffer, PDWORD Bus, PDWORD Dev, PDWORD Fn)
{
    *Bus = GetNumberWithPrefix("BUS_", &(DevNodeIDBuffer[22]));
    *Dev = GetNumberWithPrefix("DEV_", &(DevNodeIDBuffer[22]));
    *Fn  = GetNumberWithPrefix("FUNC_", &(DevNodeIDBuffer[22]));
    return(1);
}



PDEVICETABLE CheckHardwareNode(DWORD DevNode, PDEVICETABLE pDevTable, DWORD *pnDevices)
{
    char    DevNodeIDBuffer[MAX_DEVICE_ID_LEN + 1];
    DWORD   index;

    if (CM_Get_Device_ID( DevNode, DevNodeIDBuffer, MAX_DEVICE_ID_LEN + 1, 0)) 
    {
	DISPDBG(("Getting ID of Child failed"));
	return NULL;
    }

    DISPDBG(("%s 0x%x", DevNodeIDBuffer, DevNode));

    for (index = 0; index < *pnDevices; index++) 
    {
        if (pDevTable[index].DevNode == DevNode) 
        {
            DISPDBG(("Already enumerated device"));
            return NULL;
        }
    }

    // Is this a Glint Device?
    if ((prefix("PCI\\VEN_3D3D", DevNodeIDBuffer)) ||
	(prefix("PCI\\VEN_104C", DevNodeIDBuffer))) 
    {
        PDEVICETABLE	pDev;
        ULONG		ulValue;
        ULONG           Bus, Dev, Fn;

        pDev                = &(pDevTable[*pnDevices]);
	// It is a Glint Device. Probe the PCI space for more information
	pDev->DevNode 	    = DevNode;
	pDev->ActualDevNode = DevNode;
	pDev->dwFlags 	    = 0;
        pDev->IRQHandle     = 0;
        pDev->pDeltaDev     = 0;
        pDev->pGLInfo       = 0;
#ifdef POWER_STATE_MANAGE
        pDev->dwPowerState  = CM_POWERSTATE_D0; // IGX 112399 normm
#endif //#ifdef POWER_STATE_MANAGE

        GetBusDevFnNumbers(DevNodeIDBuffer, &Bus, &Dev, &Fn);

        pDev->PCIBus        = (WORD) Bus;
        pDev->PCIDevice     = (WORD) Dev;
        pDev->PCIFunction   = (WORD) Fn;

        DISPDBG(("PCI Bus 0x%x, Slot 0x%x, Function 0x%x", pDev->PCIBus, pDev->PCIDevice, pDev->PCIFunction));
	DISPDBG(("Glint Device Found. Checking Log Conf"));
	CM_Call_Enumerator_Function(DevNode,
                                    PCI_ENUM_FUNC_GET_DEVICE_INFO,
				    0, 
                                    &(pDev->PCIConfig), 
                                    sizeof(PCI_COMMON_CONFIG), 
                                    0);
	// Yet another double check
	if ((pDev->PCIConfig.VendorID == 0x3d3d) || (pDev->PCIConfig.VendorID == 0x104c)) 
        {
            DWORD nToGo = 0;

            DISPDBG(("SubSystemVendorID = 0x%04x, SubSystemID = 0x%04x",
            pDev->PCIConfig.u.type0.SubSystemVendorID, pDev->PCIConfig.u.type0.SubSystemID));

            pDev->ChipID            = pDev->PCIConfig.DeviceID;
            pDev->RevisionID        = pDev->PCIConfig.RevisionID | pDev->ChipID << 16;
            pDev->FamilyID          = PERMEDIA_ID;

            // Get CM Config information
            CM_Get_Alloc_Log_Conf(  &(pDev->CMConfig), 
                                    DevNode, 
                                    0);
            if (pDev->CMConfig.wNumMemWindows == 0) 
            {
                DISPDBG(("Normal configuration not available, resorting to boot config"));
                CM_Get_Alloc_Log_Conf(  &(pDev->CMConfig), 
                                        DevNode, 
                                        1);
            }
            if (!pDev->PCIConfig.Command & PCI_ENABLE_MEMORY_SPACE) 
            {
                DISPDBG(("Physically enabling device in PCI header"));
                pDev->PCIConfig.Command = PCI_ENABLE_MEMORY_SPACE | PCI_ENABLE_BUS_MASTER;
                CM_Call_Enumerator_Function(DevNode, 
                                            PCI_ENUM_FUNC_SET_DEVICE_INFO,
			                    0x4, 
                                            &(pDev->PCIConfig.Command), 
                                            0x2, 
                                            0);
            }

            if (pDev->CMConfig.wNumMemWindows == 0) 
            {
                // Hope we dont get here with a Triton Chipset, as this crashes here
                DISPDBG(("No W95 configuration available, probe PCI for regions"));
                DISPDBG(("NOTE: Some PCI Chipsets will cause delta boards to crash if they"));
                DISPDBG(("      write to the PCI space using bytes rather than words."));

                // We may have a resource clash, so the config manager has zeroed
                // all our regions. However, the PCI config contains the regions
                // we should have been allocated.
                // We need to validate these regions, just in case, but for now,
                // just use them
                ulValue = 0;
                while (ulValue < 6) 
                {
                    pDev->CMConfig.dMemBase[ulValue] = pDev->PCIConfig.u.type0.BaseAddresses[ulValue];
                    pDev->PCIConfig.u.type0.BaseAddresses[ulValue] = 0xffffffff;
                    ulValue++;
                }
                pDev->CMConfig.wNumMemWindows = (WORD) ulValue;

                // Write the config space back to the chip with all bits set in
                // each region. When we read them back, the bits that are still
                // set will allow us to calculate the region size
                pDev->PCIConfig.Command &= ~PCI_ENABLE_MEMORY_SPACE;
                CM_Call_Enumerator_Function(DevNode, 
                                            PCI_ENUM_FUNC_SET_DEVICE_INFO,
                                            0x4, 
                                            &(pDev->PCIConfig.Command), 
                                            0x2, 
                                            0);
                CM_Call_Enumerator_Function(DevNode, 
                                            PCI_ENUM_FUNC_SET_DEVICE_INFO,
                                            0x10, 
                                            &(pDev->PCIConfig.u.type0.BaseAddresses[0]), 
                                            0x18, 
                                            0);
                CM_Call_Enumerator_Function(DevNode, 
                                            PCI_ENUM_FUNC_GET_DEVICE_INFO,
                                            0x10, 
                                            &(pDev->PCIConfig.u.type0.BaseAddresses[0]), 
                                            0x18, 
                                            0);

                // Now probe the results and restore our config region
                for (ulValue = 0; ulValue < pDev->CMConfig.wNumMemWindows; ulValue++) 
                {
                    pDev->CMConfig.dMemLength[ulValue] = 1 + ~pDev->PCIConfig.u.type0.BaseAddresses[ulValue];
                    pDev->PCIConfig.u.type0.BaseAddresses[ulValue] = pDev->CMConfig.dMemBase[ulValue];						
                    DISPDBG(("Regions %d from PCI: Addr 0x%x Len 0x%x", ulValue,
                    pDev->CMConfig.dMemBase[ulValue], pDev->CMConfig.dMemLength[ulValue])); 
                }

                // Reset our base addresses to what they should be
                pDev->PCIConfig.Command |= PCI_ENABLE_MEMORY_SPACE;
                CM_Call_Enumerator_Function(DevNode, 
                                            PCI_ENUM_FUNC_SET_DEVICE_INFO,
                                            0x10, 
                                            &(pDev->PCIConfig.u.type0.BaseAddresses[0]), 
                                            0x18, 
                                            0);
                CM_Call_Enumerator_Function(DevNode, 
                                            PCI_ENUM_FUNC_SET_DEVICE_INFO,
                                            0x4, 
                                            &(pDev->PCIConfig.Command), 
                                            0x2, 
                                            0);
            }

            // remove all the VGA regions from the start of the region list.
            // These should be the first 2 or 3 regions, which have a length <= 0x10000.
            // All real glint/permedia regions will be longer.
            nToGo = 0;
            while ( (pDev->CMConfig.dMemLength[nToGo] <= 0x10000) || 
                    (pDev->CMConfig.dMemBase[nToGo] == 0)) 
            {
                if (++nToGo >= pDev->CMConfig.wNumMemWindows)
                    break;
            }

	    if (pDev->CMConfig.wNumMemWindows > nToGo)
		pDev->CMConfig.wNumMemWindows -= (WORD) nToGo;
	    else
		pDev->CMConfig.wNumMemWindows = 0;		    
            // We have been given VGA resources. Remove them from our lists.
	    ulValue = nToGo;
            while (ulValue < MAX_MEM_REGISTERS) 
            {
                pDev->CMConfig.dMemBase[ulValue - nToGo]   = pDev->CMConfig.dMemBase[ulValue];
                pDev->CMConfig.dMemLength[ulValue - nToGo] = pDev->CMConfig.dMemLength[ulValue];
                pDev->CMConfig.wMemAttrib[ulValue - nToGo] = pDev->CMConfig.wMemAttrib[ulValue];
                ++ulValue;
            }
            // All our devices have at least 1 window (registers)
            if (pDev->CMConfig.wNumMemWindows < 1) 
            {
                DISPDBG(("wNumMemWindows < 1 - Not a usable 3Dlabs chip"));
                memset(pDev, 0, sizeof(DEVICETABLE));
            }
            else 
            {
                // The chip is one of ours!
                (*pnDevices)++;
                return(pDev);
            }
	}
	else 
        {
	    DISPDBG(( "GlintDevice has invalid PCI header"));
	}
    }
    return(NULL);
}

DWORD 
LocateHardwareBelowNode(DWORD           DevNode, 
                        PDEVICETABLE    pDevTable, 
                        DWORD           *pnDevices)
{
    DEVNODE Child;

    if (CM_Get_Child(&Child, DevNode, 0)) 
    {
	return (0);
    }

    do 
    {
        CheckHardwareNode(Child, pDevTable, pnDevices);
        // Look for more devices beneath this one
        LocateHardwareBelowNode(Child, pDevTable, pnDevices);
        // Get the devices sibling and repeat the operation.
    }	while (CM_Get_Sibling( &Child, Child, 0 ) == 0);
    return 0;
}


DWORD 
LocateHardwareNode(PDEVICETABLE pDevTable)
{
    DEVNODE	RootDevNode = 0;
    DWORD	nDevices;
    ULONG       i, j;

    DISPDBG(( "Locating Hardware Nodes"));
    if (CM_Locate_DevNode(  &RootDevNode, 
                            NULL, 
                            0)) 
    {
	DISPDBG(( "Locate Root Device Node failed"));
	return (0);
    }
    // Before this is called, InitialiseHardwareNode will either have found a glint device
    // or not. Whatever the case, it will have initialised the Chip field in pDevTable[0]
    // Set up nDevices to avoid overwriting this first node.
    for(nDevices = 0; nDevices < MAX_GLINT_DEVICES; nDevices++) 
    {
        if (pDevTable[nDevices].ChipID == NOCHIP_ID)
            break;
    }

    LocateHardwareBelowNode(RootDevNode, pDevTable, &nDevices);

    DISPDBG(( "Glint Device Table:"));
    // Check each glint device for a matching delta device
    for (i = 0; i < MAX_GLINT_DEVICES; i++) 
    {

        if (pDevTable[i].ChipID == DELTA_ID) 
        {
            pDevTable[i].pDeltaDev = 0;// Dont do a thing if we are on a delta
        }
        else 
        {   // If we have no delta, the pDeltaDev needs to point back at the Glint.
            pDevTable[i].pDeltaDev = &(pDevTable[i]);
            // Search for any delta devices on the same bus and slot as the Glint
            for (j = 0; j < MAX_GLINT_DEVICES; j++)
            {
                if (pDevTable[j].ChipID == DELTA_ID) 
                {   // We have found a delta chip. Check to see if this is the right one
                    if ((pDevTable[i].PCIBus == pDevTable[j].PCIBus) &&
                        (pDevTable[i].PCIDevice == pDevTable[j].PCIDevice)) 
                    {
                        pDevTable[i].pDeltaDev = &(pDevTable[j]);
                        pDevTable[i].dwFlags |= GMVF_DELTA;
                        break;
                    }
                }
            }
        }
        DISPCHIP((&pDevTable[i]));
    }
    return (nDevices);	
}

PDEVICETABLE 
InitialiseHardwareNode(PDEVICETABLE pDevTable, 
                       ULONG        DevNode)
{
    DWORD	    nDevices = 0;
    PDEVICETABLE    pDev;

    // Before this is called, InitialiseHardwareNode will either have found a glint device
    // or not. Whatever the case, it will have initialised the Chip field in pDevTable[0]
    // Set up nDevices to avoid overwriting this first node.
    for(nDevices = 0; nDevices < MAX_GLINT_DEVICES; nDevices++) 
    {
        if (pDevTable[nDevices].ChipID == NOCHIP_ID)
            break;
    }

    pDev = CheckHardwareNode(DevNode, pDevTable, &nDevices);

    if (pDev) 
    {
        DISPDBG(( "Glint Device Found:"));
        DISPCHIP((pDev));
        return(pDev);
    }
    return(NULL);
}

// This routine needs to leave the chip in a memory and vga enabled state
DWORD _cdecl 
TurnVGAOn(DWORD         DevNode, 
          CLIENT_STRUCT *Regs)
{
    PDEVICETABLE pDev = FindpDevFromDevNodeTable(DevNode);

    DISPDBG(("TurnVGAOn DevNode 0x%x, EDX 0x%x", DevNode, Regs->CRS.Client_EDX));
    DISPCHIP((pDev));


    pDevWithVGAEnabled = pDev;

    // Enable io space and enable memory space
    pDev->PCIConfig.Command |= PCI_ENABLE_IO_SPACE;
    pDev->PCIConfig.Command |= PCI_ENABLE_MEMORY_SPACE;
    CM_Call_Enumerator_Function(DevNode, 
                                PCI_ENUM_FUNC_SET_DEVICE_INFO,
			        0x4, 
                                &(pDev->PCIConfig.Command), 
                                0x2, 
                                0);

    // Do the out which enables decoding of the VGA regions.
    __asm {
        push    eax
        push    edx
        mov     edx, 03cch
        in      al, dx
        or      al, 02h
        mov     edx, 03c2h
        out     dx, al
        pop     edx
        pop     eax
    }

    return(1);
}


// This routine needs to leave the chip in a memory enabled and VGA disabled state
DWORD _cdecl 
TurnVGAOff(DWORD            DevNode, 
           CLIENT_STRUCT    *Regs)
{
    PDEVICETABLE pDev = FindpDevFromDevNodeTable(DevNode);

    DISPDBG(("TurnVGAOff DevNode 0x%x, EDX 0x%x", DevNode, Regs->CRS.Client_EDX));
    DISPCHIP((pDev));


    pDevWithVGAEnabled = 0;

    // Enable io space and enable memory space temporarily to do the out
    pDev->PCIConfig.Command |= PCI_ENABLE_IO_SPACE;
    pDev->PCIConfig.Command |= PCI_ENABLE_MEMORY_SPACE;
    CM_Call_Enumerator_Function(DevNode, 
                                PCI_ENUM_FUNC_SET_DEVICE_INFO,
			        0x4, 
                                &(pDev->PCIConfig.Command), 
                                0x2, 
                                0);

    // Do the out which disables decoding of the VGA regions.
    __asm {
        push    eax
        push    edx
        mov     edx, 03cch
        in      al, dx
        and     al, 0fdh
        mov     edx, 03c2h
        out     dx, al
        pop     edx
        pop     eax
    }

    // Disable io space and enable memory space
    pDev->PCIConfig.Command &= ~(PCI_ENABLE_IO_SPACE);
    pDev->PCIConfig.Command |= PCI_ENABLE_MEMORY_SPACE;
    CM_Call_Enumerator_Function(DevNode, 
                                PCI_ENUM_FUNC_SET_DEVICE_INFO,
				0x4, 
                                &(pDev->PCIConfig.Command), 
                                0x2, 
                                0);
    return(1);
}
