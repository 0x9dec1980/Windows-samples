/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    WshTcpip.c

Abstract:

    This module contains necessary routines for the TCP/IP Windows Sockets
    Helper DLL.  This DLL provides the transport-specific support necessary
    for the Windows Sockets DLL to use TCP/IP as a transport.

Author:

    David Treadwell     19-Jul-1992

Revision History:

	Converted to use Win32 APIs.

--*/

#include <windows.h>
#include <winerror.h>
#include <winioctl.h>

#include <assert.h>
#include <wchar.h>

typedef LONG NTSTATUS;
/*lint -e624 */  // Don't complain about different typedefs.   // winnt
typedef NTSTATUS *PNTSTATUS;
/*lint +e624 */  // Resume checking for different typedefs.    // winnt

#include <tdi.h>

#include <winsock.h>

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt

#define RtlInitUnicodeString(s, b) \
    do \
    { \
	PUNICODE_STRING string = (s); \
	PWSTR   buffer = (b); \
	int     length = 2 * wcslen(buffer); \
 \
	string->Length = length; \
	string->MaximumLength = length; \
	string->Buffer = buffer; \
    } \
    while ( 0 );

#include <wsahelp.h>

#include <sys\snet\stcp_opt.h>

//
// Structure and variables to define the triples supported by TCP/IP. The
// first entry of each array is considered the canonical triple for
// that socket type; the other entries are synonyms for the first.
//

