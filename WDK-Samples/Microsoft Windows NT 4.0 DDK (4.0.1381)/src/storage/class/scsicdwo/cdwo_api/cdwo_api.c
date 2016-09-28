
/*++

Copyright (C) 1995-1996  Microsoft Corporation

Module Name:

    cdwo_api.c

Abstract:

    User-mode interface routines for scsicdwo driver for writing CDs.

Author:

    Tom McGuire (tommcg)

Environment:

    user mode only

--*/


#include <windows.h>
#include <winioctl.h>
#include <stdlib.h>
#include <stdio.h>

#include <scsicdwo.h>
#include <cdwo_api.h>


#ifndef SRB_STATUS_AUTOSENSE_VALID
#define SRB_STATUS_AUTOSENSE_VALID  0x80        // from srb.h
#endif


HANDLE
CdWriterOpen(
    IN ULONG WriterNumber
    )
    {
    WCHAR  WriterNameBuffer[ 32 ];
    HANDLE WriterHandle;

    swprintf( WriterNameBuffer, (LPCWSTR)L"\\\\.\\CdWriter%d", WriterNumber );

    WriterHandle = CreateFileW(
                       WriterNameBuffer,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_NO_BUFFERING,
                       NULL
                       );

    //
    //  If open fails, GetLastError() should already be set.  If the
    //  scsicdwo driver is not started, or if the requested device
    //  does not exist, GetLastError() will return ERROR_FILE_NOT_FOUND.
    //  If the requested device is already opened, GetLastError() will
    //  return ERROR_ACCESS_DENIED.
    //

    if ( WriterHandle == INVALID_HANDLE_VALUE )
         WriterHandle = NULL;

    return WriterHandle;
    }


CDWO_API_STATUS
CdWriterQueryParameters(
    IN  HANDLE               WriterHandle,
    OUT PCDWRITER_PARAMETERS WriterParams        // see scsicdwo.h
    )
    {
    BOOL  Success;
    ULONG Actual;

    ZeroMemory( WriterParams, sizeof( CDWRITER_PARAMETERS ));
    WriterParams->SizeOfCdWriterParameters = sizeof( CDWRITER_PARAMETERS );

    Success = DeviceIoControl(
                  WriterHandle,
                  IOCTL_CDWRITER_QUERY_PARAMETERS,
                  NULL,
                  0,
                  WriterParams,
                  sizeof( CDWRITER_PARAMETERS ),
                  &Actual,
                  NULL
                  );

    //
    //  Note that this command does not cause any i/o to the device.
    //

    return ( Success ? CDWO_API_STATUS_SUCCESS : CDWO_API_STATUS_FAILURE );
    }


CDWO_API_STATUS
CdWriterExecuteCommand(
    IN     HANDLE            WriterHandle,
    IN OUT PCDWRITER_CONTROL WriterControl          // see scsicdwo.h
    )
    {
    CDWO_API_STATUS Status;
    ULONG           Actual;
    BOOL            Success;

    WriterControl->SizeOfCdWriterControl = sizeof( CDWRITER_CONTROL );

    if ( WriterControl->TimeoutSeconds == 0 )
         WriterControl->TimeoutSeconds = CDWO_STANDARD_TIMEOUT;

    //
    //  Note that all parameter validation is done in the driver, so there's
    //  no point in having redundant validation code here.
    //

    Success = DeviceIoControl(
                  WriterHandle,
                  IOCTL_CDWRITER_CONTROL,
                  WriterControl,
                  sizeof( CDWRITER_CONTROL ),
                  NULL,
                  0,
                  &Actual,
                  NULL
                  );

    if ( ! Success ) {
        Status = CDWO_API_STATUS_FAILURE;
        if ( WriterControl->SrbStatus & SRB_STATUS_AUTOSENSE_VALID ) {
            Status |= CDWO_API_STATUS_SENSE_DATA_VALID;
            }
        }
    else {
        Status = CDWO_API_STATUS_SUCCESS;
        }

    return Status;
    }



LPCSTR
TranslateScsiStatus(
    IN UCHAR ScsiStatus
    )
    {
    switch ( ScsiStatus ) {
        case 0x00:  return "SCSI Success";          //  no error
        case 0x02:  return "SCSI Check Condition";
        case 0x04:  return "SCSI Condition Met";
        case 0x08:  return "SCSI Busy";
        case 0x10:  return "SCSI Intermediate";
        case 0x14:  return "SCSI Intermediate Met";
        case 0x18:  return "SCSI Reservation Conflict";
        case 0x22:  return "SCSI Command Terminated";
        case 0x28:  return "SCSI Queue Full";
        default:    return "Unknown SCSI Error";
        }
    }


LPCSTR SrbStatusText[] = {

    /* 0x00 */  "SCSI Request Pending",
    /* 0x01 */  "SCSI Success",                     //  no error
    /* 0x02 */  "SCSI Request Aborted",
    /* 0x03 */  "SCSI Abort Failed",
    /* 0x04 */  "SCSI Error",
    /* 0x05 */  "SCSI Busy",
    /* 0x06 */  "SCSI Invalid Request",
    /* 0x07 */  "SCSI Invalid Path ID",
    /* 0x08 */  "SCSI Device Does Not Exist",
    /* 0x09 */  "SCSI Timeout",
    /* 0x0A */  "SCSI Selection Timeout",
    /* 0x0B */  "SCSI Command Timeout",
    /* 0x0C */  "Unknown SCSI Error",
    /* 0x0D */  "SCSI Message Rejected",
    /* 0x0E */  "SCSI Bus Reset",
    /* 0x0F */  "SCSI Parity Error",
    /* 0x10 */  "SCSI Request Sense Failed",
    /* 0x11 */  "No SCSI Host Bus Adapter",
    /* 0x12 */  "SCSI Success",                     //  overrun or underrun
    /* 0x13 */  "SCSI Unexpected Bus Free",
    /* 0x14 */  "SCSI Phase Sequence Failure",
    /* 0x15 */  "SCSI Bad SRB Block Length",
    /* 0x16 */  "SCSI Request Flushed",
    /* 0x17 */  "Unknown SCSI Error",
    /* 0x18 */  "Unknown SCSI Error",
    /* 0x19 */  "Unknown SCSI Error",
    /* 0x20 */  "SCSI Invalid LUN",
    /* 0x21 */  "SCSI Invalid Target ID",
    /* 0x22 */  "Bad SCSI Function",
    /* 0x23 */  "SCSI Error Recovery"

    };


LPCSTR
TranslateSrbStatus(
    IN UCHAR SrbStatus
    )
    {
    if (( SrbStatus &= 0x3F ) <= 0x23 ) {
        return SrbStatusText[ SrbStatus ];
        }
    else {
        return "Unknown SCSI Error";
        }
    }



