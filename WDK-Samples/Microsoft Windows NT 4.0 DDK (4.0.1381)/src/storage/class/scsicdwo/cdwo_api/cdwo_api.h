
/*++

Copyright (C) 1995-1996  Microsoft Corporation

Module Name:

    cdwo_api.h

Abstract:

    User-mode interface for cdwo_api interface to scsicdwo driver
    for writing CDs.

Author:

    Tom McGuire (tommcg)

--*/


#ifndef _CDWO_API_
#define _CDWO_API_

typedef ULONG CDWO_API_STATUS;

//
//  General error codes.  If CDWO_API_STATUS_FAILURE bit is set, other bits
//  might also be set to indicate the nature of the failure.
//

#define CDWO_API_STATUS_SUCCESS             0x00000000
#define CDWO_API_STATUS_FAILURE             0x80000000
#define CDWO_API_STATUS_SENSE_DATA_VALID    0x00000001


//
//  Prototypes for interfacing CD Writer(s) via the scsicdwo driver.
//

HANDLE
CdWriterOpen(
    IN ULONG WriterNumber
    );

CDWO_API_STATUS
CdWriterQueryParameters(
    IN     HANDLE               WriterHandle,
    IN OUT PCDWRITER_PARAMETERS WriterParams        // see scsicdwo.h
    );

CDWO_API_STATUS
CdWriterExecuteCommand(
    IN     HANDLE            WriterHandle,
    IN OUT PCDWRITER_CONTROL WriterControl          // see scsicdwo.h
    );


//
//  Helper functions:
//

LPCSTR
TranslateScsiStatus(
    IN UCHAR ScsiStatus
    );

LPCSTR
TranslateSrbStatus(
    IN UCHAR SrbStatus
    );


#endif /* _CDWO_API_ */
