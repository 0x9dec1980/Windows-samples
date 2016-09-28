/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Detect.c

--*/

#include <ntddk.h>
#include <ntddnetd.h>

#include <windef.h>
#include <winerror.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "detect.h"

#if DBG
#define STATIC
#else
#define STATIC static
#endif

//  Prototype "borrowed" from WINUSER.H
extern int WINAPIV wsprintfW(LPWSTR, LPCWSTR, ...);
//  Prototype "borrowed" from WINBASE.H
VOID WINAPI Sleep( DWORD dwMilliseconds );


//
// This is the structure for all the cards which MS is shipping in this DLL.
// To add detection for a new adapter(s), simply add the proper routines to
// this structure.  The rest is automatic.
//

DETECT_ADAPTER DetectAdapters[] = {

        {
            LanceIdentifyHandler,
            LanceFirstNextHandler,
            LanceOpenHandleHandler,
            LanceCreateHandleHandler,
            LanceCloseHandleHandler,
            LanceQueryCfgHandler,
            LanceVerifyCfgHandler,
            LanceQueryMaskHandler,
            LanceParamRangeHandler,
            LanceQueryParameterNameHandler,
            0

        },

        {
            EisaIdentifyHandler,
            EisaFirstNextHandler,
            EisaOpenHandleHandler,
            EisaCreateHandleHandler,
            EisaCloseHandleHandler,
            EisaQueryCfgHandler,
            EisaVerifyCfgHandler,
            EisaQueryMaskHandler,
            EisaParamRangeHandler,
            EisaQueryParameterNameHandler,
            0

        }

    };


//
// Constant strings for parameters
//


WCHAR IrqString[] = L"IRQ";
WCHAR IrqTypeString[] = L"IRQTYPE";
WCHAR IoAddrString[] = L"IOADDR";
WCHAR IoLengthString[] = L"IOADDRLENGTH";
WCHAR MemAddrString[] = L"MEMADDR";
WCHAR MemLengthString[] = L"MEMADDRLENGTH";
WCHAR TransceiverString[] = L"TRANSCEIVER";
WCHAR ZeroWaitStateString[] = L"ZEROWAITSTATE";
WCHAR SlotNumberString[] = L"SLOTNUMBER";


//
// Variables for keeping track of the resources that the DLL currently has
// claimed.
//

PNETDTECT_RESOURCE ResourceList;
ULONG NumberOfResources = 0;
ULONG NumberOfAllocatedResourceSlots = 0;


BOOLEAN
PASCAL
NcDetectInitialInit(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This routine calls CreateFile to open the device driver.

Arguments:

    DllHandle - Not Used

    Reason - Attach or Detach

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG SupportedAdapters;
    LONG TotalAdapters = 0;
    LONG ReturnValue;
    LONG Length = 0;

    if (Reason == 0) {

        //
        // This is the close call
        //

        //
        // Free ResourceList
        //

        if (NumberOfAllocatedResourceSlots > 0) {

            DetectFreeHeap(ResourceList);

        }

        //
        // Free any temporary resources
        //

        // BUGBUG:  Where is this function?
        //   DetectFreeTemporaryResources();

        return(TRUE);

    }

    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // Count the total number of adapters supported by this DLL by
        // iterating through each module, finding the number of adapters
        // each module supports.
        //

        SupportedAdapters = 0;

        for ( ; ; SupportedAdapters++) {

            ReturnValue =
               (*(DetectAdapters[CurrentDriver].NcDetectIdentifyHandler))(
                   ((SupportedAdapters+10) * 100),
                   NULL,
                   Length
                   );

            if (ReturnValue == ERROR_NO_MORE_ITEMS) {

                break;

            }

        }

        TotalAdapters += SupportedAdapters;

        DetectAdapters[CurrentDriver].SupportedAdapters = SupportedAdapters;

    }

    if (TotalAdapters > 0xFFFF) {

        //
        // We do not support more than this many adapters in this DLL
        // because of the way we build the Tokens and NetcardIds.
        //

        return(FALSE);

    }

    return TRUE;
}

LONG
NcDetectIdentify(
    IN  LONG Index,
    OUT WCHAR *Buffer,
    IN LONG BuffSize
    )

