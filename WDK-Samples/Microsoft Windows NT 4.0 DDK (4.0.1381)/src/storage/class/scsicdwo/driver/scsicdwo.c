
/*++

Copyright (C) 1995-1996  Microsoft Corporation

Module Name:

    scsicdwo.c

Abstract:

    The CD Writer (Write Once) class driver translates IRPs to SRBs
    and sends them to its devices through the scsiport driver.
    (Adapted from scsicdrm\cdrom.c).

Author:

    Tom McGuire (tommcg)

Environment:

    kernel mode only

--*/

#include <ntddk.h>
#include <scsi.h>
#include <class.h>
#include <scsicdwo.h>

#define CDWO_POOL_TAG         'owdC'    // 'Cdwo'

#define IO_CDWRITER_INCREMENT      7    // high boost for io completion


typedef struct _CDWRITER_EXTENSION CDWRITER_EXTENSION, *PCDWRITER_EXTENSION;

struct _CDWRITER_EXTENSION {

    PSCSI_REQUEST_BLOCK Srb;            // NonPagedPool
    PSCSI_REQUEST_BLOCK SenseSrb;       // NonPagedPool
    PMDL                SenseMdl;       // IoAllocateMdl
    PVOID               VendorIdText;   // PagedPool
    ERESOURCE           DeviceControl;  // Assume device extension is NonPaged
    ERESOURCE_THREAD    OwningThread;
    BOOLEAN             Unloading;

    };

#define SIZEOF_FULL_EXTENSION (                             \
            (ULONG)((PDEVICE_EXTENSION)( NULL ) + 1 ) +     \
            sizeof( CDWRITER_EXTENSION )                    \
            )

#if DBG
    #define DBGPRINT( args ) DbgPrint args
#else
    #define DBGPRINT( args )
#endif


//
//  Initialization function prototypes.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
ScsiCdWriterFindWritersOnPort(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG          PortNumber
    );

NTSTATUS
ScsiCdWriterCreateDevice(
    IN PDRIVER_OBJECT        DriverObject,
    IN PDEVICE_OBJECT        PortDeviceObject,
    IN ULONG                 PortNumber,
    IN ULONG                 DeviceNumber,
    IN PIO_SCSI_CAPABILITIES PortCapabilities,
    IN PSCSI_INQUIRY_DATA    LunInfo
    );

//
//  Preceding function declarations are only called during initialization
//  of the driver, so they belong in the INIT code section so they can be
//  discarded after the DriverEntry function returns.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( INIT, ScsiCdWriterFindWritersOnPort )
#pragma alloc_text( INIT, ScsiCdWriterCreateDevice )
#endif


//
//  Pageable function prototypes.
//

VOID
ScsiCdWriterUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
ScsiCdWriterOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiCdWriterClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiCdWriterDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiCdWriterQueryParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiCdWriterExecute(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
//  Preceding function declarations are called from high-level io dispatch
//  routines, so they can be pageable (ie, never called at DPC+ level).
//

#ifdef ALLOC_PRAGMA

    #pragma alloc_text( PAGE, ScsiCdWriterUnload )
    #pragma alloc_text( PAGE, ScsiCdWriterOpen )
    #pragma alloc_text( PAGE, ScsiCdWriterClose )
    #pragma alloc_text( PAGE, ScsiCdWriterDeviceControl )
    #pragma alloc_text( PAGE, ScsiCdWriterQueryParameters )
    #pragma alloc_text( PAGE, ScsiCdWriterExecute )

#endif


//
//  Non-pageable function prototypes.
//

NTSTATUS
ScsiCdWriterIoCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
ScsiCdWriterRetryRequest(
    IN PDEVICE_OBJECT      DeviceObject,
    IN PIRP                Irp,
    IN PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
ScsiCdWriterCompleteRequest(
    IN PDEVICE_OBJECT      DeviceObject,
    IN PIRP                Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID               SenseData,
    IN ULONG               SenseDataLength
    );

NTSTATUS
ScsiCdWriterRequestSense(
    IN PDEVICE_OBJECT      DeviceObject,
    IN PIRP                OriginalIrp,
    IN PSCSI_REQUEST_BLOCK OriginalSrb
    );

NTSTATUS
ScsiCdWriterRequestSenseCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           SenseIrp,
    IN PVOID          Context
    );

BOOLEAN
ScsiCdWriterShouldRetryRequest(
    IN PDEVICE_OBJECT      DeviceObject,
    IN OUT PIRP            Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG               SenseDataLength,
    IN PWRITER_SENSE_DATA  SenseData
    );

//
//  Preceding function declarations are called in DPC context as part
//  of io completion routine processing, so they must remain in the
//  default "text" code section which is non-pageable.
//



ULONG ScsiCdWriterDeviceCount;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
    {
    WCHAR          DeviceNameBuffer[ 20 ];      // "\Device\ScsiPortXX"
    PDEVICE_OBJECT PortDeviceObject;
    PFILE_OBJECT   PortFileObject;
    UNICODE_STRING PortDeviceName;
    ULONG          PortNumber;
    ULONG          CdWritersFound;
    NTSTATUS       Status;

    PAGED_CODE();

    DBGPRINT(( "SCSICDWO: Init\n" ));

    //
    //  Scsi ports are numbered zero through however many exist.
    //  Start with zero, attempt to open that scsi port.  While
    //  the open succeeds, continue with next port number until
    //  the port open fails.  For each port that is successfully
    //  opened, find all the CD writers attached to that port.
    //

    PortNumber = 0;
    ScsiCdWriterDeviceCount = 0;

    do  {

        swprintf( DeviceNameBuffer, L"\\Device\\ScsiPort%d", PortNumber );

        RtlInitUnicodeString( &PortDeviceName, DeviceNameBuffer );

        Status = IoGetDeviceObjectPointer(
                     &PortDeviceName,
                     FILE_READ_ATTRIBUTES,
                     &PortFileObject,
                     &PortDeviceObject
                     );

        if ( NT_SUCCESS( Status )) {

            DBGPRINT(( "SCSICDWO: Opened ScsiPort%d\n", PortNumber ));

            ScsiCdWriterFindWritersOnPort(
                DriverObject,
                PortDeviceObject,
                PortNumber
                );

            ObDereferenceObject( PortFileObject );

            PortNumber++;

            }

        }
    while ( NT_SUCCESS( Status ));

    DBGPRINT(( "SCSICDWO: Found %d writers\n", ScsiCdWriterDeviceCount ));

    //
    //  If no writers found, unload with appropriate status.
    //

    if ( ScsiCdWriterDeviceCount == 0 ) {
        DBGPRINT(( "SCSICDWO: Unloading\n" ));
        return STATUS_NO_SUCH_DEVICE;
        }


    //
    //  Setup IRP handler functions for this driver and
    //  we're ready for business.
    //

    DriverObject->MajorFunction[ IRP_MJ_CREATE ]         = ScsiCdWriterOpen;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE ]          = ScsiCdWriterClose;
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = ScsiCdWriterDeviceControl;
    DriverObject->MajorFunction[ IRP_MJ_SCSI ]           = ScsiClassInternalIoControl;
    DriverObject->DriverUnload                           = ScsiCdWriterUnload;

    return STATUS_SUCCESS;
    }


