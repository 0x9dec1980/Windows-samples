/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ex_top.c

Abstract:

    This source file implements the top exit module for a subsystem-parallel
    (ie. non mp-safe) STREAMS stack that resides below either the stream
    head or a FULL_PARALLEL module/driver.

    All upstream messages that arrive at this module are queued up, to be
    serviced by our read service procedure, insrsrv().  Note that this
    module is declared to be FULL_PARALLEL.

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
    EXTOP_STID, EXTOP_NAME, 0,      INFPSZ, 0x7fff, 0x7fff,
};


//
//  Queue Initialization Templates
//  ------------------------------
//  put     service open    close   future  minfo   future  subsystem
//
static
struct qinit urinit = {
    putq   ,insrsrv,noopen, noclose,NULL,  &minfo,  NULL,   FULL_PARALLEL
};

static
struct qinit uwinit = {
    putnext,NULL,   NULL,   NULL,   NULL,  &minfo,  NULL,   FULL_PARALLEL
};


//
//  Streamtab Entry
//  ---------------
//  upper queue pair    lower queue pair
//
struct streamtab extopinfo = {
    &urinit, &uwinit,   NULL, NULL
};