/*++

Routine Description:

    This routine returns information about the netcards supported by
    this DLL.

Arguments:

    Index -  The index of the netcard being address.  The first
    cards information is at index 1000, the second at 1100, etc.

    Buffer - Buffer to store the result into.

    BuffSize - Number of bytes in pwchBuffer

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (Index / 100) - 10;
    LONG CodeNumber = Index % 100;


    //
    // First we check the index for any of the 'special' values.
    //

    if (Index == 0) {

        //
        // Return manufacturers identfication
        //

        if (BuffSize < 4) {

            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Copy in the identification number
        //

        wsprintfW(Buffer,L"0x0");

        return(0);

    }

    if (Index == 1) {

        //
        // Return the date and version
        //

        if (BuffSize < 12) {

            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Copy it in
        //

        wsprintfW(Buffer,L"0x10920301");

        return(0);

    }

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);


    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectIdentifyHandler))(
                  ((AdapterNumber + 10) * 100) + CodeNumber,
                  Buffer,
                  BuffSize
                  );

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_NO_MORE_ITEMS);

}

LONG
NcDetectFirstNext(
    IN  LONG NetcardId,
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    IN  BOOL First,
    OUT PVOID *Token,
    OUT LONG *Confidence
    )

/*++

Routine Description:

    This routine finds the instances of a physical adapter identified
    by the NetcardId.

Arguments:

    NetcardId -  The index of the netcard being address.  The first
    cards information is id 1000, the second id 1100, etc.

    InterfaceType - Any bus type.

    BusNumber - The bus number of the bus to search.

    First - TRUE is we are to search for the first instance of an
    adapter, FALSE if we are to continue search from a previous stopping
    point.

    Token - A pointer to a handle to return to identify the found
    instance

    Confidence - A pointer to a long for storing the confidence factor
    that the card exists.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (NetcardId / 100) - 10;

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    if ((NetcardId % 100) != 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            //
            // Yes, so call to get the right one.
            //

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectFirstNextHandler))(
                    (AdapterNumber + 10) * 100,
                    InterfaceType,
                    BusNumber,
                    First,
                    Token,
                    Confidence
                  );

            if (ReturnValue == 0) {

                //
                // Store information
                //

                *Token = (PVOID)(((ULONG)(*Token)) | (CurrentDriver << 16));

            } else {

                *Token = 0;

            }

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_INVALID_PARAMETER);
}




LONG
NcDetectOpenHandle(
    IN  PVOID Token,
    OUT PVOID *Handle
    )

/*++

Routine Description:

    This routine takes a token returned by FirstNext and converts it
    into a permanent handle.

Arguments:

    Token - The token.

    Handle - A pointer to the handle, so we can store the resulting
    handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG ReturnValue;
    LONG DriverToken = ((ULONG)Token & 0xFFFF);
    LONG DriverNumber = ((ULONG)Token >> 16);
    PADAPTER_HANDLE Adapter;

    ReturnValue =
              (*(DetectAdapters[DriverNumber].NcDetectOpenHandleHandler))(
                    (PVOID)DriverToken,
                    Handle
                  );

    if (ReturnValue == 0) {

        //
        // Store information
        //

        Adapter = DetectAllocateHeap(
                       sizeof(ADAPTER_HANDLE)
                       );

        if (Adapter == NULL) {

            //
            // Error
            //

            (*(DetectAdapters[DriverNumber].NcDetectCloseHandleHandler))(
                    *Handle
                  );

            return(ERROR_NOT_ENOUGH_MEMORY);

        }

        Adapter->Handle = *Handle;
        Adapter->DriverNumber = DriverNumber;

        *Handle = Adapter;

    } else {

        *Handle = NULL;

    }

    return(ReturnValue);

}




LONG
NcDetectCreateHandle(
    IN  LONG NetcardId,
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    OUT PVOID *Handle
    )

/*++

Routine Description:

    This routine is used to force the creation of a handle for cases
    where a card is not found via FirstNext, but the user says it does
    exist.

Arguments:

    NetcardId - The id of the card to create the handle for.

    InterfaceType - Any bus type.

    BusNumber - The bus number of the bus in the system.

    Handle - A pointer to the handle, for storing the resulting handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (NetcardId / 100) - 10;
    PADAPTER_HANDLE Adapter;

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    if ((NetcardId % 100) != 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            //
            // Yes, so call to get the right one.
            //

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectCreateHandleHandler))(
                    (AdapterNumber + 10) * 100,
                    InterfaceType,
                    BusNumber,
                    Handle
                  );

            if (ReturnValue == 0) {

                //
                // Store information
                //

                Adapter = DetectAllocateHeap(
                               sizeof(ADAPTER_HANDLE)
                               );

                if (Adapter == NULL) {

                    //
                    // Error
                    //

                    (*(DetectAdapters[CurrentDriver].NcDetectCloseHandleHandler))(
                            *Handle
                          );

                    return(ERROR_NOT_ENOUGH_MEMORY);

                }

                Adapter->Handle = *Handle;
                Adapter->DriverNumber = CurrentDriver;

                *Handle = Adapter;

            } else {

                *Handle = NULL;

            }

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_INVALID_PARAMETER);
}



LONG
NcDetectCloseHandle(
    IN PVOID Handle
    )

/*++

Routine Description:

    This frees any resources associated with a handle.

Arguments:

   pvHandle - The handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PADAPTER_HANDLE Adapter = (PADAPTER_HANDLE)(Handle);

    (*(DetectAdapters[Adapter->DriverNumber].NcDetectCloseHandleHandler))(
                            Adapter->Handle
                          );

    DetectFreeHeap( Adapter );

    return(0);
}



LONG
NcDetectQueryCfg(
    IN  PVOID Handle,
    OUT WCHAR *Buffer,
    IN  LONG BuffSize
    )

/*++

Routine Description:

    This routine calls the appropriate driver's query config handler to
    get the parameters for the adapter associated with the handle.

Arguments:

    Handle - The handle.

    Buffer - The resulting parameter list.

    BuffSize - Length of the given buffer in WCHARs.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PADAPTER_HANDLE Adapter = (PADAPTER_HANDLE)(Handle);
    LONG ReturnValue;

    ReturnValue = (*(DetectAdapters[Adapter->DriverNumber].NcDetectQueryCfgHandler))(
                            Adapter->Handle,
                            Buffer,
                            BuffSize
                          );

    return(ReturnValue);
}



LONG
NcDetectVerifyCfg(
    IN PVOID Handle,
    IN WCHAR *Buffer
    )

/*++

Routine Description:

    This routine verifys that a given parameter list is complete and
    correct for the adapter associated with the handle.

Arguments:

    Handle - The handle.

    Buffer - The parameter list.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PADAPTER_HANDLE Adapter = (PADAPTER_HANDLE)(Handle);
    LONG ReturnValue;

    ReturnValue = (*(DetectAdapters[Adapter->DriverNumber].NcDetectVerifyCfgHandler))(
                            Adapter->Handle,
                            Buffer
                          );

    return(ReturnValue);
}



LONG
NcDetectQueryMask(
    IN  LONG NetcardId,
    OUT WCHAR *Buffer,
    IN  LONG BuffSize
    )

/*++

Routine Description:

    This routine returns the parameter list information for a specific
    network card.

Arguments:

    NetcardId - The id of the desired netcard.

    Buffer - The buffer for storing the parameter information.

    BuffSize - Length of Buffer in WCHARs.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (NetcardId / 100) - 10;

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    if ((NetcardId % 100) != 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            //
            // Yes, so call to get the right one.
            //

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectQueryMaskHandler))(
                  ((AdapterNumber + 10) * 100),
                  Buffer,
                  BuffSize
                  );

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_INVALID_PARAMETER);
}

LONG
NcDetectParamRange(
    IN  LONG NetcardId,
    IN  WCHAR *Param,
    OUT LONG *Values,
    OUT LONG *BuffSize
    )

/*++

Routine Description:

    This routine returns a list of valid values for a given parameter name
    for a given card.

Arguments:

    NetcardId - The Id of the card desired.

    Param - A WCHAR string of the parameter name to query the values of.

    Values - A pointer to a list of LONGs into which we store valid values
    for the parameter.

    BuffSize - At entry, the length of plValues in LONGs.  At exit, the
    number of LONGs stored in plValues.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (NetcardId / 100) - 10;

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    if ((NetcardId % 100) != 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            //
            // Yes, so call to get the right one.
            //

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectParamRangeHandler))(
                  ((AdapterNumber + 10) * 100),
                  Param,
                  Values,
                  BuffSize
                  );

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_INVALID_PARAMETER);
}



LONG
NcDetectQueryParameterName(
    IN  WCHAR *Param,
    OUT WCHAR *Buffer,
    IN  LONG  BufferSize
    )

/*++

Routine Description:

    Returns a localized, displayable name for a specific parameter.

Arguments:

    Param - The parameter to be queried.

    Buffer - The buffer to store the result into.

    BufferSize - The length of Buffer in WCHARs.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // No way to tell where this came from -- guess until success.
        //

        ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectQueryParameterNameHandler))(
                  Param,
                  Buffer,
                  BufferSize
                  );

        if (ReturnValue == 0) {

            return(0);

        }

    }

    return(ERROR_INVALID_PARAMETER);
}


LONG
NcDetectResourceClaim(
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    IN  ULONG Type,
    IN  ULONG Value,
    IN  ULONG Length,
    IN  ULONG Flags,
    IN  BOOL Claim
    )

/*++

Routine Description:

    Attempts to claim a resources, failing if there is a conflict.

Arguments:

    InterfaceType - Any type.

    BusNumber - The bus number of the bus to search.

    Type - The type of resource, Irq, Memory, Port, Dma

    Value - The starting value

    Length - The Length of the resource from starting value to end.

    Flags - If Type is IRQ, this defines if this is Latched or LevelSensitive.

    Claim - TRUE if we are to permanently claim the resource, else FALSE.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    ULONG i;
    NTSTATUS NtStatus;

    //
    // Check the resources we've claimed for ourselves
    //

    for (i = 0; i < NumberOfResources; i++) {

        if ((ResourceList[i].InterfaceType == InterfaceType) &&
            (ResourceList[i].BusNumber == BusNumber) &&
            (ResourceList[i].Type == Type)) {

            if (Value < ResourceList[i].Value) {

                if ((Value + Length) > ResourceList[i].Value) {

                    return(ERROR_SHARING_VIOLATION);

                }

            } else if (Value == ResourceList[i].Value) {

                return(ERROR_SHARING_VIOLATION);

            } else if (Value < (ResourceList[i].Value + ResourceList[i].Length)) {

                return(ERROR_SHARING_VIOLATION);

            }

        }

    }

    //
    // Make sure resource list has space for this one.
    //

    if (NumberOfResources == NumberOfAllocatedResourceSlots) {

        PVOID TmpList;

        //
        // Get more space
        //

        TmpList = DetectAllocateHeap((NumberOfAllocatedResourceSlots + 32) *
                                     sizeof(NETDTECT_RESOURCE)
                                    );

        //
        // Copy data
        //

        memcpy(TmpList,
               ResourceList,
               (NumberOfAllocatedResourceSlots * sizeof(NETDTECT_RESOURCE))
              );

        //
        // Update counter
        //

        NumberOfAllocatedResourceSlots += 32;

        //
        // Free old space
        //

        DetectFreeHeap(ResourceList);

        ResourceList = (PNETDTECT_RESOURCE)TmpList;

    }

    //
    // Add it to the list
    //

    ResourceList[NumberOfResources].InterfaceType = InterfaceType;
    ResourceList[NumberOfResources].BusNumber = BusNumber;
    ResourceList[NumberOfResources].Type = Type;
    ResourceList[NumberOfResources].Value = Value;
    ResourceList[NumberOfResources].Length = Length;
    ResourceList[NumberOfResources].Flags = Flags;

    //
    // Try to claim the resource
    //

    NtStatus = DetectClaimResource(
                  NumberOfResources + 1,
                  (PVOID)ResourceList
                  );


    //
    // If failed, exit
    //

    if (NtStatus == STATUS_CONFLICTING_ADDRESSES) {

        //
        // Undo the claim
        //

        DetectClaimResource(
                NumberOfResources,
                (PVOID)ResourceList
                );

        return(ERROR_SHARING_VIOLATION);

    }

    if (NtStatus) {

        return(ERROR_NOT_ENOUGH_MEMORY);

    }

    if (!Claim) {

        //
        // Undo the claim
        //

        DetectClaimResource(
                NumberOfResources,
                (PVOID)ResourceList
                );

    } else {

        //
        // Adjust total count only if this is permanent
        //

        NumberOfResources++;

    }

    //
    // no error
    //

    return(0);
}





//
// Support routines.
//
// These routines are common routines used within each detection module.
//






ULONG
UnicodeStrLen(
    IN WCHAR *String
    )

/*++

Routine Description:

    This routine returns the number of Unicode characters in a NULL
    terminated Unicode string.

Arguments:

    String - The string.

Return Value:

    The length in number of unicode characters

--*/


