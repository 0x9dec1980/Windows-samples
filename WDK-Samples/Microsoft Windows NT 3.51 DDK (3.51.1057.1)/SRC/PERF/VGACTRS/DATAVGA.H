/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

      datavga.h

Abstract:

    Header file for the VGA Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

   Russ Blake  02/24/93

Revision History:


--*/

#ifndef _DATAVGA_H_
#define _DATAVGA_H_

/****************************************************************************\
								   18 Jan 92
								   russbl

           Adding a Counter to the Extensible Objects Code



1.  Modify the object definition in extdata.h:

    a.	Add a define for the offset of the counter in the
	data block for the given object type.

    b.	Add a PERF_COUNTER_DEFINITION to the <object>_DATA_DEFINITION.

2.  Add the Titles to the Registry in perfctrs.ini and perfhelp.ini:

    a.	Add Text for the Counter Name and the Text for the Help.

    b.	Add them to the bottom so we don't have to change all the
        numbers.

    c.  Change the Last Counter and Last Help entries under
        PerfLib in software.ini.

    d.  To do this at setup time, see section in pmintrnl.txt for
        protocol.

3.  Now add the counter to the object definition in extdata.c.
    This is the initializing, constant data which will actually go
    into the structure you added to the <object>_DATA_DEFINITION in
    step 1.b.	The type of the structure you are initializing is a
    PERF_COUNTER_DEFINITION.  These are defined in winperf.h.

4.  Add code in extobjct.c to collect the data.

Note: adding an object is a little more work, but in all the same
places.  See the existing code for examples.  In addition, you must
increase the *NumObjectTypes parameter to Get<object>PerfomanceData
on return from that routine.

\****************************************************************************/
 
//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundries. Alpha support may 
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

#define VGA_NUM_PERF_OBJECT_TYPES 1

//----------------------------------------------------------------------------

//
//  Nbf Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define NUM_BITBLTS_OFFSET	    sizeof(DWORD)
#define NUM_TEXTOUTS_OFFSET	    NUM_BITBLTS_OFFSET + sizeof(DWORD)
#define SIZE_OF_VGA_PERFORMANCE_DATA \
				    NUM_TEXTOUTS_OFFSET + sizeof(DWORD)


//
//  This is the counter structure presently returned by Nbf for
//  each Resource.  Each Resource is an Instance, named by its number.
//

typedef struct _VGA_DATA_DEFINITION {
    PERF_OBJECT_TYPE		VgaObjectType;
    PERF_COUNTER_DEFINITION	NumBitBlts;
    PERF_COUNTER_DEFINITION	NumTextOuts;
} VGA_DATA_DEFINITION;

#pragma pack ()

#endif //_DATANBF_H_
