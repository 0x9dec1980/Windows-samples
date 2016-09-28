/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

   ldioctl.c

Abstract:

   This is the VxD, LDVXD.VXD, that provides the interface to CDVSD.VXD 
   used by the LOCKCD.EXE drive lock demo.
   This module contains code to issue I/O requests to CDVSD using IOS.
   In particular, the application can issue an IOCTL to CDVSD or a command to read digital audio.
   These interfaces both go via a custom IOCTL format issued by LOCKCD.EXE to LDVXD.VXD.
   Note that the requests are all synchronous - the asynchronous IOCTL is not supported.
   These interfaces are necessary because the IOCTL interfaces to CDVSD.VXD 
   do not have a user mode equivalent.

Author:

        original Redbook audio CD demo code by Len Smale

Environment:

   Kernel mode only


Revision History:
    4/19/2000 Modifications to add use of drive locking mechanisms by 
	Kevin Burrows and Mark Amos. 
	Redbook-specific code stripped, tool now independent of disk format. 
	File and function names changed to reflect new purpose. Thanks, Len!

--*/
#define _BLDR_
#define USECMDWRAPPERS
#define AFTERPROTOTYPE  ;
#include <basedef.h>
#include <vmm.h>
#include <dbt.h>
#include <vxdwraps.h>
#include <debug.h>
#include <pci.h>
#include <winerror.h>
#include <vwin32.h>



#include <blockdev.h>
#include <aep.h>
#include <sgd.h>
//#include <bpb.h>
#include <drp.h>
#include <ilb.h>
#include <ior.h>
#include <iop.h>
#include <irs.h>
#include <isp.h>
#include <med.h>
#include <ida.h>
#include <vrp.h>
#include <ios.h>
#include <dcb.h>
#include <ddb.h>
#include <vpicd.h>
#include <cdioctl.h>
#include "../inc/ldif.h"

typedef DWORD IlbCriteria(PIOR);
typedef IlbCriteria *pIlbCriteria;

typedef DWORD IlbInternalCriteria(pIOP);
typedef IlbInternalCriteria *pIlbInternalCriteria;

#define STRUCOFFSET(pstruc, elem) ((ULONG)&((pstruc)NULL)->elem)
#define MAX_DRIVE_NUMBER 25

extern ILB LDVXD_ilb;

DWORD gfnSendLockCommand[MAX_DRIVE_NUMBER+1] = {0};

DWORD __stdcall LD_IoctlProxy (PDIOCPARAMETERS pDIOCParams);
DWORD __stdcall LD_ReadDA (PDIOCPARAMETERS pDIOCParams);
DWORD __stdcall LD_TrackInfo (PDIOCPARAMETERS pDIOCParams);
PDCB __stdcall LD_GetDCB (DWORD dwVolume);
pIOP __stdcall LD_GetIOP (PDCB pDcb);
void __stdcall LD_ReturnIOP(PDCB pDcb, pIOP pIop);
void __stdcall LD_SendRequest(PDCB pDcb, pIOP pIop, DWORD dwVolume);
DWORD __stdcall LD_LockDrive(DWORD dwVolume);
void  __stdcall LD_RefreshLock(DWORD gfnSendLockCommand);
DWORD __stdcall LD_UnlockDrive(DWORD gfnSendLockCommand);
DWORD __stdcall LD_IsDriveLocked(DWORD dwVolume);
DWORD __stdcall LD_MapIORResult(DWORD IorResult);
DWORD __stdcall LD_ProcessLockRequest(DWORD dwRequest, PDIOCPARAMETERS pDIOCParams);