typedef struct _MAPPING_TRIPLE {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;

MAPPING_TRIPLE TcpMappingTriples[] = { AF_INET,   SOCK_STREAM, IPPROTO_TCP,
				       AF_INET,   SOCK_STREAM, 0,
				       AF_INET,   0,           IPPROTO_TCP,
				       AF_UNSPEC, 0,           IPPROTO_TCP,
				       AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP };

MAPPING_TRIPLE UdpMappingTriples[] = { AF_INET,   SOCK_DGRAM,  IPPROTO_UDP,
				       AF_INET,   SOCK_DGRAM,  0,
				       AF_INET,   0,           IPPROTO_UDP,
				       AF_UNSPEC, 0,           IPPROTO_UDP,
				       AF_UNSPEC, SOCK_DGRAM,  IPPROTO_UDP };

//
// Forward declarations of internal routines.
//

VOID
WINAPI
CompleteTdiActionApc (
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
    );

INT
DoTdiAction (
    IN HANDLE TdiConnectionObjectHandle,
    IN INT OptionLevel,
    IN INT OptionName,
    IN INT OptionalData
    );

BOOLEAN
IsTripleInList (
    IN PMAPPING_TRIPLE List,
    IN ULONG ListLength,
    IN INT AddressFamily,
    IN INT SocketType,
    IN INT Protocol
    );

//
// The socket context structure for this DLL.  Each open TCP/IP socket
// will have one of these context structures, which is used to maintain
// information about the socket.
//

typedef struct _WSHTCPIP_SOCKET_CONTEXT {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
    INT ReceiveBufferSize;
    BOOLEAN KeepAlive;
    BOOLEAN DontRoute;
    BOOLEAN NoDelay;
} WSHTCPIP_SOCKET_CONTEXT, *PWSHTCPIP_SOCKET_CONTEXT;

#define DEFAULT_RECEIVE_BUFFER_SIZE 8192


INT
WINAPI
WSHGetSockaddrType (
    IN PSOCKADDR Sockaddr,
    IN DWORD SockaddrLength,
    OUT PSOCKADDR_INFO SockaddrInfo
    )

/*++

Routine Description:

    This routine parses a sockaddr to determine the type of the
    machine address and endpoint address portions of the sockaddr.
    This is called by the winsock DLL whenever it needs to interpret
    a sockaddr.

Arguments:

    Sockaddr - a pointer to the sockaddr structure to evaluate.

    SockaddrLength - the number of bytes in the sockaddr structure.

    SockaddrInfo - a pointer to a structure that will receive information
	about the specified sockaddr.


Return Value:

    INT - a winsock error code indicating the status of the operation, or
	NO_ERROR if the operation succeeded.

--*/

{
    UNALIGNED SOCKADDR_IN *sockaddr = (PSOCKADDR_IN)Sockaddr;
    ULONG i;

    //
    // Make sure that the address family is correct.
    //

    if ( sockaddr->sin_family != AF_INET ) {
	return WSAEAFNOSUPPORT;
    }

    //
    // Make sure that the length is correct.
    //

    if ( SockaddrLength < sizeof(SOCKADDR_IN) ) {
	return WSAEFAULT;
    }

    //
    // The address passed the tests, looks like a good address.
    // Determine the type of the address portion of the sockaddr.
    //

    if ( sockaddr->sin_addr.s_addr == INADDR_ANY ) {
	SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
    } else if ( sockaddr->sin_addr.s_addr == INADDR_BROADCAST ) {
	SockaddrInfo->AddressInfo = SockaddrAddressInfoBroadcast;
    } else if ( sockaddr->sin_addr.s_addr == INADDR_LOOPBACK ) {
	SockaddrInfo->AddressInfo = SockaddrAddressInfoLoopback;
    } else {
	SockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;
    }

    //
    // Determine the type of the port (endpoint) in the sockaddr.
    //

    if ( sockaddr->sin_port == 0 ) {
	SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    } else if ( ntohs( sockaddr->sin_port ) < 2000 ) {
	SockaddrInfo->EndpointInfo = SockaddrEndpointInfoReserved;
    } else {
	SockaddrInfo->EndpointInfo = SockaddrEndpointInfoNormal;
    }

    //
    // Zero out the sin_zero part of the address.  We silently allow
    // nonzero values in this field.
    //

    for ( i = 0; i < sizeof(sockaddr->sin_zero); i++ ) {
	sockaddr->sin_zero[i] = 0;
    }

    return NO_ERROR;

} // WSHGetSockaddrType


INT
WINAPI
WSHGetSocketInformation (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN INT Level,
    IN INT OptionName,
    OUT PCHAR OptionValue,
    OUT PINT OptionLength
    )

/*++

Routine Description:

    This routine retrieves information about a socket for those socket
    options supported in this helper DLL.  The options supported here
    are SO_KEEPALIVE and SO_DONTROUTE.  This routine is called by
    the winsock DLL when a level/option name combination is passed
    to getsockopt() that the winsock DLL does not understand.

Arguments:

    HelperDllSocketContext - the context pointer returned from
	WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're getting
	information.

    TdiAddressObjectHandle - the TDI address object of the socket, if
	any.  If the socket is not yet bound to an address, then
	it does not have a TDI address object and this parameter
	will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
	if any.  If the socket is not yet connected, then it does not
	have a TDI connection object and this parameter will be NULL.

    Level - the level parameter passed to getsockopt().

    OptionName - the optname parameter passed to getsockopt().

    OptionValue - the optval parameter passed to getsockopt().

    OptionLength - the optlen parameter passed to getsockopt().

Return Value:

    INT - a winsock error code indicating the status of the operation, or
	NO_ERROR if the operation succeeded.

--*/

{
    PWSHTCPIP_SOCKET_CONTEXT context = HelperDllSocketContext;

    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );

    //
    // Check if this is an internal request for context information.
    //

    if ( Level == SOL_INTERNAL && OptionName == SO_CONTEXT ) {

	//
	// The Windows Sockets DLL is requesting context information 
	// from us.  If an output buffer was not supplied, the Windows 
	// Sockets DLL is just requesting the size of our context 
	// information.  
	//

	if ( OptionValue != NULL ) {

	    //
	    // Make sure that the buffer is sufficient to hold all the 
	    // context information.  
	    //

	    if ( *OptionLength < sizeof(*context) ) {
		return WSAEFAULT;
	    }

	    //
	    // Copy in the context information.
	    //

	    CopyMemory( OptionValue, context, sizeof(*context) );
	}

	*OptionLength = sizeof(*context);

	return NO_ERROR;
    }

    //
    // The only other levels we support here are SOL_SOCKET and 
    // IPPROTO_TCP.  
    //

    if ( Level != SOL_SOCKET && Level != IPPROTO_TCP ) {
	return WSAEINVAL;
    }

    //
    // Make sure that the output buffer is sufficiently large.
    //

    if ( *OptionLength < sizeof(int) ) {
	return WSAEFAULT;
    }

    //
    // Handle TCP-level options.
    //

    if ( Level == IPPROTO_TCP ) {
	       
	if ( OptionName != TCP_NODELAY ) {
	    return WSAEINVAL;
	}

	if ( context->SocketType == SOCK_DGRAM ) {
	    return WSAENOPROTOOPT;
	}

	ZeroMemory( OptionValue, *OptionLength );

	*OptionValue = context->NoDelay;
	*OptionLength = sizeof(int);

	return NO_ERROR;
    }

    //
    // Handle socket-level options.
    //

    switch ( OptionName ) {

    case SO_KEEPALIVE:

	if ( context->SocketType == SOCK_DGRAM ) {
	    return WSAENOPROTOOPT;
	}

	ZeroMemory( OptionValue, *OptionLength );

	*OptionValue = context->KeepAlive;
	*OptionLength = sizeof(int);

	break;

    case SO_DONTROUTE:

	ZeroMemory( OptionValue, *OptionLength );

	*OptionValue = context->DontRoute;
	*OptionLength = sizeof(int);

	break;

    default:

	return WSAENOPROTOOPT;
    }

    return NO_ERROR;

} // WSHGetSocketInformation


