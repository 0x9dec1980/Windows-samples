/******************************Module*Header*******************************\
* Module Name: precomp.h
*
* Common headers used throughout the display driver.  This entire include
* file will typically be pre-compiled.
*
* Copyright (c) 1994-1995 Microsoft Corporation
* Copyright (c) 1994-1995 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include <stddef.h>
#include <windef.h>
#include <windows.h>
#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ntsdexts.h>
#include <ioaccess.h>
#include <limits.h>
#include <rx.h>

#include "lines.h"
#include "debug.h"
#include "driver.h"
#include "hw.h"
