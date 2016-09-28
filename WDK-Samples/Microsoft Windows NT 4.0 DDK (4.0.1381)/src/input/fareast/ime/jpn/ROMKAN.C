/*********************************************************************
*                                                                    
*      ROMKAN.C - Windows NT / Windows 95 FAKEIME                                
*                                                                   
*      Copyright (c) 1994-1996  Microsoft Corporation               
*                                                                   
**********************************************************************/
#include "precomp.h"
#include "stdio.h"

typedef struct tagROMKANPAIR {
   PTCHAR pKeys;
   PTCHAR pKana;
   INT    cKeys;
   INT    cKana;
   VOID *pNext;
} ROMKANPAIR, *PROMKANPAIR;

//
// per-process Romaji-Kana conversion table
//
PROMKANPAIR gapRomKanPairRoot[ 'z' - 'a' + 1 + 1 ];

INT GetIndex( TCHAR ch )
{
    INT index;
    TCHAR buf1[2];
    TCHAR buf2[2];

    buf1[0] = ch; buf1[1] = TEXT('\0');
    if ( LCMapString( JAPANESE_LOCALE,
                      LCMAP_HALFWIDTH|LCMAP_LOWERCASE,
                      buf1,
                      1,
                      buf2,
                      2 ) > 0 )
    {
        TCHAR c = buf2[0];

        if ( TEXT('z') >= c && c >= TEXT('a') )  {
            return( c - TEXT('a'));
        }
    }
    return ( TEXT('z') - TEXT('a') + 1 );
}

#define MYCSFLAG        NORM_IGNORECASE | \
                      NORM_IGNOREKANATYPE | \
                      NORM_IGNOREWIDTH
#define CS_EQUAL   2

PROMKANPAIR FindMatch( PTSTR p )
{
    PROMKANPAIR pPair;

    pPair = gapRomKanPairRoot[ GetIndex( *p ) ];
    while ( pPair != NULL ) {
        if ( CompareString( JAPANESE_LOCALE,
                            MYCSFLAG,
                            pPair->pKeys,
                            pPair->cKeys,
                            p,
                            pPair->cKeys) == CS_EQUAL )
            break;
        pPair = (PROMKANPAIR)pPair->pNext;
    }
    return pPair;
}

TCHAR bufNN[2] = { TEXT('n'), TEXT('\0') };
TCHAR bufMM[2] = { TEXT('m'), TEXT('\0') };
TCHAR NN[] = { 0x3093, TEXT('\0') };

BOOL IsN( TCHAR ch )
{
    INT i;
    TCHAR buf[2];

    buf[0] = ch;
    buf[1] = TEXT('\0');

    i = CompareString(JAPANESE_LOCALE, MYCSFLAG, buf, -1, bufNN, -1);
    if ( i == CS_EQUAL )
        return TRUE;
    i = CompareString(JAPANESE_LOCALE, MYCSFLAG, buf, -1, bufMM, -1);
    if ( i == CS_EQUAL )
        return TRUE;
    return FALSE;
}