DWORD __stdcall LD_IoctlProxy (PDIOCPARAMETERS pDIOCParams)
{
    PCDROM_ISSUE_IOCTL  pIoctlProxy = (PCDROM_ISSUE_IOCTL)(pDIOCParams->lpvInBuffer);
    DWORD           Status = NO_ERROR;
    ULONG           gPageAddress;
    ULONG           nPageOffset;
    ULONG           nPages;
    PDCB            pDcb = NULL;
    pIOP            pIop = NULL;

    // Validate IOCTL request.
    if ((!pIoctlProxy) ||
        (pDIOCParams->cbInBuffer < sizeof(CDROM_ISSUE_IOCTL)) ||
        (!pDIOCParams->lpvOutBuffer)) {
        Status = ERROR_INVALID_PARAMETER;
    }

    // Validate the device number and get the DCB
    if (Status == NO_ERROR) {
        pDcb = LD_GetDCB(pIoctlProxy->Volume);
        if (!pDcb) {
            Status = ERROR_BAD_UNIT;
        }
    }
    
    // Obtain an IOPX/IOP/IOR.
    if (Status == NO_ERROR) {
        pIop = LD_GetIOP(pDcb);  
        if (!pIop) {
            // Unable to allocate IOPX.
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // Map output buffer into system space and lock.
    if (Status == NO_ERROR) {
        nPageOffset = pDIOCParams->lpvOutBuffer & PAGEMASK;
        nPages = NPAGES(nPageOffset+pDIOCParams->cbOutBuffer); // NPAGES assumes cb is at start of page
        gPageAddress = _LinPageLock(PAGE(pDIOCParams->lpvOutBuffer), 
                                    nPages, 
                                    PAGEMARKDIRTY | PAGEMAPGLOBAL);
        if (!gPageAddress) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // Fill in final values and issue request.
    if (Status == NO_ERROR) {
        pIop->IOP_ior.IOR_func = IOR_GEN_IOCTL;
        pIop->IOP_ior.IOR_ioctl_function = pIoctlProxy->IoctlCode;
        pIop->IOP_ior.IOR_ioctl_buffer_ptr = gPageAddress+nPageOffset;

        // Make sure the I/O criteria are met.
        pIop->IOP_original_dcb = (ULONG)pDcb;
        // Following crashes or fails sometimes because the VRP is not set up,
        // so use internal form.
        // if ((*(pIlbCriteria)LDVXD_ilb.ILB_io_criteria_rtn)(&pIop->IOP_ior) != 0) {
        if ((*(pIlbInternalCriteria)LDVXD_ilb.ILB_int_io_criteria_rtn)(pIop) != 0) {
            pIop->IOP_ior.IOR_flags |= IORF_DOUBLE_BUFFER;
        }

        // Issue the request and map the result back to a user error.
        LD_SendRequest(pDcb, pIop, gfnSendLockCommand[pIoctlProxy->Volume]);
        Status = pIop->IOP_ior.IOR_ioctl_return;
            
        // Unlock the output buffer.
        _LinPageUnLock(PAGE(gPageAddress), nPages, PAGEMAPGLOBAL);
    }
        
    // Free up the IOPX/IOP/IOR, if allocated
    if (pIop) {
        LD_ReturnIOP(pDcb, pIop);
    }
    
    // Return result to requestor.
    // Fill in return parameter for error cases because apps can be really
    // stupid about checking errors and fault on undefined returned values.

    if (pDIOCParams->lpcbBytesReturned) {
        if (Status == NO_ERROR) {
            *(PDWORD)(pDIOCParams->lpcbBytesReturned) = pDIOCParams->cbOutBuffer;
        }
        else {
            *(PDWORD)(pDIOCParams->lpcbBytesReturned) = 0;
        }
    }
    return Status;
}

DWORD __stdcall LD_TrackInfo (PDIOCPARAMETERS pDIOCParams)
{
    PCDROM_TRACKINFO pTrackInfo = (PCDROM_TRACKINFO)pDIOCParams->lpvOutBuffer;

}

DWORD __stdcall LD_ReadDA (PDIOCPARAMETERS pDIOCParams)
{
    PCDROM_READ_DA  pReadDA = (PCDROM_READ_DA)(pDIOCParams->lpvInBuffer);
    PDCB            pDcb = NULL;
    pIOP            pIop = NULL;
    ULONG           gPageAddress;
    ULONG           nPageOffset;
    ULONG           nPages;
    DWORD           Status = NO_ERROR;

    // bugbug: support overlapped  request.

    // Validate IOCTL request.
    if ((!pReadDA) ||
        (pDIOCParams->cbInBuffer < sizeof(CDROM_READ_DA)) ||
        (!pDIOCParams->lpvOutBuffer) ||
        (pDIOCParams->cbOutBuffer != DA_SIZE)) {
        Status = ERROR_INVALID_PARAMETER;
    }

    // Validate the device number and get the DCB
    // bugbug: validate that the device is CDVSD.
    if (Status == NO_ERROR) {
        pDcb = LD_GetDCB(pReadDA->Volume);
        if (!pDcb) {
            Status = ERROR_BAD_UNIT;
        }
    }
    
    // Obtain an IOPX/IOP/IOR.
    if (Status == NO_ERROR) {
        pIop = LD_GetIOP(pDcb);             
        if (!pIop) {
            // Unable to allocate IOPX.
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // Map output buffer into system space and lock.
    if (Status == NO_ERROR) {
        nPageOffset = pDIOCParams->lpvOutBuffer & PAGEMASK;
        nPages = NPAGES(nPageOffset+DA_SIZE); // NPAGES assumes cb is at start of page
        gPageAddress = _LinPageLock(PAGE(pDIOCParams->lpvOutBuffer), 
                                    nPages, 
                                    PAGEMARKDIRTY | PAGEMAPGLOBAL);
        if (!gPageAddress) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // Fill in final values and issue request.
    if (Status == NO_ERROR) {
        pIop->IOP_ior.IOR_func = IOR_READ;
        pIop->IOP_ior.IOR_start_addr[0] = pReadDA->Sector;
        pIop->IOP_ior.IOR_buffer_ptr = gPageAddress+nPageOffset;
        pIop->IOP_ior.IOR_xfer_count = DA_SIZE;

        // Make sure the I/O criteria are met.
        pIop->IOP_original_dcb = (ULONG)pDcb;
        if ((*(pIlbCriteria)LDVXD_ilb.ILB_io_criteria_rtn)(&pIop->IOP_ior) != 0) {
            pIop->IOP_ior.IOR_flags |= IORF_DOUBLE_BUFFER;
        }

        // Issue the request and map the result back to a user error.
        LD_SendRequest(pDcb, pIop, gfnSendLockCommand[pReadDA->Volume]);
        Status = LD_MapIORResult(pIop->IOP_ior.IOR_status);
            
        // Unlock the output buffer.
        _LinPageUnLock(PAGE(gPageAddress), nPages, PAGEMAPGLOBAL);
    }
        
    // Free up the IOPX/IOP/IOR, if allocated
    if (pIop) {
        LD_ReturnIOP(pDcb, pIop);
    }
    
    // Return result to requestor.
    // Fill in return parameter for error cases because apps can be really
    // stupid about checking errors and fault on undefined returned values.

    if (pDIOCParams->lpcbBytesReturned) {
        if (Status == NO_ERROR) {
            *(PDWORD)(pDIOCParams->lpcbBytesReturned) = DA_SIZE;
        }
        else {
            *(PDWORD)(pDIOCParams->lpcbBytesReturned) = 0;
        }
    }
    return Status;
}

DWORD __stdcall LD_ProcessLockRequest(DWORD dwRequest, PDIOCPARAMETERS pDIOCParams)
{
  PCDROM_LOCK_DRIVE pLockDrive = (PCDROM_LOCK_DRIVE)(pDIOCParams->lpvInBuffer);
  DWORD             Status = NO_ERROR;
  DWORD             dwLockedCommandFunction = 0;
  DWORD             dwDrive = 0;

  if( !pLockDrive )
  {
    Status = ERROR_INVALID_PARAMETER;
  } else {
    dwDrive = pLockDrive->Volume;
    switch( dwRequest )
    {
      case CDROM_IOCTL_LOCK_DRIVE:
        //
        // Lock the specified drive
        //
        dwLockedCommandFunction = LD_LockDrive(dwDrive);
    
        //
        // Store the address of the locked IOS_SendCommand function
        //
        gfnSendLockCommand[dwDrive] = dwLockedCommandFunction;
        if( pDIOCParams->lpvOutBuffer )
        {
          *(PDWORD)(pDIOCParams->lpvOutBuffer) = dwLockedCommandFunction;
          *(PDWORD)(pDIOCParams->lpcbBytesReturned) = sizeof(DWORD);
        }
        break;
    
      case CDROM_IOCTL_UNLOCK_DRIVE:
        if( gfnSendLockCommand[dwDrive] )
        {  
          DWORD dwResult = NO_ERROR;

          //
          // Unlock the specified drive
          //
          dwResult = LD_UnlockDrive(gfnSendLockCommand[dwDrive]);

          //
          // Clear the function address
          //
          gfnSendLockCommand[dwDrive] = 0;
          if( pDIOCParams->lpvOutBuffer )
          {
            *(PDWORD)(pDIOCParams->lpvOutBuffer) = dwResult;
            *(PDWORD)(pDIOCParams->lpcbBytesReturned) = sizeof(DWORD);
          }
        } else {
          Status = ERROR_INVALID_PARAMETER;
        }
        break;

      case CDROM_IOCTL_IS_DRIVE_LOCKED:
        if( gfnSendLockCommand[dwDrive] )
        {  
          DWORD dwResult = NO_ERROR;

          //
          // Query the lock status
          //
          dwResult = LD_IsDriveLocked(dwDrive);
          if( pDIOCParams->lpvOutBuffer )
          {
            *(PDWORD)(pDIOCParams->lpvOutBuffer) = dwResult;
            *(PDWORD)(pDIOCParams->lpcbBytesReturned) = sizeof(DWORD);
          }
        } else {
          Status = ERROR_INVALID_PARAMETER;
        }
        break;

      case CDROM_IOCTL_REFRESH_LOCK:
        //
        // Refresh the lock
        //
        if( gfnSendLockCommand[dwDrive] )
        {  
          LD_RefreshLock(gfnSendLockCommand[dwDrive]);
          if( pDIOCParams->lpvOutBuffer )
          {
            *(PDWORD)(pDIOCParams->lpvOutBuffer) = dwDrive;
            *(PDWORD)(pDIOCParams->lpcbBytesReturned) = sizeof(DWORD);
          }
        } else {
          Status = ERROR_INVALID_PARAMETER;
        }
        break;


      default:
        _asm int 3
        Status = ERROR_INVALID_PARAMETER;
        break;
    }//switch
  } 
  return Status;
}



PDCB __stdcall LD_GetDCB (DWORD dwVolume)
{
    PDCB        pDcb = NULL;
    ISP_dcb_get isp;

    isp.ISP_g_d_hdr.ISP_func = ISP_GET_DCB;
    isp.ISP_g_d_drive = dwVolume;
    LDVXD_ilb.ILB_service_rtn((ISP *)&isp);
    if (isp.ISP_g_d_hdr.ISP_result == 0) {
        pDcb = (PDCB)isp.ISP_g_d_dcb;
    }
    return pDcb;
}

pIOP __stdcall LD_GetIOP (PDCB pDcb)
{
    pIOP            pIop;
    ISP_IOP_alloc   isp;
    ULONG           ulSize;

    // allocate the IOPX
    ulSize = pDcb->DCB_cmn.DCB_expansion_length + STRUCOFFSET(pIOP, IOP_ior);
    isp.ISP_delta_to_ior = ulSize;
    ulSize += sizeof(IOR) + 2 * sizeof (SGD);
    isp.ISP_IOP_size = ulSize;
    isp.ISP_i_c_flags = 0;
    isp.ISP_i_c_hdr.ISP_func = ISP_CREATE_IOP;
    LDVXD_ilb.ILB_service_rtn((ISP *)&isp);
    pIop = (pIOP)isp.ISP_IOP_ptr;
    if (pIop) {
    
        // Allocated the IOPX.
        // Add the offset to the IOP, and fill in the default values.
        pIop = (pIOP)((ULONG)pIop + pDcb->DCB_cmn.DCB_expansion_length);
        pIop->IOP_ior.IOR_next = 0;
        pIop->IOP_ior.IOR_flags = IORF_CHAR_COMMAND |
                                  IORF_AUDIO_DATA_READ |
                                  IORF_SYNC_COMMAND |
                                  IORF_BYPASS_VOLTRK | 
                                  IORF_VERSION_002 | 
                                  IORF_BYPASS_A_B |
                                  IORF_INHIBIT_GEOM_RECOMPUTE;
        pIop->IOP_ior.IOR_req_vol_handle = pDcb->DCB_cmn.DCB_vrp_ptr;
        pIop->IOP_ior.IOR_start_addr[0] = 0;
        pIop->IOP_ior.IOR_start_addr[1] = 0;
        pIop->IOP_ior.IOR_xfer_count = 0;
        pIop->IOP_ior.IOR_num_sgds = 0;
        pIop->IOP_ior.IOR_sgd_lin_phys = (ULONG)(&pIop->IOP_ior) + sizeof(IOR);
        pIop->IOP_ior.IOR_vol_designtr = pDcb->DCB_cmn.DCB_unit_number;
        pIop->IOP_ior.IOR_buffer_ptr = 0;
    }
    return pIop;
}

void __stdcall LD_ReturnIOP(PDCB pDcb, pIOP pIop)
{
    ISP_mem_dealloc isp;

    // Go from pIOP to pIOPX and deallocate memory
    isp.ISP_mem_ptr_da = (ULONG)pIop - pDcb->DCB_cmn.DCB_expansion_length;
    isp.ISP_mem_da_hdr.ISP_func = ISP_DEALLOC_MEM;
    LDVXD_ilb.ILB_service_rtn((PISP)(&isp));
}

