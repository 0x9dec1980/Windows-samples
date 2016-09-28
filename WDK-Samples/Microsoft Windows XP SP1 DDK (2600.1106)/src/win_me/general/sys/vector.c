/***************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

	vector.c

Abstract:

	This file contains pointers to routines that can be called from the
	template driver. The intention is that the developer will supply an
	OpenDriver routine that will fill in these function pointers as
	needed. This routine will be called from the DriverEntry routine.
	All of the function pointers are initialized to NULL, so just fill
	in the ones that you need. See vector.h for typedefs for called routines.
	This also includes pointers to the driver name and version.

Environment:

	kernel mode only

Notes:

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

	Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

	10-26-96 : created

Author:

	Tom Green

	
****************************************************************************/


#include <wdm.h>
#include "debugwdm.h"


/*
 * pointers to routines in driver, filled in for new drivers
 * you must supply a routine called InitDriverVectors in your device
 * specific file
 */

VECTOR_DRIVERENTRY		VectorDriverEntry		= NULL;
VECTOR_DISPATCH			VectorDispatch			= NULL;
VECTOR_UNLOAD			VectorUnload			= NULL;
VECTOR_ADDDEVICE		VectorAddDevice			= NULL;
VECTOR_REMOVEDEVICE		VectorRemoveDevice		= NULL;
VECTOR_CREATE			VectorCreate			= NULL;
VECTOR_CLOSE			VectorClose				= NULL;
VECTOR_WRITE			VectorWrite				= NULL;
VECTOR_READ				VectorRead				= NULL;


// name of driver and driver version

PCHAR					DriverName				= NULL;
PCHAR					DriverVersion			= NULL;

