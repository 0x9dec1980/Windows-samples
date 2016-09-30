/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    in_strm.c

Abstract:

    This source file contains the STREAMS procedures common to all the
    insulating modules.

--*/

#include "insulate.h"




int
noclose(
    IN queue_t *rq,
    IN int      flag,
    IN void    *credp
    )

/*++

Routine Description:

    This is a STREAMS module's close procedure.  STREAMS probably requires
    that every module have one.

Arguments:

    rq      -  the read queue of the stream being closed.
    flag    -  open(2) flag

Return Value:

    0 if successful, or the errno otherwise.

--*/

{
    return(0);

} // noclose



int
noopen(
    IN queue_t *rq,
    IN dev_t   *dev,
    IN int      flag,
    IN int      sflag,
    IN void    *credp
    )

/*++

Routine Description:

    This is a STREAMS module's open procedure.  STREAMS probably requires
    that every module have one.

Arguments:

    rq      -  read queue of the stream being opened.
    dev     -  device number of this driver
    flag    -  open(2) flag
    sflag   -  CLONEOPEN or 0

Return Value:

    0 if successful, or the errno otherwise.

--*/

{
    return(0);

} // noopen