{
    ULONG Length;

    for (Length=0; ; Length++) {

        if (String[Length] == L'\0') {

            return Length;

        }

    }

}

WCHAR *
FindParameterString(
    IN WCHAR *String1,
    IN WCHAR *String2
    )

/*++

Routine Description:

    This routine returns a pointer to the first instance of String2
    in String1.  It assumes that String1 is a parameter list where
    each parameter name is terminated with a NULL and the entire
    string terminated by two consecutive NULLs.

Arguments:

    String1 -- String to search.

    String2 -- Substring to search for.

Return Value:

    Pointer to place in String1 of first character of String2 if it
    exists, else NULL.

--*/


{
    ULONG Length1;
    ULONG Length2;
    WCHAR *Place = String1;

    Length2 = UnicodeStrLen(String2) + 1;

    Length1 = UnicodeStrLen(String1) + 1;

    //
    // While not the NULL only
    //

    while (Length1 != 1) {

        //
        // Are these the same?
        //

        if (memcmp(Place, String2, Length2 * sizeof(WCHAR)) == 0) {

            //
            // Yes.
            //

            return(Place);

        }

        Place = (WCHAR *)(Place + Length1);

        Length1 = UnicodeStrLen(Place) + 1;

    }

    return(NULL);

}


VOID
ScanForNumber(
    IN WCHAR *Place,
    OUT ULONG *Value,
    OUT BOOLEAN *Found
    )

