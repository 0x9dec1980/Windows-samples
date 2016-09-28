/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Detect.c

Abstract:

    This is the main file for the autodetection DLL for all the net cards
    which MS is shipping with Windows NT.

--*/

#include <ntddk.h>
#include <ntddnetd.h>

#include <windef.h>

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_ADVAPI32_)
#define WINADVAPI DECLSPEC_IMPORT
#else
#define WINADVAPI
#endif

//  FUDGE the definition of LPSECURITY_ATTRIBUTES
typedef void * LPSECURITY_ATTRIBUTES ;

//
//  File System time stamps are represented with the following structure:
//

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

//  Prototype "borrowed" from WINUSER.H
extern int WINAPIV wsprintfW(LPWSTR, LPCWSTR, ...);
//  Prototype "borrowed" from WINBASE.H
VOID WINAPI Sleep( DWORD dwMilliseconds );

#include <winreg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "detect.h"


#if DBG
#define STATIC
#else
#define STATIC static
#endif


#define MAX_BUS_INFO_SIZE  0x4000   //  16kb

STATIC
BOOLEAN
GetBusTypeKey(
    IN  ULONG BusNumber,
    const CHAR * BusTypeName,
    INT InterfaceType,
    OUT PVOID *InfoHandle
    )

/*++

Routine Description:

    This routine finds the Microchannel bus with BusNumber in the
    registry and returns a handle for the config information.

Arguments:

    BusNumber     - The bus number of the bus to search for.
    BusTypeName   - String name of bus type to search for
    InterfaceType - Interface type numeric id.
    InfoHandle    - The resulting root in the registry.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/
{
    static PSTR BusTypePathBase = "Hardware\\Description\\System\\";
    static PSTR ConfigData = "Configuration Data";
    CHAR BusTypePath [MAX_PATH] ;
    PUCHAR BufferPointer = NULL ;
    char SubkeyName [MAX_PATH] ;
    PCM_FULL_RESOURCE_DESCRIPTOR FullResource;
    HKEY BusTypeHandle = NULL,
         BusHandle = NULL ;
    FILETIME LastWrite ;
    ULONG Index;
    DWORD Type,
          BufferSize,
          NameSize ;
    LONG err ;
    BOOL Result = FALSE ;

    *InfoHandle = NULL;

    if (BusNumber > 98)
        return FALSE;

    //
    // Open the root.
    //
    strcpy( BusTypePath, BusTypePathBase ) ;
    strcat( BusTypePath, BusTypeName );

    if ( err = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                           BusTypePath,
                           0,
                           KEY_READ,
                           & BusTypeHandle ) )
    {
        return FALSE ;
    }

    for ( Index = 0 ; ! Result ; Index++ )
    {
        if ( BufferPointer )
        {
            free( BufferPointer ) ;
            BufferPointer = NULL ;
        }
        if ( BusHandle )
        {
            RegCloseKey( BusHandle ) ;
            BusHandle = NULL ;
        }

        //
        // Enumerate through keys, searching for the proper bus number
        //
        NameSize = sizeof SubkeyName ;
        err = RegEnumKeyExA( BusTypeHandle,
                            Index,
                            SubkeyName,
                            & NameSize,
                            0,
                            NULL,
                            0,
                            & LastWrite ) ;
        if ( err )
        {
            break ;
        }

        //
        // Open the BusType root + Bus Number
        //

        err = RegOpenKeyExA( BusTypeHandle,
                            SubkeyName,
                            0,
                            KEY_READ,
                            & BusHandle ) ;
        if ( err )
        {
            continue ;
        }

        BufferPointer = (PUCHAR) malloc( BufferSize = MAX_BUS_INFO_SIZE ) ;
        if ( BufferPointer == NULL )
        {
            break ;
        }

        err = RegQueryValueExA( BusHandle,
                               ConfigData,
                               NULL,
                               & Type,
                               BufferPointer,
                               & BufferSize ) ;
        if ( err )
        {
            break ;
        }

        //
        // Search for our bus number and type
        //
        FullResource = (PCM_FULL_RESOURCE_DESCRIPTOR) BufferPointer;

        Result = FullResource->InterfaceType == InterfaceType
              && FullResource->BusNumber == BusNumber ;
    }

    if ( BusTypeHandle )
       RegCloseKey( BusTypeHandle ) ;
    if ( BusHandle )
       RegCloseKey( BusHandle ) ;

    if ( Result )
    {
        *InfoHandle = BufferPointer ;
    }
    else
    if ( BufferPointer )
    {
        free( BufferPointer ) ;
    }

    return Result ;

}



BOOLEAN
GetMcaKey(
    IN  ULONG BusNumber,
    OUT PVOID *InfoHandle
    )

/*++

Routine Description:

    This routine finds the Microchannel bus with BusNumber in the
    registry and returns a handle for the config information.

Arguments:

    BusNumber - The bus number of the bus to search for.

    InfoHandle - The resulting root in the registry.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/
{
    return GetBusTypeKey( BusNumber,
                          "MultifunctionAdapter",
                          MicroChannel,
                          InfoHandle ) ;
}


BOOLEAN
GetMcaPosId(
    IN  PVOID BusHandle,
    IN  ULONG SlotNumber,
    OUT PULONG PosId
    )

/*++

Routine Description:

    This routine returns the PosId of an adapter in SlotNumber of an MCA bus.

Arguments:

    BusHandle - Handle returned by GetMcaKey().

    SlotNumber - the desired slot number

    PosId - the PosId.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR FullResource;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    ULONG i;
    ULONG TotalSlots;
    PCM_MCA_POS_DATA PosData;

    FullResource = (PCM_FULL_RESOURCE_DESCRIPTOR) BusHandle ;
    ResourceList = &FullResource->PartialResourceList;

    //
    // Find the device-specific information, which is where the POS data is.
    //
    for ( i = 0 ;  i < ResourceList->Count; i++ )
    {
        if (ResourceList->PartialDescriptors[i].Type == CmResourceTypeDeviceSpecific)
            break;
    }

    if (i == ResourceList->Count) {

        //
        // Couldn't find device-specific information.
        //

        return FALSE;
    }

    TotalSlots = ResourceList->PartialDescriptors[i].u.DeviceSpecificData.DataSize;

    TotalSlots = TotalSlots / sizeof(CM_MCA_POS_DATA);

    if (SlotNumber <= TotalSlots) {

        PosData = (PCM_MCA_POS_DATA)(&ResourceList->PartialDescriptors[i+1]);
        PosData += (SlotNumber - 1);

        *PosId = PosData->AdapterId;
        return(TRUE);

    }

    return(FALSE);

}


VOID
DeleteMcaKey(
    IN PVOID BusHandle
    )

/*++

Routine Description:

    This routine frees resources associated with an MCA handle.

Arguments:

    BusHandle - Handle returned by GetMcaKey().

Return Value:

    None.

--*/
{
    free( BusHandle ) ;
}


