/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   async2.c

Abstract:

   This driver is a derivitive of the original async sample which showed
   how to notify a user mode application or service of an asynchronous event
   from kernel mode. This sample is updated to take advantage of new system
   services available in kernel mode for NT 4.0.

Author:

   Mark A. Overby (MarkOv)  1-Mar-1996

Environment:

   Kernel Mode Only

Revision History:

   1-Mar-1996  Revised original async.c to become async2.c  (MarkOv)

--*/

#include <ntddk.h>

#ifdef DBG
ULONG DebugLevel = 0 ;
#endif // DBG

#define ERROR     1
#define INFORM_LO 3
#define INFORM_HI 7

#define Async2Message(LEVEL,STRING)    do  { \
                                          if (DebugLevel & LEVEL) { \
                                             DbgPrint STRING  ; \
                                          } \
                                       } while (0)

typedef struct _DEVICE_EXTENSION {
   ULONG DelayLength ;
   ULONG Count ;
   PDEVICE_OBJECT DeviceObject ;
   KSPIN_LOCK Lock ;
   HANDLE Handle ;
   PKEVENT Event ;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION ;

NTSTATUS
DriverEntry (
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   ) ;

VOID
Async2SimulatedDPC(
   IN PDEVICE_OBJECT DeviceObject,
   IN OUT PVOID Context
   ) ;

VOID
Async2Unload(
   IN PDRIVER_OBJECT DriverObject
   ) ;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT,DriverEntry)
#endif


NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )

/*++

Routine Description:

   This is the main entry routine for this driver which will be called by
   the I/O Manager when this driver has been requested to load. It will be
   responsible for initializing all pertinent resources, data, and objects as
   needed by this driver for initial run.

Arguments:

   DriverObject - Pointer to the DRIVER_OBJECT which represents this driver.

   RegistryPath - Pointer to the registry entry for this driver.

Return Value:

   The final NTSTATUS of the attempt to load this driver.

--*/

