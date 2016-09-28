/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: devctrl.c
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
#include "devctrl.h"
#include "p2vmicom.h"
#include "p2vidreg.h"

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

// All DIOC calls come through here
DWORD _stdcall 
VXD3DL_DeviceIOControl(DWORD    dwService, 
                       DWORD    dwDDB, 
                       DWORD    hDevice, 
                       LPDIOC   lpDIOCParms)
{
    DWORD dwRetVal = 0;
    DWORD dwRequest = lpDIOCParms->dwIoControlCode;

    DISPDBG(("GLINTMVD: ** In VXD3DL_DeviceIOControl"));
    DISPDBG(("GLINTMVD:   IoControl Code = 0x%x, hDevice = 0x%x", dwRequest, hDevice));

    if (dwRequest == DIOC_OPEN)
    {
	DISPDBG(("GLINTMVD: Open Device"));
	dwRetVal = 0;
    }
    else if (dwRequest == DIOC_CLOSEHANDLE )
    {
        DISPDBG(("GLINTMVD: Closed DEVIOCTL handle."));
        dwRetVal = 0;
    }
    // !! Can't find this definition in new DX Header files
    else if (dwRequest == 0x10000002)
    {
	DDMINIVDDTABLE* pTable  = (DDMINIVDDTABLE*)lpDIOCParms->lpvOutBuffer;
	PDEVICETABLE pDev       = FindpDevFromDevNode(hDevice);
	DISPDBG(("GLINTMVD: Requested DDHAL Kernel Callbacks"));

	if (!pDev)
	{
	    DISPDBG(("GLINTMVD: Failed to get PDEV"));
	    dwRetVal = 0;
	    return NO_ERROR;
	}

	pTable->dwMiniVDDContext        = (DWORD)pDev;	// For multimonitor support.
	pTable->vddGetIRQInfo           = (MINIPROC)MiniVDD_GetIRQInfo;
	pTable->vddIsOurIRQ             = (MINIPROC)MiniVDD_IsOurIRQ;
	pTable->vddEnableIRQ            = (MINIPROC)MiniVDD_EnableIRQ;
	pTable->vddFlipVideoPort        = (MINIPROC)MiniVDD_FlipVideoPort;
	pTable->vddFlipOverlay          = (MINIPROC)MiniVDD_FlipOverlay;
	pTable->vddGetCurrentAutoflip   = (MINIPROC)MiniVDD_GetCurrentAutoflip;
	pTable->vddGetPreviousAutoflip  = (MINIPROC)MiniVDD_GetPreviousAutoflip;
	pTable->vddLock                 = (MINIPROC)MiniVDD_Lock;
	pTable->vddSkipNextField        = (MINIPROC)MiniVDD_SkipNextField;
	pTable->vddGetPolarity          = (MINIPROC)MiniVDD_GetFieldPolarity;
	pTable->vddSetState             = (MINIPROC)MiniVDD_SetState;

	*((DWORD*)lpDIOCParms->lpcbBytesReturned) = sizeof(DDMINIVDDTABLE);

	dwRetVal = 0;
    }
    else if (dwRequest > MAX_VXD3DL_API )
    {
        // Returning a positive value will cause the WIN32 DeviceIOControl
        // call to return FALSE, the error code can then be retrieved
        // via the WIN32 GetLastError
        DISPDBG(("GLINTMVD: Requested invalid VXD function! :0x%x", dwRequest));
        dwRetVal = ERROR_NOT_SUPPORTED;
    }
    else
    {
        // CALL requested service
        DISPDBG(("GLINTMVD: Calling Function :0x%x", dwRequest));
        dwRetVal = (VXD3DL_Proc[dwRequest-1])(dwDDB, hDevice, lpDIOCParms);
    }
    return dwRetVal;
}

