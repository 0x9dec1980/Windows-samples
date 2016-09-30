/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   Delay.c

Abstract:

   This module is designed to show the usage of KeDelayExecutionThread.
   It will also demonstrate that KeDelayExecutionThread works as
   designed by getting and returning tick count information.

Author:

   Mark A. Overby (MarkOv)  12-March-1995

Revision History:

--*/

#include "ntddk.h"
#include "delay.h"

NTSTATUS
DelayLoadUnload(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   ) ;

NTSTATUS
DelayControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   ) ;

NTSTATUS
DelayUnload(
   IN PDRIVER_OBJECT DriverObject
   ) ;


NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )

/*++

Routine Description:

   This routine will handle the initialization of this driver. There is
   not a lot of code required other than to initialize the device object
   and extension, then return.

Arguments:

   DriverObject - Pointer to the driver object for this driver

   RegistryPath - Pointer to the registry entry for this driver

Return Value:

   Final NTSTATUS of the attempt to load the driver

--*/

{
   PDEVICE_OBJECT deviceObject ;
   NTSTATUS status ;
   UNICODE_STRING objectName ;
   UNICODE_STRING win32Name ;

   KdPrint (("Delay: Entered DriverEntry.\n")) ;

   // Init the local status variable to a known value.

   status = STATUS_SUCCESS ;

   // Initialize a unicode string for the driver object name.

   RtlInitUnicodeString (&objectName, L"\\Device\\DelayDrv") ;

   // Attempt to create the device object.

   status = IoCreateDevice (DriverObject, 0 , &objectName,
                            FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject) ;

   // Check the return of the device object creation. If successful continue
   // initializing the driver, otherwise we will jump to bail out code.

   if (!NT_SUCCESS(status)) {

      // Bail out.

      KdPrint (("Delay: Failed to create device ob.\n")) ;
      return(status) ;
   }

   // Set up dispatch entry points for the driver.

   DriverObject->MajorFunction[IRP_MJ_CREATE] = DelayLoadUnload ;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = DelayLoadUnload ;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DelayControl ;
   DriverObject->DriverUnload = DelayUnload ;

   // Create symbolic link into user mode for driver.

   RtlInitUnicodeString (&win32Name, L"\\DosDevices\\DelayTest") ;
   status = IoCreateSymbolicLink (&win32Name, &objectName) ;

   // Check for success of creation.

   if (!NT_SUCCESS(status)) {

      // Bail out after deleting device object.

      IoDeleteDevice (deviceObject) ;
      KdPrint (("Delay: Failed to create win32 name.\n")) ;
      return(status) ;
   }

   // Setup so entire driver can be paged.

   MmPageEntireDriver (DriverObject->DriverInit) ;

   // Done. Return status.

   return(status) ;
}



NTSTATUS
DelayLoadUnload(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

   The driver will be called in this routine for both IRP_MJ_CREATE and
   IRP_MJ_CLOSE. However, there is really nothing to be done for either of
   these IRP's so they will just be completed with success.

Arguments:

   DeviceObject - Pointer to the device object of this request

   Irp - Pointer to I/O request packet of this request

Return Value:

   Final NTSTATUS of this operation (Always STATUS_SUCCESS for this routine).

--*/

{
   // Nothing to be done except for setting the information and status fields
   // in the IRP and completing it.

   Irp->IoStatus.Status = STATUS_SUCCESS ;
   Irp->IoStatus.Information = 0 ;

   IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

   return(STATUS_SUCCESS) ;
}


NTSTATUS
DelayControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

   This driver will be called in this routine for IRP_MJ_DEVICE_CONTROL in
   response to DeviceIoControl requests. The only IOCTL that this driver will
   handle is one for IOCTL_CHECK_DELAY to implement the delay test.

Arguments:

   DeviceObject - Pointer to the device object of this request

   Irp - Pointer to the I/O request packet of this request

Return Value:

   Final NTSTATUS of the operation.

--*/

{
   PIO_STACK_LOCATION irpStack ;
   LARGE_INTEGER delayValue ;
   LARGE_INTEGER delayTrue ;
   PDELAY_PARMS buffer ;
   NTSTATUS status ;

   // Get the current Irp stack location of this request.

   irpStack = IoGetCurrentIrpStackLocation (Irp) ;

   // Switch on the Device I/O control request code.

   switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
   case IOCTL_CHECK_DELAY:

      // This IOCTL will handle a test of the delay to see if the call
      // to KeDelayExecutionThread delays for at least as long as expected.

      // First, check to make sure that the parameters are valid.

      if (!irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
          sizeof (DELAY_PARMS)) {

          // Parameters are invalid. Return a failure.

          Irp->IoStatus.Status = STATUS_INVALID_PARAMETER ;
          Irp->IoStatus.Information = 0 ;
          IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
          return(STATUS_INVALID_PARAMETER) ;
      }

      // Parameters are fine. Set up pointer to buffered data to be returned.

      buffer = (PDELAY_PARMS) Irp->AssociatedIrp.SystemBuffer ;

      // Now get the tick count multiplier.

      buffer->tickMultiple.QuadPart = KeQueryTimeIncrement () ;

      // Set up the delay value.

      delayValue.QuadPart = 10*1000*1000 ;   // 1 second
      delayTrue.QuadPart = -(delayValue.QuadPart) ;

      // Get the current tick count and store.

      KeQueryTickCount (&buffer->beginCount) ;

      // Delay the thread.

      status = KeDelayExecutionThread (KernelMode, FALSE, &delayTrue);

      // Get the current tick count and store.

      KeQueryTickCount (&buffer->endCount) ;

      // Put the status into the buffer as well.

      buffer->status.QuadPart = status ;

      // Return the IRP.

      Irp->IoStatus.Status = STATUS_SUCCESS ;
      Irp->IoStatus.Information = sizeof (DELAY_PARMS) ;
      IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
      return(STATUS_SUCCESS) ;
      break;

   default:

      // Indicate that this is not supported.

      Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED ;
      Irp->IoStatus.Information = 0 ;
      IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
      return(STATUS_NOT_IMPLEMENTED) ;
   }
}


NTSTATUS
DelayUnload(
   IN PDRIVER_OBJECT DriverObject
   )

/*++

Routine Description:

   This routine will unload this test driver.

Arguments:

   DriverObject - Pointer to the driver object for this request.

Return Value:

   Final NTSTATUS of the request.

--*/

{
   UNICODE_STRING win32Name ;

   // Initialize the name of the win32 symbolic link.

   RtlInitUnicodeString (&win32Name, L"\\DosDevices\\DelayTest") ;

   // Delete the symbolic link.

   IoDeleteSymbolicLink (&win32Name) ;

   // Delete the device object.

   IoDeleteDevice (DriverObject->DeviceObject) ;

   // return.

   return(STATUS_SUCCESS) ;
}