/*++

Routine Description:

    This routine does a sscanf(Place, "%d", Value) on a unicode string.

Arguments:

    Place - String to read from

    Value - Pointer to place to store the result.

    Found - Pointer to tell if the routine failed to find an integer.

Return Value:

    None.

--*/


{
    ULONG Tmp;

    *Value = 0;
    *Found = FALSE;

    //
    // Skip leading blanks
    //

    while (*Place == L' ') {

        Place++;

    }


    //
    // Is this a hex number?
    //

    if ((Place[0] == L'0') &&
        (Place[1] == L'x')) {

        //
        // Yes, parse it as a hex number
        //

        *Found = TRUE;

        //
        // Skip leading '0x'
        //

        Place += 2;

        //
        // Convert a hex number
        //

        while (TRUE) {

            if ((*Place >= L'0') && (*Place <= L'9')) {

                Tmp = ((ULONG)*Place) - ((ULONG)L'0');

            } else {

                switch (*Place) {

                    case L'a':
                    case L'A':

                        Tmp = 10;
                        break;

                    case L'b':
                    case L'B':

                        Tmp = 11;
                        break;

                    case L'c':
                    case L'C':

                        Tmp = 12;
                        break;

                    case L'd':
                    case L'D':

                        Tmp = 13;
                        break;

                    case L'e':
                    case L'E':

                        Tmp = 14;
                        break;

                    case L'f':
                    case L'F':

                        Tmp = 15;
                        break;

                    default:

                        return;

                }

            }

            (*Value) *= 16;
            (*Value) += Tmp;

            Place++;

        }

    } else if ((*Place >= L'0') && (*Place <= L'9')) {

        //
        // Parse it as an int
        //

        *Found = TRUE;

        //
        // Convert a base 10 number
        //

        while (TRUE) {

            if ((*Place >= L'0') && (*Place <= L'9')) {

                Tmp = ((ULONG)*Place) - ((ULONG)L'0');

            } else {

                return;

            }

            (*Value) *= 10;
            (*Value) += Tmp;

            Place++;

        }

    }

}