BOOL 
DoVMIRequest(PDEVICETABLE pDev, 
             LPVMIREQUEST pInput, 
             LPVMIREQUEST pOutput)
{
    DWORD dwCommand = pInput->dwCommand;
    DISPDBG(("Doing VMI Request"));
    //switch (pI2CIn->dwOperation)
    WAIT_GLINT_FIFO(2);
    switch(pInput->dwRegister)
    {
	case P2_VSBPartialConfig:
	{
	    BOOL bEnableA = FALSE;
	    BOOL bEnableB = FALSE;
	    BOOL bWideA = FALSE;
	    BOOL bWideB = FALSE;
	    __PermediaVSConfiguration VSConfig;
	    Permedia2Reg_VSPartialConfigB VSPartialB;
	    
	    DWORD dwUnitMode = 0;

	    *((DWORD*)(&VSPartialB)) = dwCommand;
	    *((DWORD*)(&VSConfig)) = 0;

	    // Store the partial config info for the stream
	    pDev->pGLInfo->I2C_VSPartialConfigB = dwCommand;

	    // Fill in the Partial B Stream config.
	    VSConfig.DoubleEdgeB = VSPartialB.DoubleEdgeB;
	    VSConfig.ReverseDataB = VSPartialB.ReverseDataB;
	    VSConfig.ColorSpaceB = VSPartialB.ColorSpaceB;
	    VSConfig.InterlaceB	= VSPartialB.InterlaceB;
	    VSConfig.VActiveVBIB = VSPartialB.VActiveVBIB;
	    VSConfig.FieldEdgeB	= VSPartialB.FieldEdgeB;
	    VSConfig.FieldPolarityB	= VSPartialB.FieldPolarityB;
	    VSConfig.UseFieldB = VSPartialB.UseFieldB;
	    VSConfig.VActivePolarityB = VSPartialB.VActivePolarityB;
	    VSConfig.VRefPolarityB = VSPartialB.VRefPolarityB;
	    VSConfig.HRefPolarityB = VSPartialB.HRefPolarityB;

	    if (VSPartialB.EnableStreamB == 1)
	    {
		bEnableB = 1;
	    }
	    else
	    {
		bEnableB = 0;
	    }
	    
	    if (VSPartialB.WideOutput == 1)
	    {
		bWideB = 1;
	    }
	    else
	    {
		bWideB = 0;
	    }
	    
	    if (bWideB && bEnableB)
	    {
		VSConfig.UnitMode = 2;
	    }
	    else if (!bWideB && bEnableB)
	    {
		VSConfig.UnitMode = 3;
	    }
	    else
	    {
		VSConfig.UnitMode = 0;
	    }
	    LOAD_GLINT_CTRL_REG(VSConfiguration, *((DWORD*)(&VSConfig)) );
	    DISPDBG(("Writing 0x%x to VSConfig", *((DWORD*)(&VSConfig)) ));
        }
        break;
	case P2_VSBControl:
	    LOAD_GLINT_CTRL_REG(VSBControl, dwCommand);
	break;
	case P2_VSBInterruptLine:
	    LOAD_GLINT_CTRL_REG(VSBInterruptLine, dwCommand);
	break;
	case P2_VSBCurrentLine:	
	    LOAD_GLINT_CTRL_REG(VSBCurrentLine, dwCommand);
	break;
	case P2_VSBVideoAddressHost:
	    LOAD_GLINT_CTRL_REG(VSBVideoAddressHost, dwCommand);
	break;
	case P2_VSBVideoAddressIndex:
	    LOAD_GLINT_CTRL_REG(VSBVideoAddressIndex, dwCommand);
	break;
	case P2_VSBVideoAddress0:	
	    LOAD_GLINT_CTRL_REG(VSBVideoAddress0, dwCommand);
	break;
	case P2_VSBVideoAddress1:
	    LOAD_GLINT_CTRL_REG(VSBVideoAddress1, dwCommand);
	break;
	case P2_VSBVideoAddress2:		
	    LOAD_GLINT_CTRL_REG(VSBVideoAddress2, dwCommand);
	break;
	case P2_VSBVideoStride:	
	    LOAD_GLINT_CTRL_REG(VSBVideoStride, dwCommand);
	break;
	case P2_VSBVideoStartLine:
	    LOAD_GLINT_CTRL_REG(VSBVideoStartLine, dwCommand);
	break;
	case P2_VSBVideoEndLine:		
	    LOAD_GLINT_CTRL_REG(VSBVideoEndLine, dwCommand);
	break;
	case P2_VSBVideoStartData:
	    LOAD_GLINT_CTRL_REG(VSBVideoStartData, dwCommand);
	break;
	case P2_VSBVideoEndData:
	    LOAD_GLINT_CTRL_REG(VSBVideoEndData, dwCommand);
	break;
	case P2_VSBVBIAddressHost:
	    LOAD_GLINT_CTRL_REG(VSBVBIAddressHost, dwCommand);
	break;
	case P2_VSBVBIAddressIndex:
	    LOAD_GLINT_CTRL_REG(VSBVBIAddressIndex, dwCommand);
	break;
	case P2_VSBVBIAddress0:		
	    LOAD_GLINT_CTRL_REG(VSBVBIAddress0, dwCommand);
	break;
	case P2_VSBVBIAddress1:			
	    LOAD_GLINT_CTRL_REG(VSBVBIAddress1, dwCommand);
	break;
	case P2_VSBVBIAddress2:			
	    LOAD_GLINT_CTRL_REG(VSBVBIAddress2, dwCommand);
	break;
	case P2_VSBVBIStride:			
	    LOAD_GLINT_CTRL_REG(VSBVBIStride, dwCommand);
	break;
	case P2_VSBVBIStartLine:
	    LOAD_GLINT_CTRL_REG(VSBVBIStartLine, dwCommand);
	break;
	case P2_VSBVBIEndLine:		
	    LOAD_GLINT_CTRL_REG(VSBVBIEndLine, dwCommand);
	break;
	case P2_VSBVBIStartData:
	    LOAD_GLINT_CTRL_REG(VSBVBIStartData, dwCommand);
	break;
	case P2_VSBVBIEndData:
	    LOAD_GLINT_CTRL_REG(VSBVBIEndData, dwCommand);
	break;
	default:
	break;
    }
    return TRUE;
}

	