INT
WINAPI
WSHGetWildcardSockaddr (
    IN PVOID HelperDllSocketContext,
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength
    )

/*++

Routine Description:

    This routine returns a wildcard socket address.  A wildcard address
    is one which will bind the socket to an endpoint of the transport's
    choosing.  For TCP/IP, a wildcard address has IP address ==
    0.0.0.0 and port = 0.

Arguments:

    HelperDllSocketContext - the context pointer returned from
	WSHOpenSocket() for the socket for which we need a wildcard
	address.

    Sockaddr - points to a buffer which will receive the wildcard socket
	address.

    SockaddrLength - receives the length of the wioldcard sockaddr.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
	NO_ERROR if the operation succeeded.

--*/

{
    if ( *SockaddrLength < sizeof(SOCKADDR_IN) ) {
	return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_IN);

    //
    // Just zero out the address and set the family to AF_INET--this is 
    // a wildcard address for TCP/IP.  
    //

    ZeroMemory( Sockaddr, sizeof(SOCKADDR_IN) );

    Sockaddr->sa_family = AF_INET;

    return NO_ERROR;

} // WSAGetWildcardSockaddr


DWORD
WINAPI
WSHGetWinsockMapping (
    OUT PWINSOCK_MAPPING Mapping,
    IN DWORD MappingLength
    )

/*++

Routine Description:

    Returns the list of address family/socket type/protocol triples
    supported by this helper DLL.

Arguments:

    Mapping - receives a pointer to a WINSOCK_MAPPING structure that
	describes the triples supported here.

    MappingLength - the length, in bytes, of the passed-in Mapping buffer.

Return Value:

    DWORD - the length, in bytes, of a WINSOCK_MAPPING structure for this
	helper DLL.  If the passed-in buffer is too small, the return
	value will indicate the size of a buffer needed to contain
	the WINSOCK_MAPPING structure.

--*/

{
    DWORD mappingLength;

    mappingLength = sizeof(WINSOCK_MAPPING) - sizeof(MAPPING_TRIPLE) +
			sizeof(TcpMappingTriples) + sizeof(UdpMappingTriples);

    //
    // If the passed-in buffer is too small, return the length needed
    // now without writing to the buffer.  The caller should allocate
    // enough memory and call this routine again.
    //

    if ( mappingLength > MappingLength ) {
	return mappingLength;
    }

    //
    // Fill in the output mapping buffer with the list of triples
    // supported in this helper DLL.
    //

    Mapping->Rows = sizeof(TcpMappingTriples) / sizeof(TcpMappingTriples[0])
		     + sizeof(UdpMappingTriples) / sizeof(UdpMappingTriples[0]);
    Mapping->Columns = sizeof(MAPPING_TRIPLE) / sizeof(DWORD);
    MoveMemory(
	Mapping->Mapping,
	TcpMappingTriples,
	sizeof(TcpMappingTriples)
	);
    MoveMemory(
	(PCHAR)Mapping->Mapping + sizeof(TcpMappingTriples),
	UdpMappingTriples,
	sizeof(UdpMappingTriples)
	);

    //
    // Return the number of bytes we wrote.
    //

    return mappingLength;

} // WSHGetWinsockMapping


INT
WINAPI
WSHOpenSocket (
    IN OUT PINT AddressFamily,
    IN OUT PINT SocketType,
    IN OUT PINT Protocol,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PDWORD NotificationEvents
    )