NTSTATUS
ScsiCdWriterFindWritersOnPort(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG          PortNumber
    )
    {
    PIO_SCSI_CAPABILITIES   PortCapabilities;
    PSCSI_INQUIRY_DATA      LunInfo;
    PSCSI_ADAPTER_BUS_INFO  AdapterInfo;
    PINQUIRYDATA            InquiryData;
    ULONG                   ScsiBus;
    ULONG                   Offset;
    NTSTATUS                Status;

    PAGED_CODE();

    Status = ScsiClassGetCapabilities(
                 PortDeviceObject,
                 &PortCapabilities
                 );

    if ( ! NT_SUCCESS( Status )) {
        DBGPRINT(( "SCSICDWO: ScsiClassGetCapabilities failed (%08X)\n", Status ));
        return Status;
        }

    Status = ScsiClassGetInquiryData(
                 PortDeviceObject,
                 &AdapterInfo
                 );

    if ( ! NT_SUCCESS( Status )) {
        DBGPRINT(( "SCSICDWO: ScsiClassGetInquiryData failed (%08X)\n", Status ));
        return Status;
        }

    for ( ScsiBus = 0; ScsiBus < AdapterInfo->NumberOfBuses; ScsiBus++ ) {

        Offset = AdapterInfo->BusData[ ScsiBus ].InquiryDataOffset;

        while ( Offset != 0 ) {

            LunInfo = (PVOID)((PCHAR)AdapterInfo + Offset );

            if ( ! LunInfo->DeviceClaimed ) {

                InquiryData = (PVOID)LunInfo->InquiryData;

                //
                //  Now we have an unclaimed SCSI target, and we can decide
                //  if we want to claim it based on its DeviceType, VendorId,
                //  ProductId, and/or any field in the INQUIRYDATA structure
                //  defined in scsi.h (also defined in minitape.h).
                //

                if ( InquiryData->DeviceType == WRITE_ONCE_READ_MULTIPLE_DEVICE ) {     // 0x04

                    DBGPRINT((
                        "SCSICDWO: Claiming \"%-28.28s\" port%d bus%d path%d target%d lun%d\n",
                        InquiryData->VendorId,
                        PortNumber,
                        ScsiBus,
                        LunInfo->PathId,
                        LunInfo->TargetId,
                        LunInfo->Lun
                        ));

                    Status = ScsiCdWriterCreateDevice(
                                 DriverObject,
                                 PortDeviceObject,
                                 PortNumber,
                                 ScsiCdWriterDeviceCount,
                                 PortCapabilities,
                                 LunInfo
                                 );

                    if ( NT_SUCCESS( Status )) {
                        ScsiCdWriterDeviceCount++;
                        }
                    else {
                        DBGPRINT(( "SCSICDWO: Failed to create device (%08X)\n", Status ));
                        }
                    }

                else {

                    DBGPRINT((
                        "SCSICDWO: Unclaimed \"%-28.28s\" port%d bus%d path%d target%d lun%d type%xh\n",
                        InquiryData->VendorId,
                        PortNumber,
                        ScsiBus,
                        LunInfo->PathId,
                        LunInfo->TargetId,
                        LunInfo->Lun,
                        InquiryData->DeviceType
                        ));

                    }
                }

            Offset = LunInfo->NextInquiryDataOffset;

            }
        }

    return STATUS_SUCCESS;

    }