{
   NTSTATUS status ;
   PDEVICE_OBJECT deviceObject ;
   PDEVICE_EXTENSION extension ;

#ifdef DBG
   RTL_QUERY_REGISTRY_TABLE paramTable[4] ;
   ULONG breakNow = 0 ;
   ULONG defaultValue = 0 ;
#else
   RTL_QUERY_REGISTRY_TABLE paramTable[2] ;
#endif // DBG

   ULONG delayValue = 0 ;
   ULONG delayDefault = 5 ;
   UNICODE_STRING paramPath ;
   UNICODE_STRING parsedRegPath ;
   UNICODE_STRING deviceName ;
   UNICODE_STRING eventName ;

   //
   // Prepare a unicode string that will be passed into the path so that the
   // parameters subkey of the registry entry for this driver can be accessed.
   //

   RtlInitUnicodeString (&paramPath, L"\\Parameters") ;

   //
   // Initialize the parsed registry path unicode string with a buffer large
   // enough to hold the entire path that will need to be passed into
   // RtlQueryRegistryValues.
   //

   parsedRegPath.MaximumLength = RegistryPath->MaximumLength +
                                 paramPath.MaximumLength + sizeof (WCHAR) ;
   parsedRegPath.Length = 0 ;
   parsedRegPath.Buffer = ExAllocatePool (NonPagedPool,
                                          parsedRegPath.MaximumLength) ;

   //
   // Make sure that resources could be allocated for the buffer.
   //

   if (!parsedRegPath.Buffer) {
#ifdef DBG
      KdPrint (("Async2: Failed to allocate unicode buffer in DriverEntry")) ;
#endif // DBG
      return(STATUS_INSUFFICIENT_RESOURCES) ;
   }

   //
   // Zero out the memory for the unicode string buffer and then parse the
   // two unicode strings together for the entire path.
   //

   RtlZeroMemory (parsedRegPath.Buffer, parsedRegPath.MaximumLength) ;
   RtlCopyUnicodeString (&parsedRegPath, RegistryPath) ;
   RtlAppendUnicodeStringToString (&parsedRegPath, &paramPath) ;

   //
   // Zero out the table that will be used to retrieve the registry values
   // required for initialization.
   //

   RtlZeroMemory (paramTable, sizeof (paramTable)) ;

   //
   // Set up the requisite entries in the query table to get all of the
   // needed data.
   //

   paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT ;
   paramTable[0].Name = L"DelayValue" ;
   paramTable[0].EntryContext = &delayValue ;
   paramTable[0].DefaultType = REG_DWORD ;
   paramTable[0].DefaultData = &delayDefault ;
   paramTable[0].DefaultLength = sizeof (ULONG) ;

#ifdef DBG

   //
   // These registry queries only apply if the driver is doing debugging, so
   // don't initialize them except for a checked build of this driver.
   //

   paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT ;
   paramTable[1].Name = L"BreakOnEntry" ;
   paramTable[1].EntryContext = &breakNow ;
   paramTable[1].DefaultType = REG_DWORD ;
   paramTable[1].DefaultData = &defaultValue ;
   paramTable[1].DefaultLength = sizeof (ULONG) ;

   paramTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT ;
   paramTable[2].Name = L"DebugLevel" ;
   paramTable[2].EntryContext = &DebugLevel ;
   paramTable[2].DefaultType = REG_DWORD ;
   paramTable[2].DefaultData = &defaultValue ;
   paramTable[2].DefaultLength = sizeof (ULONG) ;

#endif // DBG

   //
   // Query the registry for the requested information.
   //

   status = RtlQueryRegistryValues (RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                    parsedRegPath.Buffer,
                                    paramTable,
                                    NULL,
                                    NULL) ;

   //
   // Check for the return status.
   //

   if (!NT_SUCCESS(status)) {

      //
      // The registry could not be queried for one reason or another. Free
      // dynamically allocated resources and return an appropriate status.
      //

#ifdef DBG
      KdPrint (("Async2: Could not query registry.\n")) ;
#endif // DBG

      ExFreePool (parsedRegPath.Buffer) ;
      return(status) ;
   }

   //
   // The parsed registry path buffer is no longer needed, so go ahead and
   // free up those resources.
   //

   ExFreePool (parsedRegPath.Buffer) ;

#ifdef DBG

   //
   // If a break on entry was requested, do that now.
   //

   if (breakNow) {
      DbgBreakPoint() ;
   }

#endif // DBG

   //
   // Set up entry points for the IRP dispatch table in the DRIVER_OBJECT. No
   // entries are required as this driver doesn't use IRP's. The only dispatch
   // entry points that are required is the unload routine, so that this
   // driver can be stopped and unloaded,
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Setting dispatch entry points.\n")) ;
#endif // DBG

   DriverObject->DriverUnload = Async2Unload ;

   //
   // Create a DEVICE_OBJECT so that an IoTimerRoutine can be associated with
   // that DEVICE_OBJECT.
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Creating device object.\n")) ;
#endif // DBG

   RtlInitUnicodeString (&deviceName, L"\\Device\\Async2") ;
   status = IoCreateDevice (DriverObject,
                            sizeof (DEVICE_EXTENSION),
                            &deviceName,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &deviceObject) ;

   //
   // Check to see if the device object was successfully created.
   //

   if (!NT_SUCCESS(status)) {

#ifdef DBG
      Async2Message (ERROR,("Async2: Could not create DO. Error: %X\n",status)) ;
#endif // DBG

      return(status) ;
   }

   extension = deviceObject->DeviceExtension ;
   extension->DeviceObject = deviceObject ;
   extension->Count = 0 ;
   extension->DelayLength = delayValue ;

   //
   // Initialize the spin lock in the device extension.
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Creating spin lock.\n")) ;
#endif // DBG

   KeInitializeSpinLock (&extension->Lock) ;

   //
   // Set up and IoTimerRoutine to simulate a DPC from ISR.
   //

#ifdef DBG
   Async2Message (INFORM_LO,("Async2: Creating timer routine.\n")) ;
#endif // DBG

   status = IoInitializeTimer (deviceObject, Async2SimulatedDPC, NULL) ;

   if (!NT_SUCCESS(status)) {

#ifdef DBG
      Async2Message (ERROR,("Async2: Could not create timer routine.\n")) ;
#endif // DBG

      IoDeleteDevice (deviceObject) ;
      return(status) ;
   }

   //
   // Create a named notification event that will be used to signal when
   // the timer routine has hit its count. Make sure to set the state to
   // non-signalled.
   //

#ifdef DBG
      Async2Message (INFORM_LO,("Async2: Creating notification event.\n")) ;
#endif // DBG

   RtlInitUnicodeString (&eventName, L"\\BaseNamedObjects\\Async2SignalEvt") ;
   extension->Event = IoCreateNotificationEvent (&eventName,
                                                 &extension->Handle) ;
   if (!extension->Event) {

#ifdef DBG
      Async2Message (ERROR,("Async2: Could not create notification event.\n")) ;
#endif // DBG

      IoDeleteDevice (deviceObject) ;
      return(status) ;
   }

   KeClearEvent (extension->Event) ;

   //
   // Start the timer routine.
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Starting timer routine.\n")) ;
#endif

   IoStartTimer (deviceObject) ;

   //
   // All activity for initialization has been completed. Therefore, just
   // return a successful status.
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Finished in DriverEntry.\n")) ;
#endif // DBG

   return(STATUS_SUCCESS) ;
}


