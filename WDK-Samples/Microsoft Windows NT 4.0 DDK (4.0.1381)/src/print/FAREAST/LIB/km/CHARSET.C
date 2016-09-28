/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    charset.c


Abstract:

    This module contains about charset functions


Author:

    17-Apl-1996 Wed created  -by-  Sueya Sugihara (sueyas)


[Environment:]

    GDI printer drivers, user and kernel mode


[Notes:]


Revision History:


--*/

#if defined(DBCS)

#include <windows.h>
#include <stddef.h>
#include <winddi.h>
#include <libproto.h>


//#define JOHAB_CHARSET           130
//#define HEBREW_CHARSET          177
//#define ARABIC_CHARSET          178
//#define GREEK_CHARSET           161
//#define TURKISH_CHARSET         162
//#define THAI_CHARSET            222
//#define EASTEUROPE_CHARSET      238
//#define RUSSIAN_CHARSET         204
//
//#define MAC_CHARSET             77
//#define BALTIC_CHARSET          186

/*
** FloydR 7/4/95
**
** This is a hack implementation (although very close to the real one)
** so that all the places in the code that need to know what the default
** charset and/or codepage is don't have duplicate code all over the place.
** This allows us to have a single binary for Japan/Korea/Chinese.
**
** This code copied from \\rastaman\ntwin!src\ntgdi\gre\mapfile.c
*/

/*
** SueyaS 4/17/96
**
** We should not call GreTranslateCharsetInfo and GreXXXX.
** So, MyTranslateCharsetInfo is here.
**
*/

#define NCHARSETS       14

UINT charsets[] = {
      ANSI_CHARSET,    SHIFTJIS_CHARSET,    HANGEUL_CHARSET, JOHAB_CHARSET,
      GB2312_CHARSET,  CHINESEBIG5_CHARSET, HEBREW_CHARSET,  ARABIC_CHARSET,
      GREEK_CHARSET,   TURKISH_CHARSET,     BALTIC_CHARSET,  EASTEUROPE_CHARSET,
      RUSSIAN_CHARSET, THAI_CHARSET };

UINT codepages[] ={ 1252, 932,  949,  1361,
                    936,  950,  1255, 1256,
                    1253, 1254, 1257, 1250,
                    1251, 874 };

/* no font signature implemented */


BOOL MyTranslateCharsetInfo(
    DWORD FAR *lpSrc,
    LPCHARSETINFO lpCs,
    DWORD dwFlags)
{
    int i;

    switch( dwFlags ) {
    case TCI_SRCCHARSET:
        {
        WORD    src ;

        src = LOWORD(lpSrc);
        for ( i=0; i<NCHARSETS; i++ )
        if ( charsets[i] == src )
            {
            lpCs->ciACP      = codepages[i];
            lpCs->ciCharset  = src;
            //lpCs->fs.fsCsb[0] = fs[i];
            return 1;
            }
        }
        break;
    case TCI_SRCCODEPAGE:
        {
        WORD    src ;

        src = LOWORD(lpSrc);
        for ( i=0; i<NCHARSETS; i++ )
        if ( codepages[i] == src )
            {
            lpCs->ciACP      = src ;
            lpCs->ciCharset  = charsets[i] ;
            //lpCs->fs.fsCsb[0] = fs[i];
            return 1;
            }
        }
        break;
    case TCI_SRCFONTSIG:
    default:
        ;
    }
    return(FALSE);
}

UINT MyGetACP()
{
    USHORT OemCodePage;
    USHORT AnsiCodePage;

    EngGetCurrentCodePage(&OemCodePage,
                          &AnsiCodePage);

    return (UINT)AnsiCodePage;
}
#endif //DBCS