NTSTATUS
ScsiCdWriterCreateDevice(
    IN PDRIVER_OBJECT        DriverObject,
    IN PDEVICE_OBJECT        PortDeviceObject,
    IN ULONG                 PortNumber,
    IN ULONG                 DeviceNumber,
    IN PIO_SCSI_CAPABILITIES PortCapabilities,
    IN PSCSI_INQUIRY_DATA    LunInfo
    )
    {
    WCHAR               DeviceNameBuffer[ 20 ];     // "\Device\CdWriterXX"
    WCHAR               Win32NameBuffer[ 24 ];      // "\DosDevices\CdWriterXX"
    UNICODE_STRING      WriterDeviceName;
    UNICODE_STRING      Win32DeviceName;
    PDEVICE_OBJECT      WriterDeviceObject;
    PDEVICE_EXTENSION   ScsiDeviceExtension;
    PCDWRITER_EXTENSION WriterDeviceExtension;
    PWRITER_SENSE_DATA  SenseDataBuffer          = NULL;
    PSCSI_REQUEST_BLOCK SenseSrbBuffer           = NULL;
    PSCSI_REQUEST_BLOCK StaticSrbBuffer          = NULL;
    PMDL                SenseMdl                 = NULL;
    PVOID               VendorIdText             = NULL;
    BOOLEAN             DeviceClaimed            = FALSE;
    BOOLEAN             DeviceCreationSuccessful = FALSE;
    NTSTATUS            Status                   = STATUS_INSUFFICIENT_RESOURCES;

    PAGED_CODE();

    SenseDataBuffer = ExAllocatePoolWithTag(
                          NonPagedPoolCacheAligned,
                          CDWO_SENSE_MAX_LENGTH,
                          CDWO_POOL_TAG
                          );

    if ( SenseDataBuffer != NULL ) {

        SenseMdl = IoAllocateMdl(
                       SenseDataBuffer,
                       CDWO_SENSE_MAX_LENGTH,
                       FALSE,
                       FALSE,
                       NULL
                       );

        if ( SenseMdl != NULL ) {

            MmBuildMdlForNonPagedPool( SenseMdl );

            SenseSrbBuffer = ExAllocatePoolWithTag(
                                 NonPagedPool,
                                 sizeof( SCSI_REQUEST_BLOCK ),
                                 CDWO_POOL_TAG
                                 );

            if ( SenseSrbBuffer != NULL ) {

                StaticSrbBuffer = ExAllocatePoolWithTag(
                                      NonPagedPool,
                                      sizeof( SCSI_REQUEST_BLOCK ),
                                      CDWO_POOL_TAG
                                      );

                if ( StaticSrbBuffer != NULL ) {

                    VendorIdText = ExAllocatePoolWithTag(
                                       PagedPool,
                                       CDWO_TEXT_ID_LENGTH,
                                       CDWO_POOL_TAG
                                       );

                    if ( VendorIdText != NULL ) {

                        Status = ScsiClassClaimDevice(
                                     PortDeviceObject,
                                     LunInfo,
                                     FALSE,
                                     &PortDeviceObject
                                     );

                        if ( NT_SUCCESS( Status )) {

                            DeviceClaimed = TRUE;

                            swprintf(
                                DeviceNameBuffer,
                                L"\\Device\\CdWriter%d",
                                DeviceNumber
                                );

                            RtlInitUnicodeString(
                                &WriterDeviceName,
                                DeviceNameBuffer
                                );

                            Status = IoCreateDevice(
                                         DriverObject,
                                         SIZEOF_FULL_EXTENSION,
                                         &WriterDeviceName,
                                         FILE_DEVICE_CD_WRITER,
                                         FILE_WRITE_ONCE_MEDIA,
                                         TRUE, // exclusive
                                         &WriterDeviceObject
                                         );

                            if ( NT_SUCCESS( Status )) {

                                DeviceCreationSuccessful = TRUE;

                                }
                            }
                        }
                    }
                }
            }
        }

    if ( ! DeviceCreationSuccessful ) {

        //
        //  Something failed, so clean up whatever we did before the
        //  failure occurred and return Status.
        //

        ASSERT( ! NT_SUCCESS( Status ));

        if ( SenseMdl )
            IoFreeMdl( SenseMdl );

        if ( SenseDataBuffer )
            ExFreePool( SenseDataBuffer );

        if ( SenseSrbBuffer )
            ExFreePool( SenseSrbBuffer );

        if ( StaticSrbBuffer )
            ExFreePool( StaticSrbBuffer );

        if ( VendorIdText )
            ExFreePool( VendorIdText );

        if ( DeviceClaimed ) {

            //
            //  Release claimed device
            //

            ScsiClassClaimDevice(
                PortDeviceObject,
                LunInfo,
                TRUE,
                NULL
                );

            }

        return Status;

        }


    //
    //  If we get to here, the device creation was successful.
    //

    //
    //  Make symbolic link to Win32 device name so Win32 apps can open
    //  this device.  Note that if this fails, we do not fail the device
    //  creation, but merely DbgPrint that the symbolic link failed.
    //

    swprintf( Win32NameBuffer, L"\\DosDevices\\CdWriter%d", DeviceNumber );

    RtlInitUnicodeString( &Win32DeviceName, Win32NameBuffer );

    Status = IoCreateSymbolicLink( &Win32DeviceName, &WriterDeviceName );

    if ( ! NT_SUCCESS( Status )) {
        DBGPRINT(( "SCSICDWO: IoCreateSymbolicLink failed (%08X)\n", Status ));
        }

    //
    //  Now fill-in the device extension data and return success.
    //

    if ( PortDeviceObject->AlignmentRequirement > WriterDeviceObject->AlignmentRequirement ) {
        WriterDeviceObject->AlignmentRequirement = PortDeviceObject->AlignmentRequirement;
        }

    WriterDeviceObject->StackSize = PortDeviceObject->StackSize + 1;

    ScsiDeviceExtension = WriterDeviceObject->DeviceExtension;
    ScsiDeviceExtension->PhysicalDevice    = WriterDeviceObject;
    ScsiDeviceExtension->PortDeviceObject  = PortDeviceObject;
    ScsiDeviceExtension->PortCapabilities  = PortCapabilities;
    ScsiDeviceExtension->SrbFlags         |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                             SRB_FLAGS_DISABLE_AUTOSENSE |
                                             SRB_FLAGS_NO_QUEUE_FREEZE;
    ScsiDeviceExtension->PortNumber        = (UCHAR)PortNumber;
    ScsiDeviceExtension->PathId            = LunInfo->PathId;
    ScsiDeviceExtension->TargetId          = LunInfo->TargetId;
    ScsiDeviceExtension->Lun               = LunInfo->Lun;
    ScsiDeviceExtension->TimeOutValue      = CDWO_STANDARD_TIMEOUT;
    ScsiDeviceExtension->DeviceObject      = WriterDeviceObject;
    ScsiDeviceExtension->DeviceNumber      = DeviceNumber;

    WriterDeviceExtension = (PVOID)( ScsiDeviceExtension + 1 );
    WriterDeviceExtension->Srb          = StaticSrbBuffer;
    WriterDeviceExtension->SenseSrb     = SenseSrbBuffer;
    WriterDeviceExtension->SenseMdl     = SenseMdl;
    WriterDeviceExtension->Unloading    = FALSE;
    WriterDeviceExtension->VendorIdText = VendorIdText;

    RtlCopyMemory(
        VendorIdText,
        ((PINQUIRYDATA)LunInfo->InquiryData )->VendorId,
        CDWO_TEXT_ID_LENGTH
        );

    Status = ExInitializeResourceLite( &WriterDeviceExtension->DeviceControl );

    ASSERT( NT_SUCCESS( Status ));  // ExInitializeResourceLite never fails.


    //
    //  Initialize the static fields of the SenseSrb.
    //

    RtlZeroMemory( SenseSrbBuffer, CDWO_SENSE_MAX_LENGTH );
    SenseSrbBuffer->Length       = SCSI_REQUEST_BLOCK_SIZE;
    SenseSrbBuffer->Function     = SRB_FUNCTION_EXECUTE_SCSI;
    SenseSrbBuffer->PathId       = ScsiDeviceExtension->PathId;
    SenseSrbBuffer->TargetId     = ScsiDeviceExtension->TargetId;
    SenseSrbBuffer->Lun          = ScsiDeviceExtension->Lun;
    SenseSrbBuffer->CdbLength    = 6;
    SenseSrbBuffer->SrbFlags     = SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                                   SRB_FLAGS_DISABLE_AUTOSENSE      |
                                   SRB_FLAGS_NO_QUEUE_FREEZE        |
                                   SRB_FLAGS_DATA_IN;
    SenseSrbBuffer->DataBuffer   = SenseDataBuffer;
    SenseSrbBuffer->TimeOutValue = CDWO_STANDARD_TIMEOUT;
    SenseSrbBuffer->Cdb[ 0 ]     = SCSIOP_REQUEST_SENSE;
    SenseSrbBuffer->Cdb[ 1 ]     = ScsiDeviceExtension->Lun << 5;
    SenseSrbBuffer->Cdb[ 4 ]     = CDWO_SENSE_MAX_LENGTH;

    return STATUS_SUCCESS;
    }


