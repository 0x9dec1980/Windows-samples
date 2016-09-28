/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ex_bot.c

Abstract:

    This source file implements the bottom exit module for a subsystem-parallel
    (ie. non mp-safe) STREAMS stack that sits atop a FULL_PARALLEL module.

    All downstream messages that arrive at this module are queued up, to be
    serviced by our write service procedure, inswsrv().  Note that this
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
    EXBOT_STID, EXBOT_NAME, 0,      INFPSZ, 0x7fff, 0x7fff,
};


//
//  Queue Initialization Templates
//  ------------------------------
//  put     service open    close   future  minfo   future  subsystem
//
static
struct qinit urinit = {
    putnext,NULL,   noopen, noclose,NULL,  &minfo,  NULL,   FULL_PARALLEL
};

static
struct qinit uwinit = {
    putq,   inswsrv,NULL,   NULL,   NULL,  &minfo,  NULL,   FULL_PARALLEL
};


//
//  Streamtab Entry
//  ---------------
//  upper queue pair    lower queue pair
//
struct streamtab exbotinfo = {
    &urinit, &uwinit,   NULL, NULL
};
