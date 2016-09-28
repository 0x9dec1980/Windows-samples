/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This function contains some example KD debugger extensions

Author:

    John Vert (jvert) 6-Aug-1992

Revision History:

--*/

#include <ntddk.h>
#include <windef.h>
#include <ntkdexts.h>
#include <stdlib.h>
#include <string.h>

CHAR igrepLastPattern[256];
DWORD igrepSearchStartAddress;
DWORD igrepLastPc;


VOID
igrep(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as a KD extension to grep the instruction
    stream for a particular pattern.

    Called as:

        !kdext.igrep [pattern [expression]]

    If a pattern is not given, the last pattern is used.  If expression
    is not given, the last hit address is used.

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    DWORD dwNextGrepAddr;
    DWORD dwCurrGrepAddr;
    CHAR SourceLine[256];
    BOOL NewPc;
    DWORD d;
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_DISASM lpDisasmRoutine;
    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    LPSTR pc;
    LPSTR Pattern;
    LPSTR Expression;
    CHAR Symbol[64];
    DWORD Displacement;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpDisasmRoutine = lpExtensionApis->lpDisasmRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    if ( igrepLastPc && igrepLastPc == dwCurrentPc ) {
        NewPc = FALSE;
        }
    else {
        igrepLastPc = dwCurrentPc;
        NewPc = TRUE;
        }

    //
    // check for pattern.
    //

    pc = lpArgumentString;
    Pattern = NULL;
    Expression = NULL;
    if ( *pc ) {
        Pattern = pc;
        while (*pc > ' ') {
                pc++;
            }

        //
        // check for an expression
        //

        if ( *pc != '\0' ) {
            *pc = '\0';
            pc++;
            if ( *pc <= ' ') {
                while (*pc <= ' '){
                    pc++;
                    }
                }
            if ( *pc ) {
                Expression = pc;
                }
            }
        }

    if ( Pattern ) {
        strcpy(igrepLastPattern,Pattern);

        if ( Expression ) {
            igrepSearchStartAddress = (lpGetExpressionRoutine)(Expression);
            if ( !igrepSearchStartAddress ) {
                igrepSearchStartAddress = igrepLastPc;
                return;
                }
            }
        else {
            igrepSearchStartAddress = igrepLastPc;
            }
        }

    dwNextGrepAddr = igrepSearchStartAddress;
    dwCurrGrepAddr = dwNextGrepAddr;
    d = (lpDisasmRoutine)(&dwNextGrepAddr,SourceLine,FALSE);
    while(d) {
        if (strstr(SourceLine,igrepLastPattern)) {
            igrepSearchStartAddress = dwNextGrepAddr;
            (lpGetSymbolRoutine)((LPVOID)dwCurrGrepAddr,Symbol,&Displacement);
            (lpOutputRoutine)("%s",SourceLine);
            return;
            }
        if ((lpCheckControlCRoutine)()) {
            return;
            }
        dwCurrGrepAddr = dwNextGrepAddr;
        d = (lpDisasmRoutine)(&dwNextGrepAddr,SourceLine,FALSE);
        }
}

VOID
str(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as a KD extension to format and dump
    counted (ansi) string.

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    ANSI_STRING AnsiString;
    DWORD dwAddrString;
    CHAR Symbol[64];
    LPSTR StringData;
    DWORD Displacement;
    BOOL b;
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;


    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrString = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrString ) {
        return;
        }


    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrString,Symbol,&Displacement);

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrString,
                              &AnsiString,
                              sizeof(AnsiString),
                              NULL);

    if ( !b ) {
        return;
    }

    StringData = malloc(AnsiString.Length+1);

    b = (lpReadMemoryRoutine)((LPVOID)AnsiString.Buffer,
                              StringData,
                              AnsiString.Length,
                              NULL);
    if ( !b ) {
        free(StringData);
        return;
        }

    (lpOutputRoutine)(
        "String(%d,%d) %s+%lx at %lx: %s\n",
        AnsiString.Length,
        AnsiString.MaximumLength,
        Symbol,
        Displacement,
        dwAddrString,
        StringData
        );

    free(StringData);
}