VOID
ScsiCdWriterUnload(
    IN PDRIVER_OBJECT DriverObject
    )
    {
    WCHAR               Win32NameBuffer[ 24 ];
    UNICODE_STRING      Win32DeviceName;
    PDEVICE_OBJECT      ThisDeviceObject;
    PDEVICE_OBJECT      NextDeviceObject;
    PDEVICE_EXTENSION   ScsiDeviceExtension;
    PCDWRITER_EXTENSION WriterDeviceExtension;
    SCSI_INQUIRY_DATA   FakeLunInfo;
    NTSTATUS            Status;

    PAGED_CODE();

    DBGPRINT(( "SCSICDWO: Unloading\n" ));

    NextDeviceObject = DriverObject->DeviceObject;

    while ( NextDeviceObject != NULL ) {

        ThisDeviceObject = NextDeviceObject;

        NextDeviceObject = NextDeviceObject->NextDevice;

        ScsiDeviceExtension = ThisDeviceObject->DeviceExtension;
        WriterDeviceExtension = (PVOID)( ScsiDeviceExtension + 1 );

        WriterDeviceExtension->Unloading = TRUE;

        ExAcquireResourceExclusiveLite(
            &WriterDeviceExtension->DeviceControl,
            TRUE
            );

        WriterDeviceExtension->OwningThread = ExGetCurrentResourceThread();

        FakeLunInfo.PathId   = ScsiDeviceExtension->PathId;
        FakeLunInfo.TargetId = ScsiDeviceExtension->TargetId;
        FakeLunInfo.Lun      = ScsiDeviceExtension->Lun;

        Status = ScsiClassClaimDevice(
                     ScsiDeviceExtension->PortDeviceObject,
                     &FakeLunInfo,
                     TRUE,
                     NULL
                     );

        ASSERT( NT_SUCCESS( Status ));

        IoFreeMdl(  WriterDeviceExtension->SenseMdl );
        ExFreePool( WriterDeviceExtension->SenseSrb->DataBuffer );
        ExFreePool( WriterDeviceExtension->SenseSrb );
        ExFreePool( WriterDeviceExtension->Srb );
        ExFreePool( WriterDeviceExtension->VendorIdText );

        swprintf(
            Win32NameBuffer,
            L"\\DosDevices\\CdWriter%d",
            ScsiDeviceExtension->DeviceNumber
            );

        RtlInitUnicodeString( &Win32DeviceName, Win32NameBuffer );

        Status = IoDeleteSymbolicLink( &Win32DeviceName );

        ASSERT( NT_SUCCESS( Status ));

        Status = ExDeleteResourceLite( &WriterDeviceExtension->DeviceControl );

        ASSERT( NT_SUCCESS( Status ));

        IoDeleteDevice( ThisDeviceObject );

        }

    }


NTSTATUS
ScsiCdWriterOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
    {
    PAGED_CODE();
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
    }


NTSTATUS
ScsiCdWriterClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
    {
    PAGED_CODE();
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
    }


NTSTATUS
ScsiCdWriterDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
    {
    PIO_STACK_LOCATION  IrpStack;
    ULONG               Ioctl;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation( Irp );
    Ioctl    = IrpStack->Parameters.DeviceIoControl.IoControlCode;

#if DBG

    {
    PDEVICE_EXTENSION   ScsiDeviceExtension   = DeviceObject->DeviceExtension;
    PCDWRITER_EXTENSION WriterDeviceExtension = (PVOID)( ScsiDeviceExtension + 1 );

    ASSERT( ! WriterDeviceExtension->Unloading );
    }

#endif // DBG

    switch ( Ioctl ) {

        case IOCTL_CDWRITER_QUERY_PARAMETERS:

            return ScsiCdWriterQueryParameters( DeviceObject, Irp );

        case IOCTL_CDWRITER_CONTROL:

            return ScsiCdWriterExecute( DeviceObject, Irp );

        default:

            DBGPRINT(( "SCSICDWO: Passing IOCTL to ScsiClass!!!\n" ));
            return ScsiClassDeviceControl( DeviceObject, Irp );

        }
    }


NTSTATUS
ScsiCdWriterQueryParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
    {
    PIO_STACK_LOCATION   IrpStack;
    PDEVICE_EXTENSION    ScsiExtension;
    PCDWRITER_EXTENSION  WriterExtension;
    PCDWRITER_PARAMETERS UserBuffer;
    ULONG                UserLength;
    NTSTATUS             Status;

    PAGED_CODE();

    ScsiExtension   = DeviceObject->DeviceExtension;
    WriterExtension = (PVOID)( ScsiExtension + 1 );
    IrpStack        = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Called with METHOD_NEITHER, so careful touching user's buffers.
    //

    UserLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    UserBuffer = Irp->UserBuffer;

    Irp->IoStatus.Information = 0;      // in case of failure

    if (( UserLength < sizeof( CDWRITER_PARAMETERS )) ||
        ( UserBuffer->SizeOfCdWriterParameters != sizeof( CDWRITER_PARAMETERS ))) {
        Status = STATUS_INVALID_PARAMETER;
        }
    else {

        try {

            UserBuffer->ScsiPortNumber = ScsiExtension->PortNumber;
            UserBuffer->ScsiPathId     = ScsiExtension->PathId;
            UserBuffer->ScsiTargetId   = ScsiExtension->TargetId;
            UserBuffer->ScsiLun        = ScsiExtension->Lun;
            UserBuffer->BufferAlign    = DeviceObject->AlignmentRequirement;
            UserBuffer->BufferMaxSize  = ScsiExtension->PortCapabilities->MaximumTransferLength;
            UserBuffer->BufferMaxPages = ScsiExtension->PortCapabilities->MaximumPhysicalPages;
            UserBuffer->Terminator     = 0;

            RtlCopyMemory(
                UserBuffer->DeviceId,
                WriterExtension->VendorIdText,
                CDWO_TEXT_ID_LENGTH
                );

            //
            //  If we make it to here without taking an exception, success.
            //

            Irp->IoStatus.Information = sizeof( CDWRITER_PARAMETERS );
            Status = STATUS_SUCCESS;

            }

        except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();

            }

        }

    Irp->IoStatus.Status = Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;
    }