/*++

Routine Description:

    Does the necessary work for this helper DLL to open a socket and is
    called by the winsock DLL in the socket() routine.  This routine
    verifies that the specified triple is valid, determines the NT
    device name of the TDI provider that will support that triple,
    allocates space to hold the socket's context block, and
    canonicalizes the triple.

Arguments:

    AddressFamily - on input, the address family specified in the
	socket() call.  On output, the canonicalized value for the
	address family.

    SocketType - on input, the socket type specified in the socket()
	call.  On output, the canonicalized value for the socket type.

    Protocol - on input, the protocol specified in the socket() call.
	On output, the canonicalized value for the protocol.

    TransportDeviceName - receives the name of the TDI provider that
	will support the specified triple.

    HelperDllSocketContext - receives a context pointer that the winsock
	DLL will return to this helper DLL on future calls involving
	this socket.

    NotificationEvents - receives a bitmask of those state transitions
	this helper DLL should be notified on.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
	NO_ERROR if the operation succeeded.

--*/

{
    PWSHTCPIP_SOCKET_CONTEXT context;

    //
    // Determine whether this is to be a TCP or UDP socket.
    //

    if ( IsTripleInList(
	     TcpMappingTriples,
	     sizeof(TcpMappingTriples) / sizeof(TcpMappingTriples[0]),
	     *AddressFamily,
	     *SocketType,
	     *Protocol ) ) {

	//
	// Return the canonical form of a TCP socket triple.
	//

	*AddressFamily = TcpMappingTriples[0].AddressFamily;
	*SocketType = TcpMappingTriples[0].SocketType;
	*Protocol = TcpMappingTriples[0].Protocol;

	//
	// Indicate the name of the TDI device that will service
	// SOCK_STREAM sockets in the internet address family.
	//

	RtlInitUnicodeString( TransportDeviceName, L"\\Device\\Streams\\Tcp" );

    } else if ( IsTripleInList(
		    UdpMappingTriples,
		    sizeof(UdpMappingTriples) / sizeof(UdpMappingTriples[0]),
		    *AddressFamily,
		    *SocketType,
		    *Protocol ) ) {

	//
	// Return the canonical form of a UDP socket triple.
	//

	*AddressFamily = UdpMappingTriples[0].AddressFamily;
	*SocketType = UdpMappingTriples[0].SocketType;
	*Protocol = UdpMappingTriples[0].Protocol;

	//
	// Indicate the name of the TDI device that will service
	// SOCK_DGRAM sockets in the internet address family.
	//

	RtlInitUnicodeString( TransportDeviceName, L"\\Device\\Streams\\Udp" );

    } else {

	//
	// This should never happen if the registry information about this
	// helper DLL is correct.  If somehow this did happen, just return
	// an error.
	//

	return WSAEINVAL;
    }

    //
    // Allocate context for this socket.  The Windows Sockets DLL will
    // return this value to us when it asks us to get/set socket options.
    //

    context = (PWSHTCPIP_SOCKET_CONTEXT) HeapAlloc(
										 GetProcessHeap(),
										 0,
										 sizeof(*context)
										 );
    if ( context == NULL ) {
	return WSAENOBUFS;
    }

    //
    // Initialize the context for the socket.
    //

    context->AddressFamily = *AddressFamily;
    context->SocketType = *SocketType;
    context->Protocol = *Protocol;
    context->ReceiveBufferSize = DEFAULT_RECEIVE_BUFFER_SIZE;
    context->KeepAlive = FALSE;
    context->DontRoute = FALSE;
    context->NoDelay = FALSE;

    //
    // Tell the Windows Sockets DLL which state transitions we're
    // interested in being notified of.  The only time we need to be
    // called is after a connect has completed so that we can turn on
    // the sending of keepalives if SO_KEEPALIVE was set before the
    // socket was connected.
    //

    *NotificationEvents = WSH_NOTIFY_CONNECT | WSH_NOTIFY_CLOSE;

    //
    // Everything worked, return success.
    //

    *HelperDllSocketContext = context;
    return NO_ERROR;

} // WSHOpenSocket


INT
WINAPI
WSHNotify (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN DWORD NotifyEvent
    )

