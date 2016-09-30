/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   sysmacro.h

Abstract:

   This header file defines the device number macros for STREAMS drivers
   and modules.  It is derived from the SpiderSTREAMS source, sysmacros.h.

Author:

   Eric Chin            November 1, 1991

Revision History:

--*/

/* @(#)sysmacros.h      1.3 (Spider) 91/07/01 */

#ifndef _SYSMACRO_
#define _SYSMACRO_



/*
 * Selectors and Constructor for Device Numbers
 *
 * On NT, dev_t is an unsigned long, not an unsigned short (ref:
 * \nt\public\sdk\inc\sys\posix\sys\types.h).  Hence, minor device
 * numbers are defined to be 16 bits wide, giving 65536 possible
 * minor device numbers.
 *
 */
#define major(x)            ((unsigned) (x) >> 16)
#define minor(x)            ((x) & 0xffff)
#define makedev(x, y)       (dev_t) (((x) << 16) | (y))

#define getmajor(x)         major((x))
#define getminor(x)         minor((x))
#define makedevice(x, y)    makedev((x), (y))

#define CLONEDEV            0xffff


#ifndef min
#define min(a, b)           ((a) > (b) ? (b) : (a))
#endif

#ifndef max
#define max(a, b)           ((a) < (b) ? (b) : (a))
#endif

#endif // _SYSMACRO_