NTSTATUS
ScsiCdWriterExecute(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
    {
    PIO_STACK_LOCATION   IrpStack;
    PDEVICE_EXTENSION    ScsiExtension;
    PCDWRITER_EXTENSION  WriterExtension;
    PSCSI_REQUEST_BLOCK  Srb;
    PCDWRITER_CONTROL    UserRequestBuffer;
    ULONG                UserRequestLength;
    PVOID                UserTransferBuffer;
    ULONG                UserTransferLength;
    BOOLEAN              TransferDirectionToDevice;
    PMDL                 WriterControlMdl;
    BOOLEAN              WriterControlMdlLocked;
    PMDL                 TransferBufferMdl;
    BOOLEAN              TransferBufferMdlLocked;
    NTSTATUS             Status;

    PAGED_CODE();

    IrpStack        = IoGetCurrentIrpStackLocation( Irp );
    ScsiExtension   = DeviceObject->DeviceExtension;
    WriterExtension = (PVOID)( ScsiExtension + 1 );
    Srb             = WriterExtension->Srb;

    ExAcquireResourceExclusiveLite( &WriterExtension->DeviceControl, TRUE );
    WriterExtension->OwningThread = ExGetCurrentResourceThread();

    //
    //  Be careful touching user's buffers -- they have not been
    //  verified or locked down yet (METHOD_NEITHER).
    //

    Status = STATUS_INVALID_PARAMETER;

    ASSERT( Irp->MdlAddress == NULL );

    WriterControlMdl = NULL;
    WriterControlMdlLocked = FALSE;
    TransferBufferMdl = NULL;
    TransferBufferMdlLocked = FALSE;

    try {

        UserRequestBuffer = IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
        UserRequestLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

        if (( UserRequestLength < sizeof( CDWRITER_CONTROL )) ||
            ( UserRequestBuffer->SizeOfCdWriterControl != sizeof( CDWRITER_CONTROL ))) {

            leave;
            }

        //
        //  Since our IoCompletionRoutine will need to touch the user's
        //  request buffer to return status and sense information, and
        //  the IoCompletionRoutine runs at raised IRQL, we must lock
        //  down the user's request buffer.  However, we do not want the
        //  user's request buffer to be the Irp->MdlAddress as that is
        //  needed for the user's transfer buffer.
        //

        WriterControlMdl = IoAllocateMdl(
                               UserRequestBuffer,
                               UserRequestLength,
                               FALSE,   // not secondary
                               TRUE,    // charge quota
                               NULL     // no Irp association
                               );

        if ( WriterControlMdl == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
            }

        //
        //  MmProbeAndLockPages will raise an exception if it fails.
        //

        MmProbeAndLockPages(
            WriterControlMdl,
            UserMode,
            IoWriteAccess
            );

        WriterControlMdlLocked = TRUE;

        //
        //  Now the user's request buffer is locked, so we won't take any
        //  faults reading it, but keep in mind that another user thread in
        //  the same process could modify the information underneath use,
        //  so we still need to copy the parameters to local storage before
        //  validating and using them.
        //

        //
        //  Build Srb from user's data.
        //

        RtlZeroMemory( Srb, sizeof( SCSI_REQUEST_BLOCK ));

        UserTransferBuffer        = UserRequestBuffer->TransferBuffer;
        UserTransferLength        = UserRequestBuffer->TransferLength;
        TransferDirectionToDevice = UserRequestBuffer->WriteToDevice;
        Srb->TimeOutValue         = UserRequestBuffer->TimeoutSeconds;
        Srb->CdbLength            = UserRequestBuffer->CdbLength;

        if ( Srb->CdbLength > CDWO_CDB_MAX_LENGTH ) {
            leave;
            }

        RtlCopyMemory( Srb->Cdb, UserRequestBuffer->CdbData, Srb->CdbLength );

        RtlZeroMemory(
            &UserRequestBuffer->SenseData,
            sizeof( UserRequestBuffer->SenseData )
            );

        //
        //  Now we have user's input parameters in local storage.
        //

        if ( UserTransferLength == 0 ) {
            UserTransferBuffer = NULL;
            }
        else if ( UserTransferBuffer == NULL ) {
            UserTransferLength = 0;
            }

        //
        //  Now fill in the non-user (system) Srb data.
        //

        Srb->Length              = SCSI_REQUEST_BLOCK_SIZE;
        Srb->Function            = SRB_FUNCTION_EXECUTE_SCSI;
        Srb->PathId              = ScsiExtension->PathId;
        Srb->TargetId            = ScsiExtension->TargetId;
        Srb->Lun                 = ScsiExtension->Lun;
        Srb->Cdb[ 1 ]           &= 0x1F;           // mask  Lun bits  in Cdb
        Srb->Cdb[ 1 ]           |= Srb->Lun << 5;  // force Lun value in Cdb
        Srb->SrbFlags            = ScsiExtension->SrbFlags;
        Srb->DataTransferLength  = UserTransferLength;
        Srb->DataBuffer          = UserTransferBuffer;
        Srb->OriginalRequest     = Irp;

        if ( UserTransferBuffer == NULL ) {
            Srb->SrbFlags |= SRB_FLAGS_NO_DATA_TRANSFER;
            }
        else {

            Srb->SrbFlags |= TransferDirectionToDevice ?
                                 SRB_FLAGS_DATA_OUT :
                                 SRB_FLAGS_DATA_IN;

            //
            //  Check buffer alignment.
            //

            if ( (ULONG) UserTransferBuffer &
                 ScsiExtension->DeviceObject->AlignmentRequirement ) {

                leave;
                }

            //
            //  Make sure requested transfer size does not exceed
            //  underlying port driver's maximum transfer size.
            //

            if (( UserTransferLength >
                  ScsiExtension->PortCapabilities->MaximumTransferLength ) ||
                ( ADDRESS_AND_SIZE_TO_SPAN_PAGES( UserTransferBuffer,
                                                  UserTransferLength ) >
                  ScsiExtension->PortCapabilities->MaximumPhysicalPages )) {

               leave;
               }

            //
            //  Create an MDL for the user's transfer buffer and lock
            //  it down.  This MDL should be associated with the Irp as
            //  the primary MDL for the underlying scsiport driver.
            //

            TransferBufferMdl = IoAllocateMdl(
                                    UserTransferBuffer,
                                    UserTransferLength,
                                    FALSE,    // not secondary
                                    TRUE,     // charge quota
                                    Irp       // link to Irp
                                    );

            if ( TransferBufferMdl == NULL ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
                }

            ASSERT( Irp->MdlAddress == TransferBufferMdl );

            //
            //  MmProbeAndLockPages will raise an exception if it fails.
            //

            MmProbeAndLockPages(
                TransferBufferMdl,
                UserMode,
                TransferDirectionToDevice ? IoReadAccess : IoWriteAccess
                );

            TransferBufferMdlLocked = TRUE;

            }


        //
        //  Since the Irp's primary Mdl typically corresponds to the
        //  Irp->UserBuffer, we'd better make them agree.  Not sure
        //  if this is necessary.
        //

        Irp->UserBuffer = UserTransferBuffer;


        //
        //  Now setup our IrpStack location for our completion routine.
        //
        //  Note we'll use the 3rd and 4th generic arguments in our stack
        //  location to maintain context for this request.  By using these
        //  arguments, we're overwriting our Parameters.DeviceIoControl
        //  parameters, but we don't need those anymore.  The 3rd argument
        //  will store our WriterControlMdl for the completion routine to
        //  access, and the 4th argument will store our retry count.
        //

        IrpStack->MajorFunction = IRP_MJ_SCSI;
        IrpStack->Parameters.Scsi.Srb = Srb;
        IrpStack->Parameters.Others.Argument3 = WriterControlMdl;
        IrpStack->Parameters.Others.Argument4 = (PVOID) 0;

        IoSetCompletionRoutine(
            Irp,
            ScsiCdWriterIoCompletionRoutine,
            Srb,
            TRUE,
            TRUE,
            TRUE
            );

        //
        //  Now we need to setup parameters in the next Irp stack location,
        //  the one for the scsiport device.
        //

        IrpStack = IoGetNextIrpStackLocation( Irp );
        IrpStack->MajorFunction = IRP_MJ_SCSI;
        IrpStack->Parameters.Scsi.Srb = Srb;

        //
        //  Mark the Irp as pending, call the port driver, and return
        //  STATUS_PENDING.  Note that once we call IoCallDriver, we
        //  can no longer touch the Irp, and our IoCompletionRoutine
        //  will get called for Irp completion and do the cleanup.
        //

        IoMarkIrpPending( Irp );

        Status = IoCallDriver( ScsiExtension->PortDeviceObject, Irp );

        return STATUS_PENDING;

        }

    except( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();

        DBGPRINT(( "SCSICDWO: Exception %08X in ScsiCdWriterExecute\n", Status ));

        }

    //
    //  We get to here if anything failed.  Clean up anything that we
    //  allocated, complete the Irp, and return Status.
    //

    ASSERT( ! NT_SUCCESS( Status ));

    if ( TransferBufferMdl ) {
        if ( TransferBufferMdlLocked ) {
            MmUnlockPages( TransferBufferMdl );
            }
        IoFreeMdl( TransferBufferMdl );
        }

    if ( WriterControlMdl ) {
        if ( WriterControlMdlLocked ) {
            MmUnlockPages( WriterControlMdl );
            }
        IoFreeMdl( WriterControlMdl );
        }

    ExReleaseResourceForThreadLite(
        &WriterExtension->DeviceControl,
        WriterExtension->OwningThread
        );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;

    }


NTSTATUS
ScsiCdWriterIoCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
    {
    PSCSI_REQUEST_BLOCK Srb = Context;
    PIO_STACK_LOCATION  IrpStack = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_EXTENSION   ScsiExtension;
    PCDWRITER_EXTENSION WriterExtension;
    NTSTATUS            Status;
    BOOLEAN             Retry;

    ASSERT( IrpStack->Parameters.Scsi.Srb == Srb );
    ASSERT( IrpStack->Control & SL_PENDING_RETURNED );

    if ( SRB_STATUS( Srb->SrbStatus ) != SRB_STATUS_SUCCESS ) {

        DBGPRINT((
            "SCSICDWO: Request failed, SrbStatus=%02X, ScsiStatus=%02X\n",
            Srb->SrbStatus,
            Srb->ScsiStatus
            ));

        //
        //  Scsi request failed.  If scsi returned CHECK CONDITION status,
        //  then we need to issue a REQUEST SENSE command.  When the
        //  REQUEST SENSE command completes, it will determine whether
        //  or not to retry this original request or complete it as a
        //  failure.  If it retries the original request, the original
        //  request will return to this completion routine upon completion.
        //

        if ( Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION ) {

            DBGPRINT(( "SCSICDWO: CHECK_CONDITION returned\n" ));

            return ScsiCdWriterRequestSense( DeviceObject, Irp, Srb );

            }

        //
        //  The scsi request failed with some status other than
        //  CHECK CONDITION.  Determine if retry without sense.
        //

        ASSERT( ! NT_SUCCESS( Irp->IoStatus.Status ));

        Retry = ScsiCdWriterShouldRetryRequest(
                    DeviceObject,
                    Irp,
                    Srb,
                    0,
                    NULL
                    );

        if ( Retry ) {

            return ScsiCdWriterRetryRequest( DeviceObject, Irp, Srb );

            }

        }

    else {

        ASSERT( NT_SUCCESS( Irp->IoStatus.Status ));

        }

    //
    //  Successful request completion (or fall through from failure after
    //  maximum retries).  No sense information.
    //

    return ScsiCdWriterCompleteRequest( DeviceObject, Irp, Srb, NULL, 0 );

    }


NTSTATUS
ScsiCdWriterRequestSense(
    IN PDEVICE_OBJECT      DeviceObject,
    IN PIRP                OriginalIrp,
    IN PSCSI_REQUEST_BLOCK OriginalSrb
    )
    {
    PDEVICE_EXTENSION   ScsiExtension = DeviceObject->DeviceExtension;
    PCDWRITER_EXTENSION WriterExtension = (PVOID)( ScsiExtension + 1 );
    PSCSI_REQUEST_BLOCK SenseSrb = WriterExtension->SenseSrb;
    PIRP                SenseIrp;
    PMDL                Mdl;
    PIO_STACK_LOCATION  SenseIrpStack;
    NTSTATUS            Status;

    SenseIrp = IoAllocateIrp( DeviceObject->StackSize, FALSE );

    if ( SenseIrp == NULL ) {

        //
        //  Could not allocate Irp for request sense, so complete the
        //  original request as a failure without sense data.
        //

        ASSERT( ! NT_SUCCESS( OriginalIrp->IoStatus.Status ));

        return ScsiCdWriterCompleteRequest(
                   DeviceObject,
                   OriginalIrp,
                   OriginalSrb,
                   NULL,
                   0
                   );

        }

    //
    //  We have a static Mdl already setup for the static SenseData buffer.
    //

    SenseIrp->MdlAddress = WriterExtension->SenseMdl;

    //
    //  Zero-out the sense data buffer.
    //

    RtlZeroMemory( SenseSrb->DataBuffer, CDWO_SENSE_MAX_LENGTH );

    //
    //  Save the OriginalSrb and OriginalIrp in our Irp stack location.
    //  Also save our DeviceObject pointer.
    //

    IoSetNextIrpStackLocation( SenseIrp );
    SenseIrpStack = IoGetCurrentIrpStackLocation( SenseIrp );
    SenseIrpStack->Parameters.Scsi.Srb = OriginalSrb;
    SenseIrpStack->Parameters.Others.Argument2 = OriginalIrp;
    SenseIrpStack->DeviceObject = DeviceObject;

    //
    //  Setup parameters for port driver.
    //

    SenseIrpStack = IoGetNextIrpStackLocation( SenseIrp );
    SenseIrpStack->MajorFunction = IRP_MJ_SCSI;
    SenseIrpStack->Parameters.Scsi.Srb = WriterExtension->SenseSrb;

    //
    //  Set our completion routine.
    //

    IoSetCompletionRoutine(
        SenseIrp,
        ScsiCdWriterRequestSenseCompletion,
        SenseSrb,
        TRUE,
        TRUE,
        TRUE
        );

    //
    //  Initialize the dynamic fields of the SenseSrb.  The static
    //  fields are already initialized.  Note that we always request
    //  the full size of our sense buffer, so we expect underrun.
    //

    SenseSrb->SrbStatus = 0;
    SenseSrb->ScsiStatus = 0;
    SenseSrb->DataTransferLength = CDWO_SENSE_MAX_LENGTH;
    SenseSrb->OriginalRequest = SenseIrp;

    //
    //  Call the port driver and return.  Our completion routine will
    //  get control when the request completes.
    //

    IoCallDriver( ScsiExtension->PortDeviceObject, SenseIrp );

    return STATUS_MORE_PROCESSING_REQUIRED;

    }




NTSTATUS
ScsiCdWriterRequestSenseCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           SenseIrp,
    IN PVOID          Context
    )
    {
    PSCSI_REQUEST_BLOCK SenseSrb = Context;
    PIO_STACK_LOCATION  SenseIrpStack = IoGetCurrentIrpStackLocation( SenseIrp );
    PDEVICE_EXTENSION   ScsiExtension;
    PCDWRITER_EXTENSION WriterExtension;
    PSCSI_REQUEST_BLOCK OriginalSrb;
    PIRP                OriginalIrp;
    PWRITER_SENSE_DATA  SenseData;
    ULONG               SenseDataLength;
    BOOLEAN             Retry;
    NTSTATUS            Status;

    if ( DeviceObject == NULL ) {
        DBGPRINT(( "SCSICDWO: Null DeviceObject\n" ));
        DeviceObject = SenseIrpStack->DeviceObject;
        }

    ASSERT( DeviceObject != NULL );

    //
    //  Get our original Irp/Srb parameters off our Irp stack location.
    //

    OriginalSrb = SenseIrpStack->Parameters.Scsi.Srb;
    OriginalIrp = SenseIrpStack->Parameters.Others.Argument2;

    //
    //  We no longer need the SenseIrp.
    //

    IoFreeIrp( SenseIrp );

    if (( SRB_STATUS( SenseSrb->SrbStatus ) == SRB_STATUS_SUCCESS )      ||
        ( SRB_STATUS( SenseSrb->SrbStatus ) == SRB_STATUS_DATA_OVERRUN )) {

        //
        //  Request sense succeeded.
        //

        SenseDataLength = SenseSrb->DataTransferLength;
        SenseData = SenseSrb->DataBuffer;

        DBGPRINT(( "SCSICDWO: Request sense succeeded\n" ));
        DBGPRINT((
            "SCSICDWO: SenseLength=%2X, SenseKey=%2X, AddtlCode=%2X\n",
            SenseDataLength,
            SenseData->SenseKey,
            SenseData->AdditionalSenseCode
            ));

        }

    else {

        //
        //  Request sense failed.
        //

        SenseDataLength = 0;
        SenseData = NULL;

        DBGPRINT((
            "SCSICDWO: Request Sense failed:\n"
            "SCSICDWO:     Original Srb:\n"
            "SCSICDWO:         SrbStatus:  %02X\n"
            "SCSICDWO:         ScsiStatus: %02X\n"
            "SCSICDWO:     Sense Srb:\n"
            "SCSICDWO:         SrbStatus:  %02X\n"
            "SCSICDWO:         ScsiStatus: %02X\n",
            OriginalSrb->SrbStatus,
            OriginalSrb->ScsiStatus,
            SenseSrb->SrbStatus,
            SenseSrb->ScsiStatus
            ));

        }


    //
    //  Determine if need to retry the original request.
    //

    Retry = ScsiCdWriterShouldRetryRequest(
                DeviceObject,
                OriginalIrp,
                OriginalSrb,
                SenseDataLength,
                SenseData
                );

    if ( Retry ) {

        ScsiCdWriterRetryRequest( DeviceObject, OriginalIrp, OriginalSrb );

        }

    else {

        //
        //  Complete this request.
        //

        if ( SenseData ) {
            OriginalSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        ScsiCdWriterCompleteRequest(
            DeviceObject,
            OriginalIrp,
            OriginalSrb,
            SenseData,
            SenseDataLength
            );

        }

    return STATUS_MORE_PROCESSING_REQUIRED;

    }


NTSTATUS
ScsiCdWriterRetryRequest(
    IN PDEVICE_OBJECT      DeviceObject,
    IN PIRP                Irp,
    IN PSCSI_REQUEST_BLOCK Srb
    )
    {
    PIO_STACK_LOCATION  IrpStack = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_EXTENSION   ScsiExtension = DeviceObject->DeviceExtension;
    NTSTATUS            Status;

    ASSERT( IrpStack->Parameters.Scsi.Srb == Srb );
    ASSERT( IrpStack->Control & SL_PENDING_RETURNED );

    DBGPRINT(( "SCSICDWO: Retrying request\n" ));

    //
    //  Reset Srb fields for retrying the request.
    //

    if ( Irp->MdlAddress ) {
        Srb->DataTransferLength = Irp->MdlAddress->ByteCount;
        }
    else {
        ASSERT( Srb->DataTransferLength == 0 );
        }

    Srb->SrbStatus = 0;
    Srb->ScsiStatus = 0;

    //
    //  Most other SCSI class drivers retry failed requests with
    //  SRB_FLAGS_DISABLE_DISCONNECT, but since some writer commands
    //  take over a minute to complete, and disabling disconnect will
    //  prevent other SCSI activity on the same SCSI bus during that time,
    //  we should probably not force the SRB_FLAGS_DISABLE_DISCONNECT.
    //
    //  Srb->SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT;
    //

    //
    //  Increment retry count.
    //

    ((ULONG)( IrpStack->Parameters.Others.Argument4 ))++;

    //
    //  Reinitialize port driver's Irp parameters.
    //

    IrpStack = IoGetNextIrpStackLocation( Irp );

    IrpStack->MajorFunction = IRP_MJ_SCSI;
    IrpStack->Parameters.Scsi.Srb = Srb;

    //
    //  Reset our completion routine.
    //

    IoSetCompletionRoutine(
        Irp,
        ScsiCdWriterIoCompletionRoutine,
        Srb,
        TRUE,
        TRUE,
        TRUE
        );

    //
    //  Call the port driver.
    //

    IoCallDriver( ScsiExtension->PortDeviceObject, Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;

    }


NTSTATUS
ScsiCdWriterCompleteRequest(
    IN PDEVICE_OBJECT      DeviceObject,
    IN PIRP                Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID               SenseData,
    IN ULONG               SenseDataLength
    )
    {
    PDEVICE_EXTENSION   ScsiExtension = DeviceObject->DeviceExtension;
    PCDWRITER_EXTENSION WriterExtension = (PVOID)( ScsiExtension + 1 );
    PIO_STACK_LOCATION  IrpStack = IoGetCurrentIrpStackLocation( Irp );
    PCDWRITER_CONTROL   UserRequestBuffer;
    PMDL                WriterControlMdl;

    ASSERT( IrpStack->Parameters.Scsi.Srb == Srb );
    ASSERT( IrpStack->Control & SL_PENDING_RETURNED );

    //
    //  Copy the return status and sense information to the user's request
    //  buffer (it's locked down), then free up our allocated Mdls and
    //  complete the Irp.  Note we're not running in request thread's
    //  context, so must map system pages to the user's buffer.
    //

    WriterControlMdl = IrpStack->Parameters.Others.Argument3;

    ASSERT( WriterControlMdl != NULL );

    UserRequestBuffer = (PVOID)((ULONG)
                        MmGetSystemAddressForMdl( WriterControlMdl ) |
                        WriterControlMdl->ByteOffset );

    UserRequestBuffer->NtStatus       = Irp->IoStatus.Status;
    UserRequestBuffer->ScsiStatus     = Srb->ScsiStatus;
    UserRequestBuffer->SrbStatus      = Srb->SrbStatus;
    UserRequestBuffer->TransferLength = Srb->DataTransferLength;
    UserRequestBuffer->SenseLength    = (UCHAR)SenseDataLength;

    if ( SenseData && SenseDataLength ) {

        RtlCopyMemory(
            &UserRequestBuffer->SenseData,
            SenseData,
            SenseDataLength
            );

        }

    ASSERT( Irp->IoStatus.Information == Srb->DataTransferLength );

    //
    //  We're done writing the user's status information.  Unlock and
    //  release the Mdls for the user's buffers.
    //

    if ( Irp->MdlAddress ) {
        MmUnlockPages( Irp->MdlAddress );
        IoFreeMdl( Irp->MdlAddress );
        Irp->MdlAddress = NULL;
        }

    MmUnlockPages( WriterControlMdl );
    IoFreeMdl( WriterControlMdl );

    ASSERT( Irp->IoStatus.Status != STATUS_PENDING );
    ASSERT( Irp->IoStatus.Status != STATUS_MORE_PROCESSING_REQUIRED );

    //
    //  On completion, give the requesting thread a big boost to allow
    //  it to issue the next write operation since writer throughput is
    //  essential to preventing a failed write session.
    //

    IoCompleteRequest( Irp, IO_CDWRITER_INCREMENT );

    ExReleaseResourceForThreadLite(
        &WriterExtension->DeviceControl,
        WriterExtension->OwningThread
        );

    return STATUS_MORE_PROCESSING_REQUIRED;

    }


BOOLEAN
ScsiCdWriterShouldRetryRequest(
    IN PDEVICE_OBJECT      DeviceObject,
    IN OUT PIRP            Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG               SenseDataLength,
    IN PWRITER_SENSE_DATA  SenseData
    )
    {
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation( Irp );
    BOOLEAN            Retry;
    NTSTATUS           Status;

    Status = STATUS_IO_DEVICE_ERROR;
    Retry  = FALSE;

    //
    //  Note that we're only interested in conditions that will
    //  ALWAYS qualify for retry here.  If there's any failure that does
    //  not ALWAYS qualify as a retry-able condition, we'll just return
    //  a generic STATUS_IO_DEVICE_ERROR and let the user app determine
    //  from the sense data what is to be done (app may decide to retry).
    //
    //  Note that SenseData could also be used to determine retry here,
    //  but we're not using it for now.
    //

    UNREFERENCED_PARAMETER( SenseDataLength );
    UNREFERENCED_PARAMETER( SenseData );

    switch( SRB_STATUS( Srb->SrbStatus )) {

        case SRB_STATUS_DATA_OVERRUN:

            //
            //  This really indicates success, so just mark it successful
            //  and return so it will be completed as a success.
            //

            Status = STATUS_SUCCESS;
            break;

        case SRB_STATUS_BUSY:
        case SRB_STATUS_PHASE_SEQUENCE_FAILURE:
        case SRB_STATUS_UNEXPECTED_BUS_FREE:
        case SRB_STATUS_PARITY_ERROR:
        case SRB_STATUS_MESSAGE_REJECTED:
        case SRB_STATUS_SELECTION_TIMEOUT:

            Retry = TRUE;
            break;

        default:

            //
            //  Other failures, such as SRB_STATUS_BUS_RESET, we'll
            //  just fail because the writer will choke on a bus
            //  reset anyway if it is in writing mode.
            //

            break;  // do nothing, fall through to return failure

        }

    //
    //  If we've exceeded the MAX_RETRY number of retries for this Irp,
    //  don't retry it again.
    //

    if (( Retry ) &&
        ((ULONG)( IrpStack->Parameters.Others.Argument4 ) >= CDWO_MAX_RETRY )) {

        DBGPRINT(( "SCSICDWO: Max retries exceeded\n" ));

        Retry = FALSE;

        }

    if ( ! Retry ) {

        DBGPRINT((
            "SCSICDWO: Failure status was %08X, changing to %08X\n",
            Irp->IoStatus.Status,
            Status
            ));

        Irp->IoStatus.Status = Status;
        }

    return Retry;
    }




