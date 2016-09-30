/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    insulate.h

Abstract:

    This is the private (internal) header file for the insulator modules for
    subsystem-parallel STREAMS stacks.

    Note that the each module has to be customized for each subsystem-parallel
    stack: the subsystem id of each module has to be changed.

--*/
#ifndef _INSULATE_
#define _INSULATE_


//
// NT "System" Headers
//
#include    <ntddk.h>


//
// Regular STREAMS "System" Headers
//
#include    <sys/types.h>
#include    <stream.h>
#include    <stropts.h>
#include    <sysmacro.h>
#include    <errno.h>




//
// Private (internal) Constant Definitions
//
#define EXTOP_STID                      11          /* module id */
#define EXTOP_NAME                      "extop"     /* module name */

#define INSTOP_STID                     12          /* module id */
#define INSTOP_NAME                     "instop"    /* module name */

#define INSBOT_STID                     23          /* module id */
#define INSBOT_NAME                     "insbot"    /* module name */

#define EXBOT_STID                      24          /* module id */
#define EXBOT_NAME                      "exbot"     /* module name */

#define TCPIP_SUBSYSTEM_ID              32768


//
// Private (internal) Variables
//
extern struct streamtab exbotinfo;
extern struct streamtab extopinfo;
extern struct streamtab inbotinfo;
extern struct streamtab intopinfo;



//
// STREAMS Procedure Declarations
// ------------------------------
// A STREAMS module has an open and a close procedure.  Each module has two
// queues: a read queue and a write queue.  Each queue must have a put
// procedure, and may have a service procedure.
//
//
int
insrsrv(
    IN queue_t *rq
    );

int
inswsrv(
    IN queue_t *wq
    );

int
noclose(
    IN queue_t *rq,
    IN int      flag,
    IN void    *credp
    );

int
noopen(
    IN queue_t *rq,
    IN dev_t   *dev,
    IN int      flag,
    IN int      sflag,
    IN void    *credp
    );



#endif // _INSULATE_
