/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    packet32.c

Abstract:


Author:

    BrianL

Environment:

    User mode only.

Notes:


Future:



Revision History:

--*/

#define UNICODE 1

#include <windows.h>
#include <windowsx.h>

#include <ntddndis.h>
#include <ntddpack.h>

#include <packet32.h>

TCHAR   szWindowTitle[] = TEXT("PACKET32.DLL");






#if DBG

#define ODS(_x) OutputDebugString(TEXT(_x))

#else

#define ODS(_x)

#endif




BOOLEAN
StartPacketDriver(
    LPTSTR     ServiceName
    );





BOOL
PacketInit(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:


Arguments:

    DllHandle - Not Used

    Reason - Attach or Detach

    Context - Not Used

Return Value:

    SUCCESS - TRUE
    FAILURE - FALSE

--*/

{
    BOOLEAN     Status;

    ODS("Packet32: DllEntry\n");


    switch ( Reason ) {

        case DLL_PROCESS_ATTACH:

            Status=StartPacketDriver(TEXT("PACKET"));

            break;

        case DLL_PROCESS_DETACH:

            break;

        default:

            break;

    }

    return Status;
}








PVOID
PacketOpenAdapter(
    LPTSTR   AdapterName
    )
/*++

Routine Description:

    This rotine opens an instance of the adapter

Arguments:

    AdapterName - unicode name of the adapter to open

Return Value:

    SUCCESS - returns a pointer to an adapter object
    FAILURE - NULL

--*/


{

    LPADAPTER  lpAdapter;
    BOOLEAN    Result;

    ODS("Packet32: PacketOpenAdapter\n");

    lpAdapter=(LPADAPTER)GlobalAllocPtr(
                             GMEM_MOVEABLE | GMEM_ZEROINIT,
                             sizeof(ADAPTER)
                             );


    if (lpAdapter==NULL) {

        ODS("Packet32: PacketOpenAdapter GlobalAlloc Failed\n");

        return NULL;

    }

    wsprintf(
        lpAdapter->SymbolicLink,
        TEXT("\\\\.\\%s%s"),
        DOSNAMEPREFIX,
        &AdapterName[8]
        );

    Result=DefineDosDevice(
        DDD_RAW_TARGET_PATH,
        &lpAdapter->SymbolicLink[4],
        AdapterName
        );

    if (Result) {

        lpAdapter->hFile=CreateFile(lpAdapter->SymbolicLink,
                             GENERIC_WRITE | GENERIC_READ,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_FLAG_OVERLAPPED,
                             0
                             );

        if (lpAdapter->hFile != INVALID_HANDLE_VALUE) {

            return lpAdapter;

        }


    }

    ODS("Packet32: PacketOpenAdapter Could not open adapter\n");

    GlobalFreePtr(
        lpAdapter
        );

    return NULL;


}


VOID
PacketCloseAdapter(
    LPADAPTER   lpAdapter
    )

{
/*++

Routine Description:

    This rotine closes a previouly opened adapter

Arguments:

    Adapter object returned from PacketOpenAdapter

Return Value:


--*/

    ODS("Packet32: PacketCloseAdapter\n");

    CloseHandle(lpAdapter->hFile);

    GlobalFreePtr(lpAdapter);

}




PVOID
PacketAllocatePacket(
    LPADAPTER   AdapterObject
    )

{
/*++

Routine Description:

    This rotine this routine allocates a packet object for use
    in sending a receiveing packets

Arguments:

    Adapter object returned from PacketOpenAdapter

Return Value:

    SUCCESS - returns packet object
    FAILURE - NULL

--*/

    LPPACKET    lpPacket;

    lpPacket=(LPPACKET)GlobalAllocPtr(
                             GMEM_MOVEABLE | GMEM_ZEROINIT,
                             sizeof(PACKET)
                             );

    if (lpPacket==NULL) {

        ODS("Packet32: PacketAllocateSendPacket: GlobalAlloc Failed\n");

        return NULL;

    }

    lpPacket->OverLapped.hEvent=CreateEvent(
                        NULL,
                        FALSE,
                        FALSE,
                        NULL
                        );

    if (lpPacket->OverLapped.hEvent==NULL) {

        ODS("Packet32: PacketAllocateSendPacket: CreateEvent Failed\n");

        GlobalFreePtr(lpPacket);

        return NULL;
    }

    return lpPacket;

}

VOID
PacketFreePacket(
    LPPACKET    lpPacket
    )

{
    CloseHandle(lpPacket->OverLapped.hEvent);

    GlobalFreePtr(lpPacket);

}

VOID
PacketInitPacket(
    LPPACKET    lpPacket,
    PVOID       Buffer,
    UINT        Length
    )
/*++

Routine Description:

    This rotine initializes a packet object to point to
    a memory buffer described by Start address and length

Arguments:

    lpPacket   -  Packet object returned by PacketAllocatePacket

    Buffer     -  Begining address of a memory buffer

    Length     -  Length of memory buffer

Return Value:


--*/

{

    lpPacket->Buffer=Buffer;
    lpPacket->Length=Length;

}


BOOLEAN
PacketSendPacket(
    LPADAPTER   AdapterObject,
    LPPACKET    lpPacket,
    BOOLEAN     Sync
    )
/*++

Routine Description:

    This rotine sends a packet to the adapter

Arguments:

    AdapterObject  - AdapterObject return by PacketOpenAdapter

    lpPacket       - Packet object returned by PacketAllocatePacket and initialized
                     by PacketInitPacket

    Sync           - TRUE if service should wait for packet to transmit


Return Value:

    SUCCESS - TRUE if succeeded and SYNC==TRUE
    FAILURE -

--*/

{
    BOOLEAN      Result;

    DWORD      BytesTransfered;


    lpPacket->OverLapped.Offset=0;
    lpPacket->OverLapped.OffsetHigh=0;

    if (!ResetEvent(lpPacket->OverLapped.hEvent)) {

        return FALSE;

    }

    Result=WriteFile(
              AdapterObject->hFile,
              lpPacket->Buffer,
              lpPacket->Length,
              &BytesTransfered,
              &lpPacket->OverLapped
              );

    if (Sync) {
        //
        //  They want to wait
        //
        Result=GetOverlappedResult(
                   AdapterObject->hFile,
                   &lpPacket->OverLapped,
                   &BytesTransfered,
                   TRUE
                   );



    } else {
        //
        //  They don't want to wait, they will call PacketWaitPacket to get
        //  The real result
        //
        Result=TRUE;

    }

    return Result;

}



BOOLEAN
PacketReceivePacket(
    LPADAPTER   AdapterObject,
    LPPACKET    lpPacket,
    BOOLEAN     Sync,
    PULONG      BytesReceived
    )
/*++

Routine Description:

    This rotine issues a receive request from the adapter

Arguments:

    AdapterObject  - AdapterObject return by PacketOpenAdapter

    lpPacket       - Packet object returned by PacketAllocatePacket and initialized
                     by PacketInitPacket

    Sync           - TRUE if service should wait for packet to transmit


Return Value:

    SUCCESS - TRUE if succeeded and SYNC==TRUE
    FAILURE -

--*/


{
    BOOLEAN      Result;

    lpPacket->OverLapped.Offset=0;
    lpPacket->OverLapped.OffsetHigh=0;

    if (!ResetEvent(lpPacket->OverLapped.hEvent)) {

        return FALSE;

    }

    Result=ReadFile(
              AdapterObject->hFile,
              lpPacket->Buffer,
              lpPacket->Length,
              BytesReceived,
              &lpPacket->OverLapped
              );

    if (Sync) {
        //
        //  They want to wait
        //
        Result=GetOverlappedResult(
                   AdapterObject->hFile,
                   &lpPacket->OverLapped,
                   BytesReceived,
                   TRUE
                   );


    } else {
        //
        //  They don't want to wait, they will call PacketWaitPacket to get
        //  The real result
        //
        Result=TRUE;

    }


    return Result;

}


BOOLEAN
PacketWaitPacket(
    LPADAPTER  AdapterObject,
    LPPACKET   lpPacket,
    PULONG     BytesReceived
    )
/*++

Routine Description:

    This routine waits for an overlapped IO on a packet to complete
    Called if the send or receive call specified FALSE for the Sync parmeter

Arguments:

    AdapterObject  - AdapterObject return by PacketOpenAdapter

    lpPacket       - Packet object returned by PacketAllocatePacket and initialized
                     by PacketInitPacket

Return Value:

    SUCCESS - TRUE if overlapped call succeeded
    FAILURE -

--*/

{


    return GetOverlappedResult(
               AdapterObject->hFile,
               &lpPacket->OverLapped,
               BytesReceived,
               TRUE
               );


}


BOOLEAN
PacketResetAdapter(
    LPADAPTER  AdapterObject
    )
/*++

Routine Description:

    This rotuine resets the adapter. This will complete all pending sends receives and requests

Arguments:

    AdapterObject  - AdapterObject return by PacketOpenAdapter

Return Value:

    SUCCESS - TRUE if overlapped call succeeded
    FAILURE -

--*/

{
    UINT       BytesReturned;

    DeviceIoControl(
        AdapterObject->hFile,
        (DWORD)IOCTL_PROTOCOL_RESET,
        NULL,
        0,
        NULL,
        0,
        &BytesReturned,
        NULL
        );

    return TRUE;

}


BOOLEAN
PacketRequest(
    LPADAPTER  AdapterObject,
    BOOLEAN    Set,
    PPACKET_OID_DATA  OidData
    )
/*++

Routine Description:

    This routine sends issues a request to and adapter

Arguments:

    AdapterObject  - AdapterObject return by PacketOpenAdapter

    Set            - True means that the request is SET otherwise it is a query

    OidData        - Structure containing the details of the OID

Return Value:

    SUCCESS -
    FAILURE -

--*/

{
    UINT       BytesReturned;
    BOOLEAN    Result;

    Result=DeviceIoControl(
        AdapterObject->hFile,
        (DWORD) Set ? IOCTL_PROTOCOL_SET_OID : IOCTL_PROTOCOL_QUERY_OID,
        OidData,
        sizeof(PACKET_OID_DATA)-1+OidData->Length,
        OidData,
        sizeof(PACKET_OID_DATA)-1+OidData->Length,
        &BytesReturned,
        NULL
        );

    return Result;
}



BOOLEAN
PacketSetFilter(
    LPADAPTER  AdapterObject,
    ULONG      Filter
    )
/*++

Routine Description:

    This rotine sets the adapters packet filter

Arguments:

    AdapterObject  - AdapterObject return by PacketOpenAdapter

    Filter         - filter to be set

Return Value:

    SUCCESS -
    FAILURE -

--*/

{

    BOOLEAN    Status;


    ULONG      IoCtlBufferLength=(sizeof(PACKET_OID_DATA)+sizeof(ULONG)-1);

    PPACKET_OID_DATA  OidData;

    OidData=GlobalAllocPtr(
        GMEM_MOVEABLE | GMEM_ZEROINIT,
        IoCtlBufferLength
        );


    if (OidData == NULL) {

        return FALSE;

    }

    OidData->Oid=OID_GEN_CURRENT_PACKET_FILTER;
    OidData->Length=sizeof(ULONG);
    *((PULONG)OidData->Data)=Filter;

    Status=PacketRequest(
        AdapterObject,
        TRUE,
        OidData
        );

    GlobalFreePtr(OidData);



    return Status;

}


BOOLEAN
StartPacketDriver(
    LPTSTR     ServiceName
    )
/*++

Routine Description:

    This routine Atempts to start the kernel mode packet driver

Arguments:

    ServiceName  - Name of service to try to start

Return Value:

    SUCCESS -
    FAILURE -

--*/

{

    BOOLEAN  Status;

    SC_HANDLE  SCManagerHandle;
    SC_HANDLE  SCServiceHandle;

    /* Open a handle to the SC Manager database. */

    SCManagerHandle = OpenSCManager(
                      NULL,                   /* local machine           */
                      NULL,                   /* ServicesActive database */
                      SC_MANAGER_ALL_ACCESS); /* full access rights      */

    if (SCManagerHandle==NULL) {

        MessageBox(NULL,TEXT("Could not open SC"),szWindowTitle,MB_OK);

        return FALSE;

    } else {

        SCServiceHandle=OpenService(SCManagerHandle,
                            ServiceName,
                            SERVICE_START
                            );

        if (SCServiceHandle == NULL) {

            MessageBox(NULL,TEXT("Could not open service"),szWindowTitle,MB_OK);

        }

        Status=StartService(
                   SCServiceHandle,
                   0,
                   NULL
                   );

        if (!Status) {

            if (GetLastError()==ERROR_SERVICE_ALREADY_RUNNING) {

                ODS("Packet32: Packet service already started\n");

                return TRUE;
            }
        }

        return Status;

    }

    return FALSE;

}




ULONG
PacketGetAdapterNames(
    PTSTR   pStr,
    PULONG  BufferSize
    )
/*++

Routine Description:

    This routine returns the names all adapters availible

Arguments:

    Pstr       -  Pointer to a buffer which recieves the UNICODE names
                  Each name is NULL terminated with a second NULL at the end
                  of the list.

    BufferSize -  Size of the buffer passed in


Return Value:


    SUCCESS -
    FAILURE -

--*/

{

    HKEY       SystemKey;
    HKEY       ControlSetKey;
    HKEY       ServicesKey;
    HKEY       NdisPerfKey;
    HKEY       LinkageKey;
    LONG       Status;

    DWORD      RegType;

    Status=RegOpenKeyEx(
               HKEY_LOCAL_MACHINE,
               TEXT("SYSTEM"),
               0,
               KEY_READ,
               &SystemKey
               );

    if (Status == ERROR_SUCCESS) {

        Status=RegOpenKeyEx(
                   SystemKey,
                   TEXT("CurrentControlSet"),
                   0,
                   KEY_READ,
                   &ControlSetKey
                   );

        if (Status == ERROR_SUCCESS) {

            Status=RegOpenKeyEx(
                       ControlSetKey,
                       TEXT("Services"),
                       0,
                       KEY_READ,
                       &ServicesKey
                       );

            if (Status == ERROR_SUCCESS) {

                Status=RegOpenKeyEx(
                           ServicesKey,
                           TEXT("Packet"),
                           0,
                           KEY_READ,
                           &NdisPerfKey
                           );

                if (Status == ERROR_SUCCESS) {

                    Status=RegOpenKeyEx(
                               NdisPerfKey,
                               TEXT("Linkage"),
                               0,
                               KEY_READ,
                               &LinkageKey
                               );


                    if (Status == ERROR_SUCCESS) {

                        Status=RegQueryValueEx(
                                   LinkageKey,
                                   TEXT("Export"),
                                   NULL,
                                   &RegType,
                                   (LPBYTE)pStr,
                                   BufferSize
                                   );


                        RegCloseKey(LinkageKey);
                    }

                    RegCloseKey(NdisPerfKey);
                }

                RegCloseKey(ServicesKey);
            }

            RegCloseKey(ControlSetKey);
        }

        RegCloseKey(SystemKey);
    }


    return Status;

}