// Simple function to handle an I2C Operation.  Can be called via RegisterDisplaydriver or via a DIOCTL
BOOL 
DoI2C(PDEVICETABLE pDev, 
      LPI2CREQUEST pI2CIn, 
      LPI2CREQUEST pI2COut)
{
    // If the request is a read, then we pass the pI2COut data buffer to the read function
    // If the request is a write, then we pass the pI2CIn data buffer to the write function
    // If the request is a reset, then we do a reset on the bus, and wait for it to settle down
    // If the request is a check for a device, we do a START, ADDRESS, ACK, STOP to talk to it.
    switch (pI2CIn->dwOperation)
    {
	case GLINT_I2C_READ:
	    DISPDBG(("GLINTMVD: Reading %d items from Slave: 0x%x", pI2CIn->NumItems, pI2CIn->wSlaveAddress));
	    I2CReceiveSeq(pDev, pI2CIn->wSlaveAddress, pI2CIn->Data[0], pI2CIn->NumItems, &pI2COut->Data[0]);
	break;

	case GLINT_I2C_WRITE:
	    DISPDBG(("GLINTMVD: Writing %d items to Slave: 0x%x", pI2CIn->NumItems, pI2CIn->wSlaveAddress));
	    I2CSendSeq(pDev, pI2CIn->wSlaveAddress, pI2CIn->NumItems, &pI2CIn->Data[0]);
	break;

	case GLINT_I2C_RESET:
	    DISPDBG(("GLINTMVD: Resetting I2C bus"));
	    I2CInitBus(pDev);
	break;

	case GLINT_I2C_DEVICE_PRESENT:
	    DISPDBG(("GLINTMVD: Requesting Device 0x%x to respond",  pI2CIn->wSlaveAddress));
	    I2CGetDevice(pDev, pI2CIn->wSlaveAddress, &pI2COut->Data[0]);
	break;

	default:
	    DISPDBG(("ERROR: Unknown I2C Operation!"));
	return FALSE;
    }
    return TRUE;
}

// Do something on the I2C Bus... Calling an IOCTL from the DX HAL will probably be OK,
// but doing it from an external 32 bit app isn't (the hDevice doesn't map to a pDev)
// The way to do I2C from an external application is to use an escape to the display driver
// which will then call RegisterDisplayDriver and eventually hit the function above.
DWORD _stdcall 
VXD3DL_I2C(DWORD dwDDB, 
           DWORD hDevice, 
           LPDIOC lpDIOCParms)
{
    PDEVICETABLE    pDev        = FindpDevFromDevNode(hDevice);
    DWORD           dwRetVal    = 0;
    LPI2CREQUEST    pI2CIn      = (LPI2CREQUEST)lpDIOCParms->lpvInBuffer;
    LPI2CREQUEST    pI2COut     = (LPI2CREQUEST)lpDIOCParms->lpvOutBuffer;

    if (!pDev)
    {
	DISPDBG(("GLINTMVD: Failed to get PDEV in I2C control: hDevice=0x%x", hDevice));
	dwRetVal = 0;
	return NO_ERROR;
    }
    DoI2C(pDev, pI2CIn, pI2COut);

    DISPDBG(("GLINTMVD: I2C operation Succesfull!"));
    return (NO_ERROR);
}

