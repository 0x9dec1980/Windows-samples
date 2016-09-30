/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ramdisk.c

Abstract:

    This is the Ram Disk driver.

Author:

    Robert Nelson (RobertN) 10-Mar-1993.

Environment:

    Kernel mode only.

Notes:

Revision History:
    Added the IOCTL_DISK_GET_PARTITION_INFO query to make it work with NTFS
    driver loaded (thanks Robert Vierthaler (RobertVi)).

--*/


//
// Include files.
//

#include <ntddk.h>          // various NT definitions
#include <ntdddisk.h>       // disk device driver I/O control codes
#include <ntiologc.h>

#include <string.h>

#include "ramdisk.h"

#if DBG
ULONG   RamDiskDebugLevel = 0;
#endif


NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It fills in the dispatch entry points in the driver object.  Then
    RamDiskInitializeDisk is called to create the device object and complete
    the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if this disk is initialized; an error otherwise.

--*/

{
    NTSTATUS        ntStatus;
    UNICODE_STRING  paramPath;
    static  WCHAR   SubKeyString[] = L"\\Parameters";

    //
    // The registry path parameter points to our key, we will append
    // the Parameters key and look for any additional configuration items
    // there.  We add room for a trailing NUL for those routines which
    // require it.

    paramPath.MaximumLength = RegistryPath->Length + sizeof(SubKeyString);
    paramPath.Buffer = ExAllocatePool(PagedPool, paramPath.MaximumLength);

    if (paramPath.Buffer != NULL)
    {
        RtlMoveMemory(
            paramPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);

        RtlMoveMemory(
            &paramPath.Buffer[RegistryPath->Length / 2], SubKeyString,
            sizeof(SubKeyString));

        paramPath.Length = paramPath.MaximumLength;
    }
    else
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if DBG
    {
        //
        // We use this to query into the registry as to whether we
        // should break at driver entry.
        //
    
        RTL_QUERY_REGISTRY_TABLE    paramTable[3];
        ULONG                       zero = 0;
    
        ULONG                       debugLevel = 0;
        ULONG                       shouldBreak = 0;
    
        RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    
        paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name = L"BreakOnEntry";
        paramTable[0].EntryContext = &shouldBreak;
        paramTable[0].DefaultType = REG_DWORD;
        paramTable[0].DefaultData = &zero;
        paramTable[0].DefaultLength = sizeof(ULONG);
        
        paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name = L"DebugLevel";
        paramTable[1].EntryContext = &debugLevel;
        paramTable[1].DefaultType = REG_DWORD;
        paramTable[1].DefaultData = &zero;
        paramTable[1].DefaultLength = sizeof(ULONG);
        
        if (!NT_SUCCESS(RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            paramPath.Buffer, &paramTable[0], NULL, NULL)))
        {
            shouldBreak = 0;
            debugLevel = 0;
        }
        
        RamDiskDebugLevel = debugLevel;
        
        if (shouldBreak)
        {
            DbgBreakPoint();
        }
    }
#endif

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = RamDiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = RamDiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = RamDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = RamDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RamDiskDeviceControl;

//
// We'll enable unload when the Filesystem drivers support it
//
//  DriverObject->DriverUnload = RamDiskUnloadDriver;

    ntStatus = RamDiskInitializeDisk(DriverObject, &paramPath);

    //
    // We don't need that path anymore.
    //

    if (paramPath.Buffer)
    {
        ExFreePool( paramPath.Buffer );
    }

    return ntStatus;
}

NTSTATUS
RamDiskInitializeDisk(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  ParamPath
    )

/*++

Routine Description:
    This routine is called at initialization time by DriverEntry().

    It creates and initializes a device object for the disk.  Memory for the
    Ram Disk is allocated from the non-paged Pool.  RamDiskFormatFat is called
    to create a FAT filesystem in the memory buffer allocated.

    Two parameters may be configured using the registry.  DiskSize specifies
    the desired size (in bytes).  If the requested memory is not available in
    the nonpaged pool then STATUS_INSUFFICIENT_RESOURCES is returned.  The
    default value is 0x00100000 (1MB).

    DriveLetter is used to indicate the drive letter for the RamDisk.  The
    string must be a letter, optionally followed by :.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    ParamPath - a pointer to the Parameters key under our key in the Services
        section of the registry.

Return Value:
    STATUS_SUCCESS if this disk is initialized; an error otherwise.

--*/

