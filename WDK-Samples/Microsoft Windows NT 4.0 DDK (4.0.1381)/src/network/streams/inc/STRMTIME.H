/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nttime.h

Abstract:

    This file contains a macro to emulate the UNIX time variable. This macro
    is contained in a separate header file so that it can be selectively
    included in only those files that need it. The macro definition causes
    conflicts with local variables and structure fields.

Author:

    Mike Massa (mikemas)           August 9, 1991

Revision History:

--*/

#ifndef _NTTIME_
#define _NTTIME_


#define time StrmQuerySecondsSince1970Time()


#endif  /* def _NTTIME_  */

