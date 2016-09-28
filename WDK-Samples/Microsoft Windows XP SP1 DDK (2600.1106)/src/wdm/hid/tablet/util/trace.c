/*++
    Copyright (c) 1998-2001 Microsoft Corporation

    Module Name:
        trace.c

    Abstract:
        This module contains the code for debug tracing.

    Author:
        Michael Tsang (MikeTs) 24-Sep-1998

    Environment:
        Kernel mode

    Revision History:
--*/

#include "pch.h"

#ifdef TRACING

int   giTraceLevel = 0;
int   giVerboseLevel = 0;
int   giTraceIndent = 0;
ULONG gdwfTrace = 0;

/*++
    @doc    EXTERNAL

    @func   VOID | TraceInit | Initialize the tracing component.

    @parm   IN int | iDefTraceLevel | Specifies default trace level.
    @parm   IN int | iDefVerboseLevel | Specifies default verbose level.

    @rvalue None.
--*/

VOID
TraceInit(
    IN int iDefTraceLevel,
    IN int iDefVerboseLevel
    )
{
    giTraceLevel = iDefTraceLevel;
    giVerboseLevel = iDefVerboseLevel;
    return;
}       //TraceInit

/*++
    @doc    EXTERNAL

    @func   BOOLEAN | IsTraceProcOn | Determine if the procedure should be
            traced.

    @parm   IN ULONG | dwModuleID | Specifies the ID of the module that
            contains the procedure.
    @parm   IN PSZ | pszProcName | Points to the name of the procedure.
    @parm   IN int | iProcLevel | Specifies the trace level of the procedure.
    @parm   IN BOOLEAN | fEnter | TRUE if called from ENTERPROC.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOLEAN
IsTraceProcOn(
    IN ULONG   dwModuleID,
    IN PSZ     pszProcName,
    IN int     iProcLevel,
    IN BOOLEAN fEnter
    )
{
    BOOLEAN rc = FALSE;

    if (giTraceIndent < 0)
    {
        giTraceIndent = 0;
    }

    if (!fEnter)
    {
        giTraceIndent--;
    }

    dwModuleID = 1 << (dwModuleID + 4);
    if (!(gdwfTrace & TF_IN_PROGRESS) &&
        ((giTraceLevel & dwModuleID & MODULE_MASK) != 0) &&
        (iProcLevel <= (giTraceLevel & TRACELEVEL_MASK)))
    {
        int i;

        DbgPrint(COMPONENT_NAME ": ");
        for (i = 0; i < giTraceIndent; ++i)
        {
            DbgPrint("| ");
        }
        DbgPrint(pszProcName);
        rc = TRUE;
    }

    if (fEnter)
    {
        giTraceIndent++;
    }

    return rc;
}       //IsTraceProcOn

/*++
    @doc    EXTERNAL

    @func   BOOLEAN | IsTraceMsgOn | Determine if the message should be
            printed.

    @parm   IN PSZ | pszProcName | Points to the name of the procedure.
    @parm   IN ULONG | dwMsgType | Specifies the message type.
    @parm   IN int | iVerboseLevel | Specifies the verbose level of the message.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOLEAN
IsTraceMsgOn(
    IN PSZ   pszProcName,
    IN ULONG dwMsgType,
    IN int   iVerboseLevel
    )
{
    BOOLEAN rc = FALSE;

    if (iVerboseLevel <= giVerboseLevel)
    {
        DbgPrint(COMPONENT_NAME "_%s: ",
                 (dwMsgType == MSGTYPE_INFO)? "INFO":
                 (dwMsgType == MSGTYPE_WARN)? "WARN": "ERR");
        rc = TRUE;
    }

    if (dwMsgType == MSGTYPE_ERR)
    {
        TRAP();
    }

    return rc;
}       //IsTraceMsgOn

/*++
    @doc    EXTERNAL

    @func   PSZ | LookupName | Look up name string corresponding to the given
            code in a given name table.

    @parm   IN ULONG | dwCode | Code.
    @parm   IN PNAMETABLE | NameTable | Points to the name table.

    @rvalue SUCCESS | Returns pointer to the name string found.
    @rvalue FAILURE | Returns "unknown".
--*/

PSZ
LookupName(
    IN ULONG      dwCode,
    IN PNAMETABLE NameTable
    )
{
    PSZ pszName = "unknown";

    while (NameTable->pszName != NULL)
    {
        if (dwCode == NameTable->dwCode)
        {
            pszName = NameTable->pszName;
            break;
        }
        NameTable++;
    }

    return pszName;
}       //LookupName

#endif  //ifdef TRACING
