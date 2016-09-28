/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    packet.h

Abstract:

Author:

Revision History:

--*/

#define  MAX_REQUESTS   4


typedef struct _INTERNAL_REQUEST {
    LIST_ENTRY     ListElement;
    PIRP           Irp;
    NDIS_REQUEST   Request;

    } INTERNAL_REQUEST, *PINTERNAL_REQUEST;



//
// Port device extension.
//

typedef struct _DEVICE_EXTENSION {

    PDEVICE_OBJECT DeviceObject;

    NDIS_HANDLE    NdisProtocolHandle;

    NDIS_STRING    AdapterName;

    PWSTR          BindString;
    PWSTR          ExportString;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _OPEN_INSTANCE {

    PDEVICE_EXTENSION   DeviceExtension;

    NDIS_HANDLE    AdapterHandle;

    NDIS_HANDLE         PacketPool;

    KSPIN_LOCK          RcvQSpinLock;
    LIST_ENTRY          RcvList;

    PIRP                OpenCloseIrp;

    KSPIN_LOCK          RequestSpinLock;
    LIST_ENTRY          RequestList;

    LIST_ENTRY          ResetIrpList;

    INTERNAL_REQUEST    Requests[MAX_REQUESTS];

} OPEN_INSTANCE, *POPEN_INSTANCE;



typedef struct _PACKET_RESERVED {
    LIST_ENTRY     ListElement;
    PIRP           Irp;
    PMDL           pMdl;
    }  PACKET_RESERVED, *PPACKET_RESERVED;



#define  ETHERNET_HEADER_LENGTH   14

#define RESERVED(_p) ((PPACKET_RESERVED)((_p)->ProtocolReserved))

#define  TRANSMIT_PACKETS    16





VOID
PacketOpenAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus
    );

VOID
PacketCloseAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );


NDIS_STATUS
PacketReceiveIndicate(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
    );

VOID
PacketReceiveComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    );


VOID
PacketRequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST pRequest,
    IN NDIS_STATUS   Status
    );

VOID
PacketSendComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status
    );


VOID
PacketResetComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );


VOID
PacketStatus(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize
    );


VOID
PacketStatusComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    );

VOID
PacketTransferDataComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
    );


VOID
PacketRemoveReference(
    IN PDEVICE_EXTENSION DeviceExtension
    );


NTSTATUS
PacketCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP FlushIrp
    );


NTSTATUS
PacketShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
PacketUnload(
    IN PDRIVER_OBJECT DriverObject
    );



NTSTATUS
PacketOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PacketClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PacketWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PacketRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PacketIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