BOOLEAN
CheckFor8390(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress
    )

/*++

Routine Description:

    This routine checks for the existence of an 8390 NIC.

Arguments:

    InterfaceType - Any bus type.

    BusNumber - The bus number of the bus in the system.

    IoBaseAddress - The IoBaseAddress to check.

Return Value:

    None.

--*/

{
    UCHAR Value;
    UCHAR IMRValue;
    NTSTATUS NtStatus;
    UCHAR SavedOffset0;
    UCHAR SavedOffset3;
    UCHAR SavedOffsetF;
    BOOLEAN Status = TRUE;

    //
    // If the IoBaseAddress is the address of the DMA register on the NE2000
    // adapter, then this routine will hang the card and the machine.  To avoid
    // this, we first write to the Ne2000's reset port.
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0xF,
                                   &SavedOffsetF
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Fail;

    }

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    0xFF
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Fail;

    }

    //
    // Write STOP bit
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress,
                                   &SavedOffset0
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore1;

    }

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore1;

    }

    //
    // Read boundary
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0x3,
                                   &Value
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore2;

    }

    SavedOffset3 = Value;

    //
    // Write a different boundary
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0x3,
                                    (UCHAR)(Value + 1)
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore2;

    }

    //
    // Did it stick?
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0x3,
                                   &IMRValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore3;

    }

    if (IMRValue != (UCHAR)(Value + 1)) {

        Status = FALSE;
        goto Restore3;

    }

    //
    // Write IMR
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    0x3F
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore3;

    }


    //
    // switch to page 2
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0xA1
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Read the IMR
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0xF,
                                   &IMRValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Remove bits added by NIC
    //

    IMRValue &= 0x3F;

    //
    // switch to page 0
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore3;

    }

    //
    // Write IMR
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    (UCHAR)(IMRValue)
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore3;

    }

    //
    // switch to page 1
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x61
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Write ~IMR
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    (UCHAR)(~IMRValue)
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // switch to page 2
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0xA1
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Read IMR
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0xF,
                                   &Value
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Are they the same?
    //

    if ((UCHAR)(Value & 0x3F) == (UCHAR)(IMRValue)) {

        Status = FALSE;

    }

    //
    // Change to page 0
    //

    DetectWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress,
                         0x21
                        );