/*++

Routine Description:

    This routine is called by the winsock DLL after a state transition
    of the socket.  Only state transitions returned in the
    NotificationEvents parameter of WSHOpenSocket() are notified here.
    This routine allows a winsock helper DLL to track the state of
    socket and perform necessary actions corresponding to state
    transitions.

Arguments:

    HelperDllSocketContext - the context pointer given to the winsock
	DLL by WSHOpenSocket().

    SocketHandle - the handle for the socket.

    TdiAddressObjectHandle - the TDI address object of the socket, if
	any.  If the socket is not yet bound to an address, then
	it does not have a TDI address object and this parameter
	will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
	if any.  If the socket is not yet connected, then it does not
	have a TDI connection object and this parameter will be NULL.

    NotifyEvent - indicates the state transition for which we're being
	called.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
	NO_ERROR if the operation succeeded.

--*/

{
    PWSHTCPIP_SOCKET_CONTEXT context = HelperDllSocketContext;
    INT err;

    //
    // We should only be called after a connect() completes or when the
    // socket is being closed.
    //

    if ( NotifyEvent == WSH_NOTIFY_CONNECT ) {

	//
	// SO_KEEPALIVE or TCP_NODELAY was set before the socket was 
	// connected.  Set it now.  
	//

	if ( context->KeepAlive ) {
	    err = DoTdiAction(
		      TdiConnectionObjectHandle,
		      TOL_TLI,
		      TO_KEEPALIVE,
		      TRUE
		      );
	    if ( err != NO_ERROR ) {
		return err;
	    }
	}

	if ( context->NoDelay ) {
	    err = DoTdiAction(
		      TdiConnectionObjectHandle,
		      TOL_TLI,
		      TO_NODELAY,
		      TRUE
		      );
	    if ( err != NO_ERROR ) {
		return err;
	    }
	}

	if ( context->ReceiveBufferSize != DEFAULT_RECEIVE_BUFFER_SIZE ) {
	    err = DoTdiAction(
		      TdiConnectionObjectHandle,
		      TOL_RCVBUF,
		      0,
		      context->ReceiveBufferSize
		      );
	    if ( err != NO_ERROR ) {
		return err;
	    }
	}

    } else if ( NotifyEvent == WSH_NOTIFY_CLOSE ) {

	//
	// Just free the socket context.
	//

	HeapFree( GetProcessHeap( ), 0, (LPSTR)HelperDllSocketContext );

    } else {

	return WSAEINVAL;
    }


    return NO_ERROR;

} // WSHNotify


INT
WINAPI
WSHSetSocketInformation (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN INT Level,
    IN INT OptionName,
    IN PCHAR OptionValue,
    IN INT OptionLength
    )

/*++

Routine Description:

    This routine sets information about a socket for those socket
    options supported in this helper DLL.  The options supported here
    are SO_KEEPALIVE and SO_DONTROUTE.  This routine is called by the
    winsock DLL when a level/option name combination is passed to
    setsockopt() that the winsock DLL does not understand.

Arguments:

    HelperDllSocketContext - the context pointer returned from
	WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're getting
	information.

    TdiAddressObjectHandle - the TDI address object of the socket, if
	any.  If the socket is not yet bound to an address, then
	it does not have a TDI address object and this parameter
	will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
	if any.  If the socket is not yet connected, then it does not
	have a TDI connection object and this parameter will be NULL.

    Level - the level parameter passed to setsockopt().

    OptionName - the optname parameter passed to setsockopt().

    OptionValue - the optval parameter passed to setsockopt().

    OptionLength - the optlen parameter passed to setsockopt().

Return Value:

    INT - a winsock error code indicating the status of the operation, or
	NO_ERROR if the operation succeeded.

--*/