{
    STRING              ntNameString;       // NT Device Name "\Device\RamDisk"
    UNICODE_STRING      ntUnicodeString;    // Unicode version of ntNameString
    UNICODE_STRING      Win32PathString;    // Win32 Name "\DosDevices\Z:"
    
    PDEVICE_OBJECT      deviceObject = NULL;    // ptr to device object
    PRAMDISK_EXTENSION  diskExtension = NULL;   // ptr to device extension
    
    NTSTATUS            ntStatus;

    RTL_QUERY_REGISTRY_TABLE    paramTable[3];

    ULONG               defaultDiskSize = DEFAULT_DISK_SIZE;
    ULONG               diskSize = DEFAULT_DISK_SIZE;
    UNICODE_STRING      driveLetterString;
    WCHAR               driveLetterBuffer[sizeof(WCHAR) * 10];

    RtlInitUnicodeString( &driveLetterString, NULL );

    driveLetterString.MaximumLength = sizeof(WCHAR) * 9;
    driveLetterString.Buffer = driveLetterBuffer;

    RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    RtlZeroMemory(&driveLetterBuffer[0], sizeof(driveLetterBuffer));
    
    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"DiskSize";
    paramTable[0].EntryContext = &diskSize;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &defaultDiskSize;
    paramTable[0].DefaultLength = sizeof(ULONG);
    
    paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name = L"DriveLetter";
    paramTable[1].EntryContext = &driveLetterString;

    if (!NT_SUCCESS(RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
        ParamPath->Buffer, &paramTable[0], NULL, NULL)))
    {
        diskSize = DEFAULT_DISK_SIZE;
    }

    ASSERT(sizeof(BOOT_SECTOR) == 512);
    
    RtlInitString( &ntNameString, "\\Device\\Ramdisk" );
    
    ntStatus = RtlAnsiStringToUnicodeString(
        &ntUnicodeString, &ntNameString, TRUE );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        RamDiskDump(
            RAMDERRORS, ("RAMDISK: Couldn't create the unicode device name\n"));
        goto RamDiskInitializeDiskExit;
    }

    ntStatus = IoCreateDevice(
        DriverObject,                   // Our Driver Object
        sizeof( RAMDISK_EXTENSION ),    // Size of state information
        &ntUnicodeString,               // Device name "\Device\RamDisk"
        FILE_DEVICE_VIRTUAL_DISK,       // Device type
        0,                              // Device characteristics
        FALSE,                          // Exclusive device
        &deviceObject );                // Returned ptr to Device Object

    if ( !NT_SUCCESS( ntStatus ) )
    {
        RamDiskDump(
            RAMDERRORS, ("RAMDISK: Couldn't create the device object\n"));
        goto RamDiskInitializeDiskExit;
    }
    
    //
    // Initialize device object and extension.
    //
    
    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;
    
    diskExtension = (PRAMDISK_EXTENSION)deviceObject->DeviceExtension;

    RtlInitUnicodeString( &diskExtension->Win32NameString, NULL );

    //
    // Allocate and zero the memory disk image.
    //
    
    diskExtension->DiskImage = ExAllocatePool( NonPagedPool, diskSize );
    
    if ( diskExtension->DiskImage == NULL )
    {
        RamDiskDump(
            RAMDERRORS, ("RAMDISK: Can't allocate memory for disk image\n") );
        goto RamDiskInitializeDiskExit;
    }
    
    RtlZeroMemory( diskExtension->DiskImage, diskSize );

    //
    // Fill in device-specific numbers based on disk size obtained from the
    // configuration manager.
    //

    diskExtension->DeviceObject = deviceObject;
    diskExtension->DiskLength = diskSize;
    diskExtension->BytesPerSector = 512;
    diskExtension->SectorsPerTrack = 32;
    diskExtension->TracksPerCylinder = 2;
    diskExtension->NumberOfCylinders = diskSize / 512 / 32 / 2;

    // Format a FAT filesystem in the memory buffer
        
    RamDiskFormatFat(diskExtension, ParamPath);

    //
    // Allocate and initialize a Unicode String containing the Win32 name
    // for our device.
    //
    RtlInitUnicodeString( &Win32PathString, WIN32_PATH );

    diskExtension->Win32NameString.Buffer = ExAllocatePool(
                                    PagedPool, 
                                    sizeof(WIN32_PATH) + 2 * sizeof(WCHAR) );

    if (!diskExtension->Win32NameString.Buffer) {
        RamDiskDump(
            RAMDERRORS,
            ("RAMDISK: Couldn't allocate buffer for the symbolic link\n")
            );

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

        goto RamDiskInitializeDiskExit;
    }

    diskExtension->Win32NameString.MaximumLength = sizeof(WIN32_PATH) + sizeof(WCHAR);

    //
    // If a drive letter wasn't specified in the registry, use drive Z.
    //
    if (driveLetterBuffer[0] == L'\0')
    {
        driveLetterBuffer[0] = L'Z';
    }
    
    if (driveLetterBuffer[1] != L':')
    {
        driveLetterBuffer[1] = L':';
    }
    
    driveLetterBuffer[2] = L'\0';
    driveLetterString.Length = 2 * sizeof(WCHAR);
    
        
    RtlAppendUnicodeStringToString(
             &diskExtension->Win32NameString, &Win32PathString );
             
    RtlAppendUnicodeStringToString(
             &diskExtension->Win32NameString, &driveLetterString );

    //
    // Create a symbolic link between our device name "\Device\RamDisk" and
    // the Win32 name (ie "\DosDevices\Z:").
    //
    ntStatus = IoCreateSymbolicLink(
                        &diskExtension->Win32NameString, &ntUnicodeString );

                        
