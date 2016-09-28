/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    datavga.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the VGA Extensible Objects.

    This file contains a set of constant data structures which are
    currently defined for the VGA Extensible Objects.  This is an
    example of how other such objects could be defined.

Created:

    Russ Blake  26 Feb 93

Revision History:

    None.

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include "vgactrnm.h"
#include "datavga.h"

//
//  Constant structure initializations 
//      defined in datavga.h
//

VGA_DATA_DEFINITION VgaDataDefinition = {

    {	sizeof(VGA_DATA_DEFINITION) + SIZE_OF_VGA_PERFORMANCE_DATA,
	sizeof(VGA_DATA_DEFINITION),
    sizeof(PERF_OBJECT_TYPE),
    VGAOBJ,
    0,
	VGAOBJ,
    0,
	PERF_DETAIL_NOVICE,
	(sizeof(VGA_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
	0,
    0,
    0
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
	BITBLTS,
    0,
	BITBLTS,
    0,
    0,
	PERF_DETAIL_NOVICE,
	PERF_COUNTER_COUNTER,
        sizeof(DWORD),
	NUM_BITBLTS_OFFSET
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
	TEXTOUTS,
    0,
	TEXTOUTS,
    0,
    0,
	PERF_DETAIL_NOVICE,
	PERF_COUNTER_COUNTER,
        sizeof(DWORD),
	NUM_TEXTOUTS_OFFSET
    }
};