{
    PWSHTCPIP_SOCKET_CONTEXT context = HelperDllSocketContext;
    INT error;
    INT optionValue;

    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );

    //
    // Check if this is an internal request for context information.
    //

    if ( Level == SOL_INTERNAL && OptionName == SO_CONTEXT ) {

	//
	// The Windows Sockets DLL is requesting that we set context 
	// information for a new socket.  If the new socket was 
	// accept()'ed, then we have already been notified of the socket 
	// and HelperDllSocketContext will be valid.  If the new socket 
	// was inherited or duped into this process, then this is our 
	// first notification of the socket and HelperDllSocketContext 
	// will be equal to NULL.  
	//
	// Insure that the context information being passed to us is 
	// sufficiently large.  
	//

	if ( OptionLength < sizeof(*context) ) {
	    return WSAEINVAL;
	}

	if ( HelperDllSocketContext == NULL ) {
	    
	    //
	    // This is our notification that a socket handle was 
	    // inherited or duped into this process.  Allocate a context 
	    // structure for the new socket.  
	    //
    
	    context = (PWSHTCPIP_SOCKET_CONTEXT) HeapAlloc(
											GetProcessHeap(),
											0,
											sizeof(*context)
											);
	    if ( context == NULL ) {
		return WSAENOBUFS;
	    }
    
	    //
	    // Copy over information into the context block.
	    //
    
	    CopyMemory( context, OptionValue, sizeof(*context) );
    
	    //
	    // Tell the Windows Sockets DLL where our context information is 
	    // stored so that it can return the context pointer in future 
	    // calls.  
	    //
    
	    *(PWSHTCPIP_SOCKET_CONTEXT *)OptionValue = context;
    
	    return NO_ERROR;

	} else {

	    PWSHTCPIP_SOCKET_CONTEXT parentContext;
	    INT one = 1;

	    //
	    // The socket was accept()'ed and it needs to have the same 
	    // properties as it'sw parent.  The OptionValue buffer 
	    // contains the context information of this socket's parent.  
	    //

	    parentContext = (PWSHTCPIP_SOCKET_CONTEXT)OptionValue;

	    assert( context->AddressFamily == parentContext->AddressFamily );
	    assert( context->SocketType == parentContext->SocketType );
	    assert( context->Protocol == parentContext->Protocol );

	    //
	    // Turn on in the child any options that have been set in
	    // the parent.
	    //

	    if ( parentContext->KeepAlive ) {

		error = WSHSetSocketInformation(
			    HelperDllSocketContext,
			    SocketHandle,
			    TdiAddressObjectHandle,
			    TdiConnectionObjectHandle,
			    SOL_SOCKET,
			    SO_KEEPALIVE,
			    (PCHAR)&one,
			    sizeof(one)
			    );
		if ( error != NO_ERROR ) {
		    return error;
		}
	    }

	    if ( parentContext->DontRoute ) {

		error = WSHSetSocketInformation(
			    HelperDllSocketContext,
			    SocketHandle,
			    TdiAddressObjectHandle,
			    TdiConnectionObjectHandle,
			    SOL_SOCKET,
			    SO_DONTROUTE,
			    (PCHAR)&one,
			    sizeof(one)
			    );
		if ( error != NO_ERROR ) {
		    return error;
		}
	    }

	    if ( parentContext->NoDelay ) {

		error = WSHSetSocketInformation(
			    HelperDllSocketContext,
			    SocketHandle,
			    TdiAddressObjectHandle,
			    TdiConnectionObjectHandle,
			    IPPROTO_TCP,
			    TCP_NODELAY,
			    (PCHAR)&one,
			    sizeof(one)
			    );
		if ( error != NO_ERROR ) {
		    return error;
		}
	    }

	    if ( parentContext->ReceiveBufferSize != DEFAULT_RECEIVE_BUFFER_SIZE ) {

		error = WSHSetSocketInformation(
			    HelperDllSocketContext,
			    SocketHandle,
			    TdiAddressObjectHandle,
			    TdiConnectionObjectHandle,
			    SOL_SOCKET,
			    SO_RCVBUF,
			    (PCHAR)&parentContext->ReceiveBufferSize,
			    sizeof(parentContext->ReceiveBufferSize)
			    );
		if ( error != NO_ERROR ) {
		    return error;
		}
	    }

	    return NO_ERROR;
	}
    }

    //
    // The only other levels we support here are SOL_SOCKET and 
    // IPPROTO_TCP.  
    //

    if ( Level != SOL_SOCKET && Level != IPPROTO_TCP ) {
	return WSAEINVAL;
    }

    //
    // Make sure that the option length is sufficient.
    //

    if ( OptionLength < sizeof(int) ) {
	return WSAEFAULT;
    }

    optionValue = *(PINT)OptionValue;

    //
    // Handle TCP-level options.
    //

    if ( Level == IPPROTO_TCP ) {
	       
	if ( OptionName != TCP_NODELAY ) {
	    return WSAEINVAL;
	}

	if ( context->SocketType == SOCK_DGRAM ) {
	    return WSAENOPROTOOPT;
	}

	//
	// Atempt to turn on or off Nagle's algorithm, as necessary.
	//

	if ( !context->NoDelay && optionValue != 0 ) {

	    //
	    // NoDelay is currently off and the application wants to 
	    // turn it on.  If the TDI connection object handle is NULL, 
	    // then the socket is not yet connected.  In this case we'll 
	    // just remember that the no delay option was set and 
	    // actually turn them on in WSHNotify() after a connect() 
	    // has completed on the socket.  
	    //

	    if ( TdiConnectionObjectHandle != NULL ) {
		error = DoTdiAction(
			    TdiConnectionObjectHandle,
			    TOL_TLI,
			    TO_NODELAY,
			    TRUE
			    );
		if ( error != NO_ERROR ) {
		    return error;
		}
	    }

	    //
	    // Remember that no delay is enabled for this socket.
	    //

	    context->NoDelay = TRUE;

	} else if ( context->NoDelay && optionValue == 0 ) {

	    //
	    // No delay is currently enabled and the application wants 
	    // to turn it off.  If the TDI connection object is NULL, 
	    // the socket is not yet connected.  In this case we'll just 
	    // remember that nodelay is disabled.  
	    //

	    if ( TdiConnectionObjectHandle != NULL ) {
		error = DoTdiAction(
			    TdiConnectionObjectHandle,
			    TOL_TLI,
			    TO_NODELAY,
			    FALSE
			    );
		if ( error != NO_ERROR ) {
		    return error;
		}
	    }

	    //
	    // Remember that no delay is disabled for this socket.
	    //

	    context->NoDelay = FALSE;
	}

	return NO_ERROR;
    }

    //
    // Handle socket-level options.
    //

    switch ( OptionName ) {

    case SO_KEEPALIVE:

	//
	// Atempt to turn on or off keepalive sending, as necessary.
	//

	if ( context->SocketType == SOCK_DGRAM ) {
	    return WSAENOPROTOOPT;
	}

	if ( !context->KeepAlive && optionValue != 0 ) {

	    //
	    // Keepalives are currently off and the application wants to
	    // turn them on.  If the TDI connection object handle is
	    // NULL, then the socket is not yet connected.  In this case
	    // we'll just remember that the keepalive option was set and
	    // actually turn them on in WSHNotify() after a connect()
	    // has completed on the socket.
	    //

	    if ( TdiConnectionObjectHandle != NULL ) {
		error = DoTdiAction(
			    TdiConnectionObjectHandle,
			    TOL_TLI,
			    TO_KEEPALIVE,
			    TRUE
			    );
		if ( error != NO_ERROR ) {
		    return error;
		}
	    }

	    //
	    // Remember that keepalives are enabled for this socket.
	    //

	    context->KeepAlive = TRUE;

	} else if ( context->KeepAlive && optionValue == 0 ) {

	    //
	    // Keepalives are currently enabled and the application
	    // wants to turn them off.  If the TDI connection object is
	    // NULL, the socket is not yet connected.  In this case
	    // we'll just remember that keepalives are disabled.
	    //

	    if ( TdiConnectionObjectHandle != NULL ) {
		error = DoTdiAction(
			    TdiConnectionObjectHandle,
			    TOL_TLI,
			    TO_KEEPALIVE,
			    FALSE
			    );
		if ( error != NO_ERROR ) {
		    return error;
		}
	    }

	    //
	    // Remember that keepalives are disabled for this socket.
	    //

	    context->KeepAlive = FALSE;
	}

	break;

    case SO_DONTROUTE:

	//
	// We don't really support SO_DONTROUTE.  Just remember that the
	// option was set or unset.
	//

	if ( optionValue != 0 ) {
	    context->DontRoute = TRUE;
	} else if ( optionValue == 0 ) {
	    context->DontRoute = FALSE;
	}

	break;

    case SO_RCVBUF:

	//
	// If the receive buffer size is being changed, tell TCP about 
	// it.  Do nothing if this is a datagram.  
	//

	if ( context->ReceiveBufferSize == optionValue ||
		 context->SocketType == SOCK_DGRAM ) {
	    break;
	}

	//
	// We need to pass the new receive buffer size to the TCP 
	// transport.  If the TDI connection object is NULL, the socket 
	// is not yet connected.  In this case we'll just remember the 
	// new buffer size and set it after the socket is connected.  
	//

	if ( TdiConnectionObjectHandle != NULL ) {
	    error = DoTdiAction(
			TdiConnectionObjectHandle,
			TOL_RCVBUF,
			0,
			optionValue
			);
	    if ( error != NO_ERROR ) {
		return error;
	    }
	}

	context->ReceiveBufferSize = optionValue;

	break;

    default:

	return WSAENOPROTOOPT;
    }

    return NO_ERROR;

} // WSHSetSocketInformation