RamDiskInitializeDiskExit:

    if (ntUnicodeString.Buffer != NULL)
    {
        RtlFreeUnicodeString( &ntUnicodeString );
    }
    
    if ( !NT_SUCCESS( ntStatus ) )
    {
        //
        // Delete everything that this routine has allocated.
        //
        
        if ( deviceObject != NULL )
        {
            if (diskExtension->DiskImage != NULL)
            {
                ExFreePool( diskExtension->DiskImage );
            }
            if (diskExtension->Win32NameString.Buffer != NULL)
            {
                ExFreePool( diskExtension->Win32NameString.Buffer );
            }
            IoDeleteDevice( deviceObject );
        }
    }

    return ntStatus;
}

VOID
RamDiskFormatFat(
    IN PRAMDISK_EXTENSION   DiskExtension,
    IN PUNICODE_STRING      ParamPath
    )
/*++

Routine Description:

    This routine is called by RamDiskInitializeDisk to format a FAT filesystem
    on the memory buffer. 

    Two parameters may be configured using the registry.  RootDirEntries
    specifies the desired number of Root Directory entries.  If necessary,
    the value is rounded up to an integral number of ROOT_DIR_ENTRIES (16).
    The default value (DEFAULT_ROOT_DIR_ENTRIES) is 512.

    SectorsPerCluster determines the number of sectors in each cluster.  The
    default value (DEFAULT_SECTORS_PER_CLUSTER) is 2.

Arguments:

    DiskExtension - a pointer to the object that represents the device
        that I/O is to be done on.

    ParamPath - a pointer to the Parameters key of our key under the Serices
        section of the registry.

Return Value:

    None

--*/
{
    RTL_QUERY_REGISTRY_TABLE	paramTable[3];
    
    ULONG           defaultRootDirEntries = DEFAULT_ROOT_DIR_ENTRIES;
    ULONG           rootDirEntries = DEFAULT_ROOT_DIR_ENTRIES;
    
    ULONG           defaultSectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;
    ULONG           sectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;
    
    
    PBOOT_SECTOR    bootSector = (PBOOT_SECTOR) DiskExtension->DiskImage;
    PUCHAR          firstFatSector;
    
    int             fatType;        // Type of FAT - 12 or 16
    USHORT          fatEntries;     // Number of cluster entries in FAT
    USHORT          fatSectorCnt;   // Number of sectors for FAT

    PDIR_ENTRY      rootDir;        // Pointer to first entry in root dir    
    //
    // Retrieve user tunable parameters
    //
    RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    
    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"RootDirEntries";
    paramTable[0].EntryContext = &rootDirEntries;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &defaultRootDirEntries;
    paramTable[0].DefaultLength = sizeof(ULONG);
    
    paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name = L"SectorsPerCluster";
    paramTable[1].EntryContext = &sectorsPerCluster;
    paramTable[1].DefaultType = REG_DWORD;
    paramTable[1].DefaultData = &defaultSectorsPerCluster;
    paramTable[1].DefaultLength = sizeof(ULONG);
    
    if (!NT_SUCCESS(RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
        ParamPath->Buffer, &paramTable[0], NULL, NULL)))
    {
        rootDirEntries = DEFAULT_ROOT_DIR_ENTRIES;
        sectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;
    }

    //
    // Round Root Directory entries up if necessary
    //
    if (rootDirEntries & (DIR_ENTRIES_PER_SECTOR - 1))
    {
        RamDiskDump(RAMDDIAG1, ("RAMDISK: Adjusting RootDirEntries\n"));
        
        rootDirEntries =
            (rootDirEntries + ( DIR_ENTRIES_PER_SECTOR - 1 )) &
                ~ ( DIR_ENTRIES_PER_SECTOR - 1 );
    }

    //
    // We need to have the 0xeb and 0x90 since this is one of the
    // checks the file system recognizer uses
    //
    bootSector->bsJump[0] = 0xeb;
    bootSector->bsJump[1] = 0x3c;
    bootSector->bsJump[2] = 0x90;

    strncpy(bootSector->bsOemName, "RobertN ", 8);
    bootSector->bsBytesPerSec = (SHORT)DiskExtension->BytesPerSector;
    bootSector->bsResSectors = 1;
    bootSector->bsFATs = 1;
    bootSector->bsRootDirEnts = (USHORT)rootDirEntries;

    bootSector->bsSectors =
        (USHORT)( DiskExtension->DiskLength / DiskExtension->BytesPerSector );
    bootSector->bsMedia = RAMDISK_MEDIA_TYPE;

    bootSector->bsSecPerClus = (UCHAR)sectorsPerCluster;

    //
    // Calculate number of sectors required for FAT
    //
    fatEntries =
        (bootSector->bsSectors - bootSector->bsResSectors -
            bootSector->bsRootDirEnts / DIR_ENTRIES_PER_SECTOR) /
                bootSector->bsSecPerClus + 2;

    //
    // Choose between 12 and 16 bit FAT based on number of clusters we
    // need to map
    //
    if (fatEntries > 4087)
    {
        fatType =  16;

        fatSectorCnt = (fatEntries * 2 + 511) / 512;

        fatEntries -= fatSectorCnt;

        fatSectorCnt = (fatEntries * 2 + 511) / 512;
    }
    else
    {
        fatType =  12;

        fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
        
        fatEntries -= fatSectorCnt;
        
        fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
    }

    bootSector->bsFATsecs = fatSectorCnt;
    bootSector->bsSecPerTrack = (USHORT)DiskExtension->SectorsPerTrack;
    bootSector->bsHeads = (USHORT)DiskExtension->TracksPerCylinder;
    bootSector->bsBootSignature = 0x29;
    bootSector->bsVolumeID = 0x12345678;
    strncpy(bootSector->bsLabel, "RamDisk    ", 11);
    strncpy(bootSector->bsFileSystemType, "FAT1?   ", 8);
    bootSector->bsFileSystemType[4] = ( fatType == 16 ) ? '6' : '2';
    
    bootSector->bsSig2[0] = 0x55;
    bootSector->bsSig2[1] = 0xAA;
    
    //
    // The FAT is located immediately following the boot sector.
    //
    firstFatSector = (PUCHAR)(bootSector + 1);
    firstFatSector[0] = RAMDISK_MEDIA_TYPE;
    firstFatSector[1] = 0xFF;
    firstFatSector[2] = 0xFF;

    if (fatType == 16)
        firstFatSector[3] = 0xFF;

    //
    // The Root Directory follows the FAT
    //
    rootDir = (PDIR_ENTRY)(bootSector + 1 + fatSectorCnt);
    strcpy(rootDir->deName, "MS-RAMDR");
    strcpy(rootDir->deExtension, "IVE");
    rootDir->deAttributes = DIR_ATTR_VOLUME;
}