// Return the version
DWORD _stdcall 
VXD3DL_GetVersion(DWORD dwDDB, 
                  DWORD hDevice, 
                  LPDIOC lpDIOCParms)
{
    PDWORD pdw;
    HVM hSysVM;

    DISPDBG(("GLINTMVD: ** In VXD3DL_GetVersion"));

    // Stuff our version into the out params.
    pdw = (PDWORD)lpDIOCParms->lpvOutBuffer;
    hSysVM = Get_Sys_VM_Handle();
    pdw[0] = hSysVM;
    pdw[1] = Get_Execution_Focus();

    return(NO_ERROR);
}


// Allocate some memory for a new block of contexts.
DWORD _stdcall 
VXD3DL_16To32Pointer(DWORD dwDDB, 
                     DWORD hDevice, 
                     LPDIOC lpDIOCParms)
{
    DWORD       *Pointer16In;
    DWORD       *Pointer32Out;
    DWORD       Selector;
    DWORD       Offset;
    DWORD       HWord;
    DWORD       LWord;
    DWORD       Ptr32;
    DWORD       WinVM;

    DISPDBG(("GLINTMVD: ** In VXD3DL_16To32Pointer"));
    Pointer16In = (DWORD*)lpDIOCParms->lpvInBuffer;
    Pointer32Out = (DWORD*)lpDIOCParms->lpvOutBuffer;
    WinVM = (DWORD)Get_Sys_VM_Handle();
    Selector = ((*Pointer16In) >> 16);
    Offset = ((*Pointer16In) & 0xFFFF);

    DISPDBG(("GLINTMVD:  Sel: 0x%x, Offset: 0x%x", Selector, Offset));
    // Get the input selector
    __asm 
    {
	pushad
	push    0
	push    WinVM
	push    Selector
	VxDCall(_GetDescriptor)
	add     esp, 0xc
	mov     HWord, edx
	mov     LWord, eax
	popad
    }

    // Work out the base and the limit of the selector
    Ptr32 = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
    Ptr32 += Offset;

    DISPDBG(("GLINTMVD:  Ptr32: 0x%x", Ptr32));
    *Pointer32Out = Ptr32;
    return 0;
}

// Allocate or free a chunk of memory and pass back 16 and 32 bit pointers.
DWORD _stdcall 
VXD3DL_MemoryRequest(DWORD dwDDB, 
                     DWORD hDevice, 
                     LPDIOC lpDIOCParms)
{
    LPALLOCREQUEST lpMemReq;
    LPALLOCREQUEST  lpReturn;
    DWORD dw32BitPointer;
    WORD w16BitSelector;
    lpMemReq = (LPALLOCREQUEST)lpDIOCParms->lpvInBuffer;
    lpReturn = (LPALLOCREQUEST)lpDIOCParms->lpvOutBuffer;

    DISPDBG(("GLINTMVD: ** In VXD3DL_MemoryRequest"));
    if (lpMemReq->dwSize != sizeof(ALLOCREQUEST))
    {
	DISPDBG(("ERROR: Invalid size for ALLOCREQUEST"));
	return ERROR_INVALID_HANDLE;
    }

    if (lpMemReq->dwFlags == GLINT_MEMORY_ALLOCATE)
    {
        DWORD dwSize;
        dwSize = (lpMemReq->dwBytes + 4095) >> 12;

        DISPDBG(("  Memory Allocation Request: 0x%x Pages", dwSize));

        dw32BitPointer = (DWORD)_PageAllocate(dwSize, PG_SYS, 0, 0, 0, 0, 0, PAGEZEROINIT);
        if (dw32BitPointer)
        {
	    w16BitSelector= AllocateSelector((PVOID)dw32BitPointer, dwSize);
	    lpReturn->ptr32 = dw32BitPointer;
	    lpReturn->ptr16 = (((DWORD)w16BitSelector) << 16);
	    DISPDBG(("    Selector: 0x%x", w16BitSelector));
	    DISPDBG(("    Pointer32: 0x%x", dw32BitPointer));
	    return 0;
        }
        else
        {
	    return ERROR_NOT_ENOUGH_MEMORY;
        }

    }
    else if (lpMemReq->dwFlags == GLINT_MEMORY_FREE)
    {
	DISPDBG(("  Memory Free Request"));

	if (lpMemReq->ptr32 != 0)
	{
	    DISPDBG(("    Freeing ptr32: 0x%x, sel16: 0x%x", lpMemReq->ptr32, (lpMemReq->ptr16 >> 16)));

	     FreeSelector((WORD)(lpMemReq->ptr16 >> 16));
	    _PageFree((PVOID)lpMemReq->ptr32, 0);
	}
	else
	{
	    return ERROR_INVALID_HANDLE;
	}
	return 0;
    }
    return ERROR_INVALID_HANDLE;
}
