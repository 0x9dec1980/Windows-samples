/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    debug.h


Abstract:

    This module contains all debugger definitions


Author:

    30-Aug-1995 Wed 19:02:36 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#if DBG

VOID
cdecl
CPSUIDbgPrint(
    LPSTR   pszFormat,
    ...
    );

VOID
CPSUIDbgType(
    INT    Type
    );

VOID
_CPSUIAssert(
    LPSTR   pMsg,
    LPSTR   pFalseExp,
    LPSTR   pFilename,
    UINT    LineNo,
    DWORD   Exp,
    BOOL    Stop
    );



extern BOOL DoCPSUIWarn;


#define DBGP(x)                 (CPSUIDbgPrint x)

#if 1
#define DEFINE_DBGVAR(x)        DWORD DBG_CPSUIFILENAME=(x)
#else
#define DEFINE_DBGVAR(x)
#endif

#define CPSUIDBG(x,y)           if((x)&DBG_CPSUIFILENAME){CPSUIDbgType(0);DBGP(y);}

#define CPSUIDBGBLK(x)          x;
#define CPSUIWARN(x)            if(DoCPSUIWarn) { CPSUIDbgType(1);DBGP(x); }
#define CPSUIERR(x)             CPSUIDbgType(-1);DBGP(x)
#define CPSUIRIP(x)             CPSUIERR(x); DebugBreak()
#define CPSUIASSERT(b,x,e,i)     \
            if (!(e)) { _CPSUIAssert(x,#e,__FILE__,(UINT)__LINE__,(DWORD)i,b); }

#else   // DBG

#define CPSUIDBGBLK(x)
#define DEFINE_DBGVAR(x)
#define CPSUIDBG(x,y)
#define CPSUIWARN(x)
#define CPSUIERR(x)
#define CPSUIRIP(x)
#define CPSUIASSERT(b,x,e,i)

#endif  // DBG
