/*++

Copyright (C) 1995  Microsoft Corporation

Module Name:

   Delay.h

Abstract:

   This module contains the structure prototype for the parameters that is
   used to test KeDelayExecutionThread.

Author:

   Mark A. Overby (MarkOv)  12-April-1995

Enviroment:

   Kernel Mode Only

Revision History:

--*/

#ifndef __DELAYHDR__
#define __DELAYHDR__

#include "devioctl.h"

typedef struct _DELAY_PARAMETERS
{
   LARGE_INTEGER tickMultiple ;
   LARGE_INTEGER beginCount ;
   LARGE_INTEGER endCount ;
   LARGE_INTEGER status ;
} DELAY_PARMS, *PDELAY_PARMS ;

#define IOCTL_CHECK_DELAY \
   CTL_CODE (FILE_DEVICE_UNKNOWN, 0xAA0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif // __DELAYHDR__

