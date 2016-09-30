/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    in_ntos.c

Abstract:

    This module contains the NT skeleton of the insulating modules for
    subsystem-parallelized stacks.  It contains the functions expected
    of an NT Device Driver.

--*/

#include "insulate.h"




NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT driver,
    IN PUNICODE_STRING description
    )

/*++

Routine Description:

    This procedure is called when the NT driver is loaded.  It registers
    the STREAMS modules that it contains.

Arguments:

    driver      - ptr to the NT driver object created by the system.
    description -  unused

Return Value:

    the final status from the initialization operation.

--*/

{
    NTSTATUS status;

    status = StrmRegisterModule(driver, &exbotinfo, "stack");

    ASSERT(NT_SUCCESS(status));

    status = StrmRegisterModule(driver, &extopinfo, "stack");

    ASSERT(NT_SUCCESS(status));

    status = StrmRegisterModule(driver, &inbotinfo, NULL);

    ASSERT(NT_SUCCESS(status));

    status = StrmRegisterModule(driver, &intopinfo, NULL);

    ASSERT(NT_SUCCESS(status));

    return(status);

} // DriverEntry