BOOLEAN
IsTripleInList (
    IN PMAPPING_TRIPLE List,
    IN ULONG ListLength,
    IN INT AddressFamily,
    IN INT SocketType,
    IN INT Protocol
    )

/*++

Routine Description:

    Determines whether the specified triple has an exact match in the
    list of triples.

Arguments:

    List - a list of triples (address family/socket type/protocol) to
	search.

    ListLength - the number of triples in the list.

    AddressFamily - the address family to look for in the list.

    SocketType - the socket type to look for in the list.

    Protocol - the protocol to look for in the list.

Return Value:

    BOOLEAN - TRUE if the triple was found in the list, false if not.

--*/

{
    ULONG i;

    //
    // Walk through the list searching for an exact match.
    //

    for ( i = 0; i < ListLength; i++ ) {

	//
	// If all three elements of the triple match, return indicating
	// that the triple did exist in the list.
	//

	if ( AddressFamily == List[i].AddressFamily &&
	     SocketType == List[i].SocketType &&
	     Protocol == List[i].Protocol ) {
	    return TRUE;
	}
    }

    //
    // The triple was not found in the list.
    //

    return FALSE;

} // IsTripleInList


INT
DoTdiAction (
    IN HANDLE TdiConnectionObjectHandle,
    IN INT OptionLevel,
    IN INT OptionName,
    IN INT OptionalData
    )

