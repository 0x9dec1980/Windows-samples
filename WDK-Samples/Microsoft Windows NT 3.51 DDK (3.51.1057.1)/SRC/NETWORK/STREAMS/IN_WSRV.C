/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    in_wsrv.c

Abstract:

    This source file contains the write service procedure that is used by
    the insulating modules.

--*/

#include "insulate.h"





int
inswsrv(
    IN queue_t *wq
    )

/*++

Routine Description:

    This is a write service procedure that simply dequeues and passes
    messages to the next downstream STREAMS module/driver, subject to
    normal flow-control convention.

    It is based on the example in Unix SVR4 Programmers' Guide: STREAMS
    (page 8-7).

Arguments:

    wq      -  queue whose service procedure is being run.

Return Value:

    the constant 1.  STREAMS does not specify what the return values
    from put and service procedures should be.

--*/

{
    mblk_t *mp;

    while (mp = getq(wq)) {

        switch (mp->b_datap->db_type) {
        case M_FLUSH:
            if (*(mp->b_rptr) & FLUSHW) {
                flushq(wq, FLUSHDATA);
            }
            putnext(wq, mp);
            return(1);

        //
        // high-priority messages are not subject to flow-control.
        //
        default:
            if ((mp->b_datap->db_type >= QPCTL) ||
                 canput(wq->q_next)) {
                    putnext(wq, mp);
                    continue;
            }
            putbq(wq, mp);
            return(1);
        }
    }
    return(1);

} // inswsrv