BOOLEAN
GetEisaKey(
    IN  ULONG BusNumber,
    OUT PVOID *InfoHandle
    )

/*++

Routine Description:

    This routine finds the Eisa bus with BusNumber in the
    registry and returns a handle for the config information.

Arguments:

    BusNumber - The bus number of the bus to search for.

    InfoHandle - The resulting root in the registry.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/
{
    return GetBusTypeKey( BusNumber,
                          "EisaAdapter",
                          Eisa,
                          InfoHandle ) ;
}


BOOLEAN
GetEisaCompressedId(
    IN  PVOID BusHandle,
    IN  ULONG SlotNumber,
    OUT PULONG CompressedId
    )

/*++

Routine Description:

    This routine returns the PosId of an adapter in SlotNumber of an MCA bus.

Arguments:

    BusHandle - Handle returned by GetEisaKey().

    SlotNumber - the desired slot number

    CompressedId - EISA Id in the slot desired.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR FullResource;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    ULONG i;
    ULONG TotalDataSize;
    ULONG SlotDataSize;
    PCM_EISA_SLOT_INFORMATION SlotInformation;

    FullResource = (PCM_FULL_RESOURCE_DESCRIPTOR) BusHandle;
    ResourceList = &FullResource->PartialResourceList;

    //
    // Find the device-specific information, which is where the POS data is.
    //
    for (i=0; i<ResourceList->Count; i++) {
        if (ResourceList->PartialDescriptors[i].Type == CmResourceTypeDeviceSpecific) {
            break;
        }
    }

    if (i == ResourceList->Count) {

        //
        // Couldn't find device-specific information.
        //

        return FALSE;
    }


    //
    // Bingo!
    //

    ResourceDescriptor = &(ResourceList->PartialDescriptors[i]);

    TotalDataSize = ResourceDescriptor->u.DeviceSpecificData.DataSize;

    SlotInformation = (PCM_EISA_SLOT_INFORMATION)
                        ((PUCHAR)ResourceDescriptor +
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    while (((LONG)TotalDataSize) > 0) {

        if (SlotInformation->ReturnCode == EISA_EMPTY_SLOT) {

            SlotDataSize = sizeof(CM_EISA_SLOT_INFORMATION);

        } else {

            SlotDataSize = sizeof(CM_EISA_SLOT_INFORMATION) +
                      SlotInformation->NumberFunctions *
                      sizeof(CM_EISA_FUNCTION_INFORMATION);
        }

        if (SlotDataSize > TotalDataSize) {

            //
            // Something is wrong again
            //

            return FALSE;

        }

        if (SlotNumber != 0) {

            SlotNumber--;

            SlotInformation = (PCM_EISA_SLOT_INFORMATION)
                ((PUCHAR)SlotInformation + SlotDataSize);

            TotalDataSize -= SlotDataSize;

            continue;

        }

        //
        // This is our slot
        //

        break;

    }

    if ((SlotNumber != 0) || (TotalDataSize == 0)) {

        //
        // No such slot number
        //

        return(FALSE);

    }

    //
    // End loop
    //

    *CompressedId = SlotInformation->CompressedId & 0x00FFFFFF;

    return(TRUE);

}

VOID
DeleteEisaKey(
    IN PVOID BusHandle
    )

/*++

Routine Description:

    This routine frees resources associated with an EISA handle.

Arguments:

    BusHandle - Handle returned by GetEisaKey().

Return Value:

    None.

--*/
{
    free( BusHandle ) ;
}
