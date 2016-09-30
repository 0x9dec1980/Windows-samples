/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    packet.c

Abstract:


Author:


Environment:

    Kernel mode only.

Notes:


Future:



Revision History:

--*/


#if DBG

#define IF_PACKETDEBUG(f) if (PacketDebugFlag & (f))
extern ULONG PacketDebugFlag;

#define PACKET_DEBUG_LOUD               0x00000001  // debugging info
#define PACKET_DEBUG_VERY_LOUD          0x00000002  // excessive debugging info

#define PACKET_DEBUG_INIT               0x00000100  // init debugging info

//
// Macro for deciding whether to dump lots of debugging information.
//

#define IF_LOUD(A) IF_PACKETDEBUG( PACKET_DEBUG_LOUD ) { A }
#define IF_VERY_LOUD(A) IF_PACKETDEBUG( PACKET_DEBUG_VERY_LOUD ) { A }
#define IF_INIT_LOUD(A) IF_PACKETDEBUG( PACKET_DEBUG_INIT ) { A }

#else

#define IF_LOUD(A)
#define IF_VERY_LOUD(A)
#define IF_INIT_LOUD(A)

#endif