//
// read romaji-kana definition file
//
BOOL RK_OpenDefFile()
{
    FILE *fp;
    int c;
    char buf[256];
    PROMKANPAIR pPair;
    PCHAR pc;
    PCHAR pc2;
    int   index;
    int   n;
    char FileName[MAX_PATH];
    char *pName;

    pName = FileName;
    pName += GetWindowsDirectoryA( FileName, MAX_PATH );
    if ( *(pName-1) != '\\' )
        *pName++ = TEXT('\\');
    strcpy( pName, "defroma.txt");

    fp = fopen( FileName, "r" );
    if ( fp == NULL ) {
        return FALSE;
    }

    while ( fgets( buf, 255, fp ) != NULL ) {
        pc = buf;
        index = GetIndex( *pc );
        //
        // find '='
        //
        while ( *pc ) {
            if ( *pc == '=' ) {
                *pc = '\0';
                pc += 1;
                pc2 = pc;
                break;
            }
            pc++;
        }

        if ( *(pc) == '\0' ) {
            fclose(fp);
            return FALSE;
        }

        while ( *pc ) {
            if ( *pc == '\n' ) {
                *pc = '\0';
                break;
            }
            pc++;
        }

        pPair = LocalAlloc(LMEM_FIXED, sizeof(ROMKANPAIR) );
        if ( pPair == NULL ) {
            fclose(fp);
            return FALSE;
        }
        pPair->cKeys = lstrlenA( buf );
        pPair->cKana = lstrlenA( pc2 );

        pPair->pKeys = LocalAlloc(LMEM_FIXED, (pPair->cKeys + 1 ) * sizeof(TCHAR) );
        pPair->pKana = LocalAlloc(LMEM_FIXED, (pPair->cKana + 1 ) * sizeof(TCHAR) );

        if ( pPair->pKeys == NULL || pPair->pKana == NULL ) {
            fclose(fp);
            return FALSE;
        }
        pPair->pNext = NULL;
        
        //
        // The definition file is written in Shift-JIS.
        // Let's convert it to Unicode.
        //
        n = MultiByteToWideChar( JAPANESE_CODEPAGE,
                                 0,
                                 buf,
                                 pPair->cKeys + 1,
                                 pPair->pKeys,
                                 (pPair->cKeys + 1) * sizeof(WCHAR) );
        pPair->cKeys = n - 1;

        n = MultiByteToWideChar( JAPANESE_CODEPAGE,
                                 0,
                                 pc2,
                                 pPair->cKana + 1,
                                 pPair->pKana,
                                 (pPair->cKana + 1) * sizeof(WCHAR) );
        pPair->cKana = n - 1;

        //
        // add pPair to the list
        //
        if ( gapRomKanPairRoot[index] == NULL ) {
            gapRomKanPairRoot[index] = pPair;
        } else {
            PROMKANPAIR pTmp;

            pTmp = gapRomKanPairRoot[index];
            while ( pTmp->pNext != NULL )
                pTmp = (PROMKANPAIR)pTmp->pNext;
            pTmp->pNext = (PVOID)pPair;
        }
    }
    fclose( fp );
    return TRUE;
}

//
// converts the given roma-ji string to kana string
// returns the new cursor position
//
INT RK_Convert( PTSTR pDstBuf, INT cDst, PTSTR pSrcBuf, DWORD dwCursorPos )
{
    PROMKANPAIR pPair;
    INT NewCursorPos = -1;
    WCHAR wchN = TEXT('\0');
    PTSTR pDst = pDstBuf;
    PTSTR pSrc = pSrcBuf;

    if ( dwCursorPos == 0 ) 
        NewCursorPos = 0;

    while ( *pSrc ) {
        pPair = FindMatch( pSrc );

        if ( pPair != NULL ) {
            if ( wchN != TEXT('\0') ) {
                cDst--;
                if ( cDst >= 0 ) {
                    lstrcpy( pDst, NN );
                    pDst++;
                } else {
                    return -1;
                }
                wchN = TEXT('\0');
            }

            cDst -= pPair->cKana;
            if ( cDst >= 0 ) {
                lstrcpy( pDst, pPair->pKana);
                pSrc += pPair->cKeys;
                if ( NewCursorPos < 0 && (DWORD)(pSrc - pSrcBuf) > dwCursorPos ) {
                    NewCursorPos = pDst - pDstBuf; 
                }
                pDst += pPair->cKana;
            } else {
                return -1;
            }

        } else { 
            if ( wchN != TEXT('\0') ) {

                cDst -= 1;
                if ( cDst >= 0 ) {
                    lstrcpy( pDst, NN );
                    pDst++;
                } else {
                    return -1;
                }
                wchN = TEXT('\0');
            }

            if ( IsN( *pSrc ) ) {
                wchN = *pSrc;
                pSrc++; 
                if ( NewCursorPos < 0 && (DWORD)(pSrc - pSrcBuf) > dwCursorPos ) {
                    NewCursorPos = pDst - pDstBuf; 
                }
            } else {
                cDst--;
                if ( cDst >= 0 ) {
                    *pDst++ = *pSrc++;
                    if ( NewCursorPos < 0 && (DWORD)(pSrc - pSrcBuf) > dwCursorPos ) {
                        NewCursorPos = pDst - pDstBuf - 1; 
                    }
                } else {
                    return -1;
                }
            }
        }
    }

    if ( wchN != TEXT('\0') ) {
        cDst--;
        if ( cDst >= 0 ) {        
            *pDst++ = wchN;
        } else {
            return -1;
        }
    }
    cDst--;
    if ( cDst >= 0 ) 
        *pDst = TEXT('\0');
    else
        return -1;

    if ( NewCursorPos < 0 ) {
        return pDst - pDstBuf;
    } else {
        return NewCursorPos;
    }
}
