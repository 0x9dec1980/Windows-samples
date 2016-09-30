/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dl_proto.h

Abstract:

    This module defines some of the data types and manifests for the
    Spider Generic Ethernet Interface.

Author:

    Eric Chin (ericc)           August 9, 1991

Revision History:

--*/
/*
 *  Spider STREAMS Data Link Interface Primitives
 *
 *  Copyright (c) 1989  Spider Systems Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  All rights reserved.
 *
 *  Written by Mark Valentine
 *
 *  Made in Scotland.
 *
 *	 /usr/projects/spare/PBRAIN/SCCS/pbrainG/dev/src/include/sys/snet/0/s.dl_proto.h
 *	@(#)dl_proto.h	1.9
 *
 *	Last delta created	08:50:12 1/8/90
 *	This file extracted	08:53:40 7/10/91
 *
 *	Modifications:
 *
 *
 */
#ifndef DL_PROTO_INCLUDED
#define DL_PROTO_INCLUDED

#include <sys\snet\uint.h>
#include <ntdef.h>

/*
 *  This defines Version 2 of Spider's STREAMS Data Link protocol.
 *  Its main feature is its ability to cope with hardware addresses
 *  of length not equal to 6.
 */

/*
 *  Primitive type values.
 */

/* note: may also need DL_VERSION_0, etc. for parcpp to cope (bds) */

#define DATAL_TYPE	'R'		/* data link registration */ 
#define DATAL_VERSION0	DATAL_TYPE	/* Version 0 response */
#define DATAL_RESPONSE	'D'		/* Version > 0 response */
#define DATAL_PARAMS	'P'		/* data link parameters */
#define DATAL_TX	't'		/* packet for transmission */
#define DATAL_RX	'r'		/* incoming packet */

/*
 *  Data Link registration.
 *
 *  Currently, the DATAL_TYPE message is always the first to be sent to
 *  the Data Link driver.  The Data Link driver responds either with a
 *  DATAL_RESPONSE message, or with a DATAL_VERSION0 message (this is the
 *  case for older drivers).
 *
 *  The upper user must remember the version number of the protocol
 *  returned in the DATAL_RESPONSE response, or assume version 0 in the
 *  case of an DATAL_VERSION0 response, since the version number is not
 *  currently guaranteed set in other returned messages.
 */

typedef struct datal_response {
	uint8 prim_type;		/* i.e. DATAL_RESPONSE */
	uint8 version;			/* protocol version number == 0 */
	uint8 hw_type;			/* hardware type (host byte order) */
	uint8 addr_len;			/* hardware address length */
	uint16 lwb;			/* lower bound of type range */
	uint16 upb;			/* upper bound of type range */
	uint16 frgsz;			/* max. packet size on net */
	uint8 addr[1];			/* hardware address (variable length) */
} S_DATAL_RESPONSE;

typedef struct datal_type {
	uint8 prim_type;		/* i.e. DATAL_TYPE/ETH_TYPE */
	uint8 version;			/* *assume* version == 0 */
	uint16 pad;			/* compatibility */
	uint16 lwb;			/* lower bound of type range */
	uint16 upb;			/* upper bound of type range */
	uint16 frgsz;			/* max. packet size on net */
	int	(*should_put) (		/* this is the function to call to */
		queue_t*,		/*  see if a packet should be passed */
		char *,			/*  upstream */
		int,
                char *,
                int);
	uint8 addr[1];			/* data link address */
} S_DATAL_TYPE;

/*
 *  Hardware types.
 */

/* these happen to agree with the values used by ARP (see RFC XXXX) */

#define HW_ETHERNET	1		/* Ethernet Version 2 */
#define HW_TOKENRING    4               /* Proteon ProNET Token ring */
#define HW_IEEE802      6               /* IEEE 802 Networks */
#define HW_ARCNET       7               /* Arcnet networks */

typedef struct datal_params {
	uint8 prim_type;		/* i.e. DATAL_PARAMS */
	uint8 version;			/* protocol version (unreliable) */
	uint8 hw_type;			/* hardware type (host byte order) */
	uint8 addr_len;			/* hardware address length */
	uint16 frgsz;			/* max. packet size on net */
	uint8 addr[1];			/* hardware address (variable length) */
} S_DATAL_PARAMS;

typedef struct datal_rx {
	uint8 prim_type;		/* i.e. DATAL_RX */
	uint8 version;			/* protocol version (unreliable) */
	uint16 pad;			/* compatibility */
	uint8 route_length;		/* size of optional routing field */
	uint8 addr_length;		/* size of source or dest address */
	uint16 type;			/* data link type field, */
					/*   (length in 802 case) */
	uint8 src[1];			/* source hardware address, */
	 				/*   followed by the destination and */
					/*   optional routing information */
} S_DATAL_RX;

typedef struct datal_tx {
	uint8 prim_type;		/* i.e. DATAL_TX */
	uint8 version;			/* protocol version (unreliable) */
	uint16 pad;			/* compatibility */
	uint8 route_length;		/* size of optional routing field */
	uint8 addr_length;		/* size of destination address */
	uint16 type;			/* data link type field */
 					/* treat type as length in 802 case */
	uint8 dst[1];			/* destination hardware address */
} S_DATAL_TX;

typedef struct datal_bind {
	uint8 	prim_type;
	STRING	adapter_name;		/* adapter driver name */
	char	buffer[80];		/* buffer for the adapter name */
} S_DATAL_BIND;

/*
 *  Generic data link protocol primitive.
 */

typedef union datal_proto
{
	uint8 type;
	struct datal_type dltype;
	struct datal_params dlparm;
	struct datal_rx dlrx;
	struct datal_tx dltx;
	struct datal_bind dlbind;
	struct datal_response dlresp;
} S_DATAL_PROTO;

#endif //DL_PROTO_INCLUDED
