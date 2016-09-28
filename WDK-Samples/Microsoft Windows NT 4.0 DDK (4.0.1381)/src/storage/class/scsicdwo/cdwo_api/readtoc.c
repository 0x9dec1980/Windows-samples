
/*++

Copyright (C) 1995-1996  Microsoft Corporation

Module Name:

    readtoc.c

Abstract:

    Sample application for demonstrating user-mode interface to kernel-mode
    scsicdwo driver for writing CDs.  This sample application only demonstrates
    the interface and does not demonstrate the steps necessary to write a CD
    which vary between CD writer devices.

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


void GetWriterParameters( HANDLE DeviceHandle );
void ReadTableOfContents( HANDLE DeviceHandle );
ULONG BigToLittleEndian( ULONG BigEndianValue );


void __cdecl main( void ) {

    ULONG  WriterNumber;
    HANDLE DeviceHandle;

    printf( "\n" );

    for ( WriterNumber = 0;; WriterNumber++ ) {

        DeviceHandle = CdWriterOpen( WriterNumber );

        if ( DeviceHandle == NULL )     // assume no more CdWriter devices
            break;

        printf( "CD Writer %d\n", WriterNumber );

        GetWriterParameters( DeviceHandle );

        ReadTableOfContents( DeviceHandle );

        CloseHandle( DeviceHandle );

        }

    if ( WriterNumber == 0 ) {

        printf( "No CD Writers found.  Is scsicdwo.sys driver running?\n" );

        }
    }


void GetWriterParameters( HANDLE DeviceHandle ) {

    CDWRITER_PARAMETERS WriterParams;
    CDWO_API_STATUS     Status;

    ZeroMemory( &WriterParams, sizeof( CDWRITER_PARAMETERS ));

    WriterParams.SizeOfCdWriterParameters = sizeof( CDWRITER_PARAMETERS );

    Status = CdWriterQueryParameters( DeviceHandle, &WriterParams );

    if ( Status == CDWO_API_STATUS_SUCCESS ) {

        printf(
            "  Text Description: \"%s\"\n",
            WriterParams.DeviceId
            );

        printf(
            "  Scsi Port %d, Bus %d, Target Id %d, Lun %d\n",
            WriterParams.ScsiPortNumber,
            WriterParams.ScsiPathId,
            WriterParams.ScsiTargetId,
            WriterParams.ScsiLun
            );
        }

    else {

        printf( "  Failed to query writer parameters, error %d\n", GetLastError() );

        }
    }


void ReadTableOfContents( HANDLE DeviceHandle ) {

    CDWRITER_CONTROL WriterControl;
    CDWO_API_STATUS  Status;
    PUCHAR           TocBuffer;
    ULONG            TocLength;
    ULONG            NumberOfTracks;
    PUCHAR           TrackRecord;
    ULONG            AbsoluteAddress;
    ULONG            Minutes;
    ULONG            Seconds;
    ULONG            Frames;
    ULONG            i;

    TocLength = 804;    // 100 tracks, 8 bytes each, plus 4 byte header

    //
    //  Use VirtualAlloc versus HeapAlloc or stack-based buffer to guarantee
    //  proper buffer alignment for SCSI transfer.
    //

    TocBuffer = VirtualAlloc( NULL, TocLength, MEM_COMMIT, PAGE_READWRITE );

    if ( TocBuffer == NULL ) {
        printf( "Out of memory\n" );
        return;
        }

    ZeroMemory( &WriterControl, sizeof( CDWRITER_CONTROL ));

    WriterControl.SizeOfCdWriterControl = sizeof( CDWRITER_CONTROL );
    WriterControl.CdbLength      = 10;
    WriterControl.CdbData[ 0 ]   = 0x43;        // READ DISC TABLE OF CONTENTS
    WriterControl.CdbData[ 7 ]   = (UCHAR)( TocLength >> 8 );  // MSB
    WriterControl.CdbData[ 8 ]   = (UCHAR)( TocLength );       // LSB
    WriterControl.WriteToDevice  = FALSE;       // reading versus writing
    WriterControl.TimeoutSeconds = 30;
    WriterControl.TransferLength = TocLength;
    WriterControl.TransferBuffer = TocBuffer;   // must be suitably aligned

    Status = CdWriterExecuteCommand( DeviceHandle, &WriterControl );

    if ( Status == CDWO_API_STATUS_SUCCESS ) {

        TocLength = WriterControl.TransferLength;   // actual length

        NumberOfTracks = ( TocLength - 4 ) / 8;

        for ( i = 0; i < NumberOfTracks; i++ ) {

            TrackRecord = TocBuffer + 4 + ( i * 8 );

            AbsoluteAddress = BigToLittleEndian( *(UNALIGNED ULONG*)( TrackRecord + 4 ));

            Minutes = ( AbsoluteAddress / ( 60 * 75 ));
            Seconds = ( AbsoluteAddress / 75 ) % 60;
            Frames  = ( AbsoluteAddress % 75 );

            printf(
                "  Track 0x%02X  Control 0x%X  Start Address 0x%08X (%02d:%02d:%02d)\n",
                *( TrackRecord + 2 ),
                *( TrackRecord + 1 ) & 0x0F,
                AbsoluteAddress,
                Minutes,
                Frames,
                Seconds
                );

            }

        printf( "\n" );

        }

    else {

        printf( "  Failed to read disc table of contents.\n" );

        if ( Status & CDWO_API_STATUS_SENSE_DATA_VALID ) {

            printf( "  SenseKey  = 0x%02X\n", WriterControl.SenseData.SenseKey );
            printf( "  SenseCode = 0x%02X\n", WriterControl.SenseData.AdditionalSenseCode );

            }

        else {

            printf(
                "  Scsi Status = 0x%02X (%s)\n",
                WriterControl.ScsiStatus,
                TranslateScsiStatus( WriterControl.ScsiStatus )
                );

            printf(
                "  Srb Status  = 0x%02X (%s)\n",
                WriterControl.SrbStatus,
                TranslateSrbStatus( WriterControl.SrbStatus )
                );

            }
        }

    VirtualFree( TocBuffer, 0, MEM_RELEASE );
    }



ULONG
BigToLittleEndian(
    ULONG BigEndianValue
    )
    {
    union {
        ULONG ulong;
        UCHAR uchar[ 4 ];
        } Source, Target;

    Source.ulong = BigEndianValue;

    Target.uchar[ 0 ] = Source.uchar[ 3 ];
    Target.uchar[ 1 ] = Source.uchar[ 2 ];
    Target.uchar[ 2 ] = Source.uchar[ 1 ];
    Target.uchar[ 3 ] = Source.uchar[ 0 ];

    return Target.ulong;
    }

