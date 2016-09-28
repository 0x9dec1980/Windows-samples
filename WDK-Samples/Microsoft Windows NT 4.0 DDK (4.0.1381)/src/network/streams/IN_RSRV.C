/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    in_rsrv.c

Abstract:

    This source file contains the read service procedure that is used by
    the insulating modules.

--*/

#include "insulate.h"





int
insrsrv(
    IN queue_t *rq
    )

/*++

Routine Description:

    This is the read service procedure of the insulating STREAMS module.

    It is based on the example on Unix SVR4 Programmers' Guide: STREAMS
    (page 8-7).

Arguments:

    rq      -  queue whose service procedure is being run.

Return Value:

    the constant 1.  STREAMS does not specify what the return values
    from put and service procedures should be.

--*/

{
    mblk_t *mp;

    while (mp = getq(rq)) {

        switch (mp->b_datap->db_type) {
        case M_FLUSH:
            if (*(mp->b_rptr) & FLUSHR) {
                flushq(rq, FLUSHDATA);
            }
            putnext(rq, mp);
            return(1);

        //
        // high-priority messages are not subject to flow-control.
        //
        default:
            if ((mp->b_datap->db_type >= QPCTL) ||
                 canput(rq->q_next)) {
                putnext(rq, mp);
                continue;
            }
            putbq(rq, mp);
            return(1);
        }
    }
    return(1);

} // insrsrv