Restore3:

    DetectWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress + 0x3,
                         SavedOffset3
                        );

Restore2:

    DetectWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress,
                         SavedOffset0
                        );

Restore1:

    DetectWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress + 0xF,
                         SavedOffsetF
                        );

Fail:

    return(Status);

}

VOID
Send8390Packet(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    IN ULONG MemoryBaseAddress,
    IN COPY_ROUTINE CardCopyDownBuffer,
    IN UCHAR *NetworkAddress
    )

/*++

Routine Description:

    This routine creates an interrupt on an 8390 chip.  The only way to
    do this is to actually transmit a packet.  So, we put the card in
    loopback mode and let 'er rip.

Arguments:

    InterfaceType - Any bus type.

    BusNumber - The bus number of the bus in the system.

    IoBaseAddress - The IoBaseAddress to check.

    MemoryBaseAddress - The MemoryBaseAddress (if applicable) to copy a p
    packet to for transmission.

    CardCopyDownBuffer - A routine for copying a packet onto a card.

    NetworkAddress - The network address of the machine.

Return Value:

    None.

--*/

{

#define TEST_LEN 60
#define MAGIC_NUM 0x92

    NTSTATUS NtStatus;

    UCHAR TestPacket[TEST_LEN] = {0};     // a dummy packet.

    memcpy(TestPacket, NetworkAddress, 6);
    memcpy(TestPacket+6, NetworkAddress, 6);
    TestPacket[12] = 0x00;
    TestPacket[13] = 0x00;
    TestPacket[TEST_LEN-1] = MAGIC_NUM;

    //
    // First construct TestPacket.
    //

    TestPacket[TEST_LEN-1] = MAGIC_NUM;

    //
    // Now copy down TestPacket and start the transmission.
    //

    (*CardCopyDownBuffer)(InterfaceType,
                          BusNumber,
                          IoBaseAddress,
                          MemoryBaseAddress,
                          TestPacket,
                          TEST_LEN
                         );

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0x4,
                                    (UCHAR)(MemoryBaseAddress >> 8)
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }


    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0x6,
                                    0x0
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }


    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0x5,
                                    (UCHAR)(TEST_LEN)
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x26
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }

    //
    // We pause here to allow the xmit to complete so that we can ACK
    // it below - leaving the card in a valid state.
    //

    {
        UCHAR i;
        UCHAR RegValue;

        for (i=0;i != 0xFF;i++) {

            //
            // check for send completion
            //

            NtStatus = DetectReadPortUchar(InterfaceType,
                                           BusNumber,
                                           IoBaseAddress + 0x7,
                                           &RegValue
                                          );


            if (NtStatus != STATUS_SUCCESS) {
                return;
            }

            if (RegValue & 0xA) {
                break;
            }

        }

    }

    //
    // Turn off any interrupts
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    0x00
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }

    //
    // Acknowledge any interrupts that are floating around.
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xE,
                                    0xFF
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }

    return;

}

PVOID
DetectAllocateHeap(
    IN ULONG Size
    )
{
    PVOID pv = malloc( Size ) ;
    if ( Size )
        memset( pv, 0, Size ) ;
    return pv ;
}

VOID
DetectFreeHeap(
    IN PVOID BaseAddress
    )
{
    free( BaseAddress ) ;
}



//  End of MSNCDET.C