VOID
ustr(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as a KD extension to format and dump
    counted unicode string.

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    DWORD dwAddrString;
    CHAR Symbol[64];
    LPSTR StringData;
    DWORD Displacement;
    BOOL b;
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrString = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrString ) {
        return;
        }


    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrString,Symbol,&Displacement);

    //
    // Read the string from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrString,
                              &UnicodeString,
                              sizeof(UnicodeString),
                              NULL);

    if ( !b ) {
        return;
    }

    StringData = malloc(UnicodeString.Length+sizeof(UNICODE_NULL));

    b = (lpReadMemoryRoutine)((LPVOID)UnicodeString.Buffer,
                              StringData,
                              UnicodeString.Length,
                              NULL);
    if ( !b ) {
        free(StringData);
        return;
    }

    UnicodeString.Buffer = (PWSTR)StringData;
    UnicodeString.MaximumLength = UnicodeString.Length+(USHORT)sizeof(UNICODE_NULL);

    RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);
    free(StringData);

    (lpOutputRoutine)(
        "String(%d,%d) %s+%lx at %lx: %s\n",
        UnicodeString.Length,
        UnicodeString.MaximumLength,
        Symbol,
        Displacement,
        dwAddrString,
        AnsiString.Buffer
        );

    RtlFreeAnsiString(&AnsiString);
}

VOID
obja(
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as a KD extension to format and dump
    an object attributes structure.

Arguments:

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    UNICODE_STRING UnicodeString;
    DWORD dwAddrObja;
    OBJECT_ATTRIBUTES Obja;
    DWORD dwAddrString;
    CHAR Symbol[64];
    LPSTR StringData;
    DWORD Displacement;
    BOOL b;
    PNTKD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTKD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTKD_GET_SYMBOL lpGetSymbolRoutine;
    PNTKD_READ_VIRTUAL_MEMORY lpReadMemoryRoutine;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

    //
    // Evaluate the argument string to get the address of
    // the Obja to dump.
    //

    dwAddrObja = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrObja ) {
        return;
        }


    //
    // Get the symbolic name of the Obja
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrObja,Symbol,&Displacement);

    //
    // Read the obja from the debuggees address space into our
    // own.

    b = (lpReadMemoryRoutine)((LPVOID)dwAddrObja,
                              &Obja,
                              sizeof(Obja),
                              NULL);
    if ( !b ) {
        return;
    }
    StringData = NULL;
    if ( Obja.ObjectName ) {
        dwAddrString = (DWORD)Obja.ObjectName;
        b = (lpReadMemoryRoutine)((LPVOID)dwAddrString,
                                  &UnicodeString,
                                  sizeof(UnicodeString),
                                  NULL);
        if ( !b ) {
            return;
        }

        StringData = malloc(UnicodeString.Length+sizeof(UNICODE_NULL));

        b = (lpReadMemoryRoutine)((LPVOID)UnicodeString.Buffer,
                                  StringData,
                                  UnicodeString.Length,
                                  NULL);
        if ( !b ) {
            free(StringData);
            return;
        }
        UnicodeString.Buffer = (PWSTR)StringData;
        UnicodeString.MaximumLength = UnicodeString.Length+(USHORT)sizeof(UNICODE_NULL);
    }

    //
    // We got the object name in UnicodeString. StringData is NULL if no name.
    //

    (lpOutputRoutine)(
        "Obja %s+%lx at %lx:\n",
        Symbol,
        Displacement,
        dwAddrObja
        );
    if ( StringData ) {
        (lpOutputRoutine)("\t%s is %ws\n",
            Obja.RootDirectory ? "Relative Name" : "Full Name",
            UnicodeString.Buffer
            );
        free(StringData);
        }
    if ( Obja.Attributes ) {
            if ( Obja.Attributes & OBJ_INHERIT ) {
                (lpOutputRoutine)("\tOBJ_INHERIT\n");
                }
            if ( Obja.Attributes & OBJ_PERMANENT ) {
                (lpOutputRoutine)("\tOBJ_PERMANENT\n");
                }
            if ( Obja.Attributes & OBJ_EXCLUSIVE ) {
                (lpOutputRoutine)("\tOBJ_EXCLUSIVE\n");
                }
            if ( Obja.Attributes & OBJ_CASE_INSENSITIVE ) {
                (lpOutputRoutine)("\tOBJ_CASE_INSENSITIVE\n");
                }
            if ( Obja.Attributes & OBJ_OPENIF ) {
                (lpOutputRoutine)("\tOBJ_OPENIF\n");
                }
        }
}

