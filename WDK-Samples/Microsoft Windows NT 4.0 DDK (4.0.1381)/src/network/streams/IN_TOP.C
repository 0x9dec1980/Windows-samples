/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    in_top.c

Abstract:

    This source file implements the top insulating module for the
    subsystem-parallel (ie. non mp-safe) TCP/IP stack.

    To customize this bottom insulator for a another subsystem-parallel
    stack, replace TCPIP_SUBSYSTEM_ID with that stack's subsystem id.

    All downstream messages that arrive at this module are queued up, to be
    serviced by our write service procedure, inswsrv().

--*/

#include "insulate.h"



//
//  Queue Information Structures
//  ----------------------------
//  Each queue has an minfo structure.
//
//  id          name        min     max     hi      lo
//
static
struct module_info minfo = {
    INSTOP_STID,INSTOP_NAME,0,      INFPSZ, 0x7fff, 0x7fff,
};


//
//  Queue Initialization Templates
//  ------------------------------
//  put     service open    close   future  minfo   future  subsystem
//
static
struct qinit urinit = {
    putnext,NULL,   noopen, noclose,NULL,  &minfo,  NULL,   TCPIP_SUBSYSTEM_ID
};

static
struct qinit uwinit = {
    putq,   inswsrv,NULL,   NULL,   NULL,  &minfo,  NULL,   TCPIP_SUBSYSTEM_ID
};


//
//  Streamtab Entry
//  ---------------
//  upper queue pair    lower queue pair
//
struct streamtab intopinfo = {
    &urinit, &uwinit,   NULL, NULL
};