/*++

Routine Description:

    Performs a TDI action to the TCP/IP driver.  A TDI action translates
    into a streams T_OPTMGMT_REQ.

Arguments:

    TdiConnectionObjectHandle - a TDI connection object on which to perform
	the TDI action.

    OptionLevel - the opt_level to pass in the TOPT structure.

    OptionName - The TPI option to set.

    OptionalData - data to put in the opt_data field of the TOPT
	structure.

Return Value:

    INT - NO_ERROR, or a Windows Sockets error code.

--*/

{
    BOOLEAN deviceIoResult;
    PSTREAMS_TDI_ACTION tdiAction;
    ULONG tdiActionLength;
    TOPT *tpiOptions;
    ULONG outputLength;

    //
    // Allocate space to hold the TDI action buffer and the IO status
    // block.  These cannot be stack variables in case we must return
    // before the operation is complete.
    //

    tdiActionLength = sizeof(*tdiAction) + sizeof(*tpiOptions) -
			  sizeof(tdiAction->Buffer[0]);

    tdiAction = (PSTREAMS_TDI_ACTION) HeapAlloc(
							      GetProcessHeap( ),
							      0,
							      tdiActionLength
							      );
    if ( tdiAction == NULL ) {
	return WSAENOBUFS;
    }

    //
    // Initialize the TDI action buffer.
    //

    tdiAction->BufferLength = sizeof(*tpiOptions);

    tpiOptions = (TOPT *)tdiAction->Buffer;
    tpiOptions->opt_level = OptionLevel;
    tpiOptions->opt_name = OptionName;
    tpiOptions->opt_data[0] = OptionalData;

    //
    // Make the actual TDI action call.  The Streams TDI mapper will 
    // translate this into a TPI option management request for us and 
    // give it to TCP/IP.  
    //

    deviceIoResult = DeviceIoControl(
				 TdiConnectionObjectHandle,
				 IOCTL_TDI_ACTION,
				 tdiAction,
				 tdiActionLength,
				 tdiAction,
				 tdiActionLength,
				 &outputLength,
				 NULL
				 );

    HeapFree( GetProcessHeap( ), 0, (LPSTR)tdiAction );

    return deviceIoResult ? NO_ERROR : WSAENOBUFS;

} // DoTdiAction