VOID Async2SimulatedDPC(
   IN PDEVICE_OBJECT DeviceObject,
   IN OUT PVOID Context
   )

/*++

Routine Description:

   This is the main workhorse routine of this driver. It will be called once
   per second by the I/O Manager. It will be responsible for determining
   based on the current count if it should signal the event that user mode
   is waiting on. If so, then signal it and reset the status immediately.

Arguments:

   DeviceObject - Pointer to the DEVICE_OBJECT that this timer is from.

   Context - Driver defined context (not used)

Return Value:

   None.

--*/

{
   PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension ;
   KIRQL oldIrql ;

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Entered Async2SimulatedDpc\n")) ;
#endif // DBG

   //
   // Increment the current count of times this routine has been entered.
   // Acquire the spin lock to protect this variable.
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Incrementing current count w/ spin.\n")) ;
#endif // DBG

   KeAcquireSpinLock (&extension->Lock, &oldIrql) ;
   extension->Count++ ;

   //
   // Check to see if the limit has been reached so that the event should be
   // signalled or not.
   //

   if (extension->Count < extension->DelayLength) {

      //
      // Release the spin lock and return control.
      //

#ifdef DBG
      Async2Message (INFORM_HI,("Async2: Not time for notification.\n")) ;
#endif // DBG

      KeReleaseSpinLock (&extension->Lock, oldIrql) ;
      return ;
   }

   //
   // It is time for the event to be signalled.
   //

   extension->Count = 0 ;

   //
   // Signal the event, then immediately reset the event into a non-signalled
   // state.
   //

#ifdef DBG
   Async2Message (INFORM_LO,("Async2: Signalling event.\n")) ;
#endif // DBG

   KeSetEvent (extension->Event, 0, FALSE) ;

#ifdef DBG
   Async2Message (INFORM_LO,("Async2: Resetting event.\n")) ;
#endif // DBG

   KeClearEvent (extension->Event) ;

   //
   // Release the spin lock and return control.
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Releasing spin lock and returning.\n")) ;
#endif // DBG

   KeReleaseSpinLock (&extension->Lock, oldIrql) ;
}


VOID
Async2Unload(
   IN PDRIVER_OBJECT DriverObject
   )

/*++

Routine Description:

   This routine will be called by the I/O Manager in response to a user (or
   possibly) kernel request to unload and stop this driver.

Arguments:

   DriverObject - Pointer to the DRIVER_OBJECT which represents this driver.

Return Value:

   None.

--*/

{
   PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject ;
   PDEVICE_EXTENSION extension = deviceObject->DeviceExtension ;

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Unloading Async2.\n")) ;
#endif // DBG

   //
   // Stop the timer routine.
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Stopping timer routine.\n")) ;
#endif

   IoStopTimer (deviceObject) ;

   //
   // Close the handle to the event so that it can be freed.
   //

#ifdef DBG
   Async2Message (INFORM_LO,("Async2: Closing handle to event.\n")) ;
#endif // DBG

   ZwClose (extension->Handle) ;

   //
   // Now delete the device object.
   //

#ifdef DBG
   Async2Message (INFORM_HI,("Async2: Deleting device object.\n")) ;
#endif // DBG

   IoDeleteDevice (deviceObject) ;
}