NTSTATUS
RamDiskCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when the RamDisk is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_INVALID_PARAMETER if parameters are invalid,
    STATUS_SUCCESS otherwise.

--*/

{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    return STATUS_SUCCESS;
}

NTSTATUS
RamDiskDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

Arguments:

    DeviceObject - a pointer to the object that represents the device
        that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS if recognized I/O control code,
    STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/

{
    PRAMDISK_EXTENSION  diskExtension;
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            ntStatus;

    //
    // Set up necessary object and extension pointers.
    //

    diskExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Assume failure.
    //

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

    //
    // Determine which I/O control code was specified.
    //

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode )
    {
    
    case IOCTL_DISK_GET_MEDIA_TYPES:
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        //
        // Return the drive geometry for the ram disk.  Note that
        // we return values which were made up to suit the disk size.
        //

        if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof( DISK_GEOMETRY ) )
        {

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            
        }
        else
        {
            PDISK_GEOMETRY outputBuffer;

            outputBuffer = ( PDISK_GEOMETRY ) Irp->AssociatedIrp.SystemBuffer;
            outputBuffer->MediaType = RemovableMedia;
            outputBuffer->Cylinders = RtlConvertUlongToLargeInteger(
                diskExtension->NumberOfCylinders );
            outputBuffer->TracksPerCylinder = diskExtension->TracksPerCylinder;
            outputBuffer->SectorsPerTrack = diskExtension->SectorsPerTrack;
            outputBuffer->BytesPerSector = diskExtension->BytesPerSector;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof( DISK_GEOMETRY );
        }
        break;

    case IOCTL_DISK_GET_PARTITION_INFO: 
        //
        // Return the information about the partition.
        //

        if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof( PARTITION_INFORMATION ) )
        {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            PPARTITION_INFORMATION outputBuffer;
            PBOOT_SECTOR    bootSector = (PBOOT_SECTOR) diskExtension->DiskImage;
        
            outputBuffer = ( PPARTITION_INFORMATION )Irp->AssociatedIrp.SystemBuffer;
        
            outputBuffer->PartitionType = 
                (bootSector->bsFileSystemType[4] == '6') ?
                    PARTITION_FAT_16 : PARTITION_FAT_12;
        
            outputBuffer->BootIndicator = FALSE;
        
            outputBuffer->RecognizedPartition = TRUE;
        
            outputBuffer->RewritePartition = FALSE;
        
            outputBuffer->StartingOffset = RtlConvertUlongToLargeInteger(0);
        
            outputBuffer->PartitionLength = RtlConvertUlongToLargeInteger(diskExtension->DiskLength);
        
	    outputBuffer->HiddenSectors =  1L;
        
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof( PARTITION_INFORMATION );
        }
        break;


    case IOCTL_DISK_VERIFY:
        {
            //
            // Move parameters from the VerifyInformation structure to
            // the READ parameters area, so that we'll find them when
            // we try to treat this like a READ.
            //
            
            PVERIFY_INFORMATION	verifyInformation;

            verifyInformation = Irp->AssociatedIrp.SystemBuffer;

            irpSp->Parameters.Read.ByteOffset.LowPart =
                verifyInformation->StartingOffset.LowPart;
            irpSp->Parameters.Read.ByteOffset.HighPart =
                verifyInformation->StartingOffset.HighPart;
            irpSp->Parameters.Read.Length = verifyInformation->Length;

            //
            // A VERIFY is identical to a READ, except for the fact that no
            // data gets transferred.  So follow the READ code path.
            //

            ntStatus = RamDiskReadWrite( DeviceObject, Irp );
        }
        return ntStatus;

    default:
        //
        // The specified I/O control code is unrecognized by this driver.
        // The I/O status field in the IRP has already been set so just
        // terminate the switch.
        //

        RamDiskDump(
        RAMDDIAG1,
            ("RAMDISK: ERROR:  unrecognized IOCTL %x\n",
            irpSp->Parameters.DeviceIoControl.IoControlCode));

        break;
    }

    //
    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    //

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return ntStatus;
}

