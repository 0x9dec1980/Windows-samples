/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        errlog.c

    Abstract: This module contains the error log related functions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 29-Jan-2001

    Revision History:
--*/

#include "pch.h"
#define MODULE_ID               0xff

PDRIVER_OBJECT gDriverObj = NULL;

/*++
    @doc    INTERNAL

    @func   VOID | LogError | Log an error entry in the event log.

    @parm   IN NTSTATUS | ErrorCode | Error code.
    @parm   IN NTSTATUS | NTStatus | NT status code if applicable.
    @parm   IN ULONG | UniqueID | Unique error ID, used when there are more
            than one place reporting the same ErrorCode.
    @parm   IN PWSTR | String1 | Optional substitution string 1.
    @parm   IN PWSTR | String2 | Optional substitution string 2.

    @rvalue None.
--*/

VOID
LogError(
    IN NTSTATUS ErrorCode,
    IN NTSTATUS NTStatus,
    IN ULONG    UniqueID OPTIONAL,
    IN PWSTR    String1  OPTIONAL,
    IN PWSTR    String2  OPTIONAL
    )
{
    TRACEPROC("LogError", 5)

    TRACEENTER(("(ErrorCode=%x,NTStatus=%x,UniqueID=%x,Str1=%S,Str2=%S)\n",
                ErrorCode, NTStatus, UniqueID, String1? String1: L"",
                String2? String2: L""));

    TRACEASSERT(gDriverObj != NULL);
    if (gDriverObj != NULL)
    {
        int len1, len2, len;
        PIO_ERROR_LOG_PACKET ErrEntry;

        len1 = String1? (wcslen(String1) + 1)*sizeof(WCHAR): 0;
        len2 = String2? (wcslen(String2) + 1)*sizeof(WCHAR): 0;
        len = len1 + len2 + FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
        len = max(len, sizeof(IO_ERROR_LOG_PACKET));
        if (len <= 255)
        {
            ErrEntry = IoAllocateErrorLogEntry(gDriverObj, (UCHAR)len);
            if (ErrEntry)
            {
                PUCHAR pbBuff = (PUCHAR)ErrEntry +
                                        FIELD_OFFSET(IO_ERROR_LOG_PACKET,
                                                     DumpData);

                ErrEntry->NumberOfStrings = 0;
                if (len1 > 0)
                {
                    ErrEntry->NumberOfStrings++;
                    RtlCopyMemory(pbBuff, String1, len1);
                    pbBuff += len1;
                }
                if (len2 > 0)
                {
                    ErrEntry->NumberOfStrings++;
                    RtlCopyMemory(pbBuff, String1, len1);
                    pbBuff += len1;
                }
                ErrEntry->StringOffset = FIELD_OFFSET(IO_ERROR_LOG_PACKET,
                                                      DumpData);

                ErrEntry->ErrorCode = ErrorCode;
                ErrEntry->FinalStatus = NTStatus;
                ErrEntry->UniqueErrorValue = UniqueID;
                IoWriteErrorLogEntry(ErrEntry);
            }
            else
            {
                TRACEWARN(("failed to allocate error log entry (len=%d).\n",
                           len));
            }
        }
        else
        {
            TRACEWARN(("error log entry too big (len=%d).\n", len));
        }
    }

    TRACEEXIT(("!\n"));
    return;
}       //LogError

#ifdef DEBUG
/*++
    @doc    INTERNAL

    @func   VOID | LogDbgMsg | Log an error message in the event log.

    @parm   IN NTSTATUS | ErrorCode | Error code.
    @parm   IN NTSTATUS | NTStatus | NT status code if applicable.
    @parm   IN PSZ | pszFormat | Points to the format string.
    @parm   ...

    @rvalue None.

    @note   Please note that it is a very bad practice to embed unlocalizable
            strings in the code.  Therefore, this function is DEBUG only just
            for the convenience of debugging.  A formal error message should
            be done via the LogError function and the error message text should
            be defineed in the message file (errcodes.mc) with an assigned
            error code.
--*/

VOID __cdecl
LogDbgMsg(
    IN NTSTATUS ErrorCode,
    IN NTSTATUS NTStatus OPTIONAL,
    IN PSZ      pszFormat,
    ...
    )
{
    TRACEPROC("LogDbgMsg", 5)
    #define MAX_ERRMSG_LEN      ((ERROR_LOG_MAXIMUM_SIZE -                     \
                                  FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData)) \
                                 /sizeof(WCHAR))
    static char szErrMsg[MAX_ERRMSG_LEN] = {0};

    TRACEENTER(("(ErrorCode=%x,NTStatus=%x,Format=%s)\n",
                ErrorCode, NTStatus, pszFormat));

    TRACEASSERT(gDriverObj != NULL);
    if (gDriverObj != NULL)
    {
        va_list arglist;
        int iLen, iTotalLen;
        PIO_ERROR_LOG_PACKET ErrEntry;

        va_start(arglist, pszFormat);
        iLen = _vsnprintf(szErrMsg, sizeof(szErrMsg) - 1, pszFormat, arglist);
        va_end(arglist);

        if (iLen == -1)
        {
            iLen = MAX_ERRMSG_LEN - 1;
            szErrMsg[iLen] = '\0';
        }

        iTotalLen = FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) +
                    (iLen + 1)*sizeof(WCHAR);
        iTotalLen = max(iTotalLen, sizeof(IO_ERROR_LOG_PACKET));
        ErrEntry = IoAllocateErrorLogEntry(gDriverObj, (UCHAR)iTotalLen);
        if (ErrEntry)
        {
            ErrEntry->NumberOfStrings = 1;
            ErrEntry->ErrorCode = ErrorCode;
            ErrEntry->StringOffset = FIELD_OFFSET(IO_ERROR_LOG_PACKET,
                                                  DumpData);
            mbstowcs((WCHAR *)ErrEntry->DumpData, szErrMsg, iLen);
            ErrEntry->FinalStatus = NTStatus;
            IoWriteErrorLogEntry(ErrEntry);
        }
        else
        {
            TRACEWARN(("failed to allocate error log entry (len=%d).\n",
                       iTotalLen));
        }

        if (ErrorCode == ERRLOG_DEBUG_INFORMATION)
        {
            TRACEINFO(0, ("%s\n", szErrMsg));
        }
        else if (ErrorCode == ERRLOG_DEBUG_WARNING)
        {
            TRACEWARN(("%s\n", szErrMsg));
        }
        else if (ErrorCode == ERRLOG_DEBUG_ERROR)
        {
            TRACEERROR(("%s\n", szErrMsg));
        }
    }

    TRACEEXIT(("!\n"));
    return;
}       //LogDbgMsg
#endif  //ifdef DEBUG
