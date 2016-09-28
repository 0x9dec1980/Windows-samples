/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Module Name:

    umpd.c

Abstract:

    sample source


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <wchar.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>

#if DBG
#define DBGMSG( argsPrint )     printf argsPrint
#else
#define DBGMSG( argsPrint )
#endif

//
// Common string definitions
//

HANDLE hInst;
CRITICAL_SECTION UmpdSection;


BOOL
DllEntryPoint(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes)
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        hInst = hModule;

        InitializeCriticalSection(&UmpdSection);
        DisableThreadLibraryCalls(hModule);

        return TRUE;

    case DLL_PROCESS_DETACH:
        return TRUE;
    }

    UNREFERENCED_PARAMETER( lpRes );
}


typedef struct _INIDRIVER {
    DWORD       signature;
    HANDLE      hPrinter;
    DWORD       JobId;
    DWORD       Status;
    UINT        cWritePrinter;
    UINT        cStartPage;
    UINT        cEndPage;
} INIDRIVER, *PINIDRIVER;

#define DRIVER_SIGNATURE    'DRSG'

#define  STATUS_STARTDOC     0x00000001
#define  STATUS_ENDDOC       0x00000002
#define  STATUS_ABORT        0x00000004



HANDLE
DrvSplStartDoc(
    HANDLE  hPrinter,
    DWORD   JobId
)
{
    PINIDRIVER  pData;
    HANDLE  hDriver;

    //
    //  Allocate Per Job Instance Data
    //

    pData = GlobalAlloc(GMEM_FIXED, sizeof( INIDRIVER ));

    if ( pData ){

        pData->signature = DRIVER_SIGNATURE;
        pData->hPrinter  = hPrinter;
        pData->JobId     = JobId;
        pData->Status    = STATUS_STARTDOC;
    }

    hDriver = pData;

    DBGMSG( ("DrvSplStartDoc called hPrinter %x JobId %d returning handle %x\n", hPrinter, JobId, hDriver ));

    return hDriver;
}


BOOL
DrvSplWritePrinter(
    HANDLE  hDriver,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    PINIDRIVER pData = (PINIDRIVER) hDriver;
    BOOL    bReturn;

    assert( pData && pData->signature == DRIVER_SIGNATURE );

    DBGMSG( ("DrvSplWritePrinter call #%d hDriver %x pBuf %x cbBuf %d pcWritten %x\n",
                         pData->cWritePrinter, hDriver, pBuf, cbBuf, pcWritten ));

    //
    //  Do processing of the Data
    //

    pData->cWritePrinter++;

    //
    //  Pass the data on to the spooler
    //

    bReturn = WritePrinter( pData->hPrinter, pBuf, cbBuf, pcWritten );

    return bReturn;
}


VOID
DrvSplEndDoc(
    HANDLE  hDriver
)
{
    PINIDRIVER pData = (PINIDRIVER) hDriver;

    assert( pData &&
            pData->signature == DRIVER_SIGNATURE &&
            pData->Status & STATUS_STARTDOC );

    DBGMSG( ("DrvSplEndDoc called hDriver %x Status %x cWritePrinter %d cStartPage %d cEndpage %d\n",
                         hDriver, pData->Status, pData->cWritePrinter, pData->cStartPage, pData->cEndPage ));

    //
    //  Last chance to do any processing on this job.
    //  Flush any last IOs or do any other processing on this job
    //

    pData->Status |= STATUS_ENDDOC;
}



VOID
DrvSplClose(
    HANDLE  hDriver
)
{
    PINIDRIVER pData = (PINIDRIVER) hDriver;

    assert( pData &&
            pData->signature == DRIVER_SIGNATURE );


    DBGMSG( ("DrvSplClose called hDriver %x Status %x\n", hDriver, pData->Status ));

    //
    //  Do resource cleanup no more calls will be made with this handle
    //

#if DBG
    //  Make sure we don't see this as a handle again
    pData->signature = 0xdeadf00d;
#endif

    GlobalFree( pData );
}



//
//  Optional Routines
//
//




BOOL
DrvSplStartPage(
    HANDLE  hDriver
)
{
    PINIDRIVER pData = (PINIDRIVER) hDriver;

    assert( pData &&
            pData->signature == DRIVER_SIGNATURE &&
            pData->Status & STATUS_STARTDOC );

    //
    //  Do Pre Page Processing Now
    //

    pData->cStartPage++;


    DBGMSG( ("DrvSplStartPage called hDriver %x cStartPage %d\n", hDriver, pData->cStartPage ));

    return  TRUE;

}


BOOL
DrvSplEndPage(
    HANDLE  hDriver
)
{
    PINIDRIVER pData = (PINIDRIVER) hDriver;

    assert( pData &&
            pData->signature == DRIVER_SIGNATURE &&
            pData->Status & STATUS_STARTDOC );

    //
    //  Do Post Page Processing Now
    //

    pData->cEndPage++;

    DBGMSG( ("DrvSplEndPage called hDriver %x cEndPage %d\n", hDriver, pData->cEndPage ));

    return  TRUE;
}



VOID
DrvSplAbort(
    HANDLE  hDriver
)
{
    PINIDRIVER pData = (PINIDRIVER) hDriver;

    assert( pData &&
            pData->signature == DRIVER_SIGNATURE &&
            pData->Status & STATUS_STARTDOC );


    //
    //  Abort requested, free resouces, throw out pending data
    //  put printer to known state.
    //

    pData->Status |= STATUS_ABORT;

    DBGMSG( ("DrvSplAbort called hDriver %x Status %x cWritePrinter %d cStartPage %d cEndpage %d\n",
                         hDriver, pData->Status, pData->cWritePrinter, pData->cStartPage, pData->cEndPage ));

}