NTSTATUS
RamDiskReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system to read or write to a
    device that we control.  It can also be called by
    RamDiskDispatchDeviceControl() to do a VERIFY.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_INVALID_PARAMETER if parameters are invalid,
    STATUS_SUCCESS otherwise.

--*/

{
    PRAMDISK_EXTENSION  diskExtension;
    PIO_STACK_LOCATION  irpSp;
    PUCHAR              CurrentAddress;

    //
    // Set up necessary object and extension pointers.
    //

    diskExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Check for invalid parameters.  It is an error for the starting offset
    // + length to go past the end of the buffer, or for the length to
    // not be a proper multiple of the sector size.
    //
    // Others are possible, but we don't check them since we trust the
    // file system and they aren't deadly.
    //

    if (RtlLargeIntegerGreaterThan(
            RtlLargeIntegerAdd(
                irpSp->Parameters.Read.ByteOffset,
                RtlConvertUlongToLargeInteger(irpSp->Parameters.Read.Length)),
            RtlConvertUlongToLargeInteger(diskExtension->DiskLength)) ||
        (irpSp->Parameters.Read.Length & (diskExtension->BytesPerSector - 1)))
    {
        //
        // Do not give an I/O boost for parameter errors.
        //

        RamDiskDump(
            RAMDERRORS,
            (
                "RAMDISK: Error invalid parameter\n"
                "         ByteOffset: %x\n"
                "         Length: %d\n"
                "         Operation: %x\n",
                Irp,
                irpSp->Parameters.Read.ByteOffset,
                irpSp->Parameters.Read.Length,
                irpSp->MajorFunction
            ));

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get a system-space pointer to the user's buffer.  A system
    // address must be used because we may already have left the
    // original caller's address space.
    //

    if ( Irp->MdlAddress != NULL )
    {
        CurrentAddress = MmGetSystemAddressForMdl( Irp->MdlAddress );
    }
    else
    {
        //
        // Strictly speaking this isn't necessary, the only time we
        // won't have an MDL is if we're processing IOCTL_DISK_VERIFY.
        // In which case, we don't reference CurrentAddress.  But at
        // least this should keep the compiler from warning us.

        CurrentAddress = 0;
    }

    RamDiskDump(
        RAMDDIAG2,
        (
            "RAMDISK: Irp of Request: %x\n"
            "         Vmem Address of Transfer: %x - %x\n"
            "         Length of Transfer: %d\n"
            "         Operation: %x\n"
            "         Starting ByteOffset: %x\n",
            Irp,
            CurrentAddress,
            ((PUCHAR)CurrentAddress) + irpSp->Parameters.Read.Length,
            irpSp->Parameters.Read.Length,
            irpSp->MajorFunction,
            irpSp->Parameters.Read.ByteOffset.LowPart
        ));

    Irp->IoStatus.Information = irpSp->Parameters.Read.Length;

    switch (irpSp->MajorFunction)
    {
    case IRP_MJ_READ:
        RtlMoveMemory(
            CurrentAddress,
            diskExtension->DiskImage + irpSp->Parameters.Read.ByteOffset.LowPart,
            irpSp->Parameters.Read.Length);
        break;

    case IRP_MJ_DEVICE_CONTROL:
        //
        // The only way we can get this major code is if the VERIFY
        // ioctl called RamDiskDispatchReadWrite().
        //
        break;

    case IRP_MJ_WRITE:
        RtlMoveMemory(
            diskExtension->DiskImage + irpSp->Parameters.Read.ByteOffset.LowPart,
            CurrentAddress, irpSp->Parameters.Read.Length);
        break;

    default:
        Irp->IoStatus.Information = 0;
        break;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

VOID
RamDiskUnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver.

Return Value:

    None
--*/

{
    PDEVICE_OBJECT      deviceObject = DriverObject->DeviceObject;

    if ( deviceObject != NULL )
    {
        PRAMDISK_EXTENSION      diskExtension = deviceObject->DeviceExtension;

        if (diskExtension != NULL)
        {
            if (diskExtension->DiskImage != NULL)
            {
                ExFreePool( diskExtension->DiskImage );
            }
            if (diskExtension->Win32NameString.Buffer != NULL)
            {
                IoDeleteSymbolicLink( &diskExtension->Win32NameString );

                ExFreePool( diskExtension->Win32NameString.Buffer );
            }
        }
        IoDeleteDevice( deviceObject );
    }
}
