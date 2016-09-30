/******************************Module*Header*******************************\
* Module Name: debug.c
*
* Debug helper routines.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#if DBG

////////////////////////////////////////////////////////////////////////////
// DEBUGGING INITIALIZATION CODE
//
// When you're bringing up your display for the first time, you can
// recompile with 'DebugLevel' set to 100.  That will cause absolutely
// all DISPDBG messages to be displayed on the kernel debugger (this
// is known as the "PrintF Approach to Debugging" and is about the only
// viable method for debugging driver initialization code).

LONG DebugLevel = 0;            // Set to '100' to debug initialization code
                                //   (the default is '0')

LONG gcFifo = 0;                // Number of currently free FIFO entries

////////////////////////////////////////////////////////////////////////////
// Miscellaneous Driver Debug Routines
////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug print
 *      routine.
 *      If the specified debug level for the print statement is lower or equal
 *      to the current debug level, the message will be printed.
 *
 *   Arguments:
 *
 *      DebugPrintLevel - Specifies at which debugging level the string should
 *          be printed
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
DebugPrint(
    LONG  DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )
{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel) {

        char buffer[256];
        int  len;

        // We prepend the STANDARD_DEBUG_PREFIX to each string, and
        // append a new-line character to the end:

        strcpy(buffer, STANDARD_DEBUG_PREFIX);
        len = vsprintf(buffer + strlen(STANDARD_DEBUG_PREFIX),
                        DebugMessage, ap);

        buffer[strlen(STANDARD_DEBUG_PREFIX) + len]     = '\n';
        buffer[strlen(STANDARD_DEBUG_PREFIX) + len + 1] = '\0';

        OutputDebugStringA(buffer);
    }

    va_end(ap);

} // DebugPrint()

////////////////////////////////////////////////////////////////////////////

VOID vCheckFifoSpace(
BYTE*   pjBase,
ULONG   level)
{
    gcFifo = level;

    CP_EIEIO();
    do {} while ((ULONG) CP_READ_REGISTER_BYTE(pjBase + HST_FIFOSTATUS) < level);
}

CHAR cGetFifoSpace(
BYTE*   pjBase)
{
    CHAR c;

    CP_EIEIO();
    c = CP_READ_REGISTER_BYTE(pjBase + HST_FIFOSTATUS);

    gcFifo = c;

    return(c);
}

VOID vWriteDword(
BYTE*   pj,
ULONG   ul)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_REGISTER_ULONG(pj, ul);
}

VOID vWriteByte(
BYTE*   pj,
BYTE    j)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_REGISTER_UCHAR(pj, j);
}

#endif // DBG
