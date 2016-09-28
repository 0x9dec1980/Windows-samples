/**[f******************************************************************
 * afm.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 * USAGE: AFM <AFM filename> <MS database filename>
 *        output is filename.PFM
 *
 **f]*****************************************************************/

/*****************************************************************
* This module of the afm compiler parses the afm file and collects
* information in the afm structure.  It then passes control to the
* pfm module which outputs the pfm file from the data in the afm
* structure.
*
*****************************************************************
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys\types.h>
#include <io.h>
#include <sys\stat.h>
#include <string.h>

#include "pfm.h"

/****************************************************************************/
#ifdef DEBUG_ON
#define DBMSG(msg) printf msg
#else
#define DBMSG(msg)
#endif
/****************************************************************************/

#define TRUE    1
#define FALSE    0
#define NULL     0

typedef int BOOL;
BOOL fEOF = FALSE;
BOOL fUnGetLine = FALSE;
int fhIn;        /* Input file handle */
FILE *fhData;    /* MS specific data file handle */

#define FW_LIGHT    250
#define FW_NORMAL   400
#define FW_BOLD     700
#define IBULLET     0x095        /* 87-1-15 sec (was 1) */

char rgbBuffer[2048];    /* The file buffer */
int cbBuffer;            /* The number of bytes in the buffer */
char *pbBuffer;         /* Ptr to current location in buffer */

char rgbLine[160];        /* The current line of text being processed */
char *szLine;            /* Ptr to the current location in the line */

#ifdef DBCS
int jWinCharset = -1;
#endif // DBCS

/* A lookup table key structure for converting strings to tokens */
typedef struct
    {
    char *szKey;    /* Ptr to the string */
    int iValue;     /* The corresponding token value */
    }KEY;

/* The AFM tokens */
#define TK_UNDEFINED        0
#define TK_EOF            1
#define TK_STARTKERNDATA    2
#define TK_STARTKERNPAIRS   3
#define TK_KPX            4
#define TK_ENDKERNPAIRS     5
#define TK_ENDKERNDATA        6
#define TK_FONTNAME        7
#define TK_WEIGHT        8
#define TK_ITALICANGLE        9

#define TK_ISFIXEDPITCH     10
#define TK_UNDERLINEPOSITION    11
#define TK_UNDERLINETHICKNESS    12
#define TK_FONTBBOX        13
#define TK_CAPHEIGHT        14
#define TK_XHEIGHT        15
#define TK_DESCENDER        16
#define TK_ASCENDER        17
#define TK_STARTCHARMETRICS    18
#define TK_ENDCHARMETRICS    19
#define TK_ENDFONTMETRICS    20
#define TK_STARTFONTMETRICS    21
#define TK_ENCODINGSCHEME    22

/* Microsoft extensions to AFM spec */
#define TK_ISTRUETYPE           23

#ifdef DBCS
/* more AFM tokens */
#define TK_CHARWIDTH 24
#endif // DBCS



/*************************************************************
* Name: szIsEqual()
*
* Action: Compare two NULL terminated strings.
*
* Returns: TRUE if they are equal
*       FALSE if they are different
*
*************************************************************
*/
BOOL szIsEqual(sz1, sz2)
char *sz1;
char *sz2;
    {
    while (*sz1 && *sz2)
    {
    if (*sz1++ != *sz2++)
        return(FALSE);
    }
    return(*sz1 == *sz2);
    }




/*************************************************************
* Name: szMove()
*
* Action: Copy a string.  This function will copy at most the
*      number of bytes in the destination area - 1.
*
**************************************************************
*/
szMove(szDst, szSrc, cbDst)
char *szDst;        /* Ptr to the destination area */
char *szSrc;        /* Ptr to the source area */
int cbDst;        /* The size of the destination area */
    {
    while (*szDst++ = *szSrc++)
    if (--cbDst <= 0)
        {
        *(szDst-1) = 0;
        break;
        }
    }


/****************************************************************
* Name: GetBuffer()
*
* Action: Read a new buffer full of text from the input file.
*
* Note: If the end of file is encountered in this function then
*    the program is aborted with an error message.  Normally
*    the program will stop processing the input when it sees
*    the end of information keyword.
*
*****************************************************************
*/
BOOL GetBuffer()
    {
    cbBuffer = 0;
    if (!fEOF)
    {
    cbBuffer = read(fhIn, rgbBuffer, sizeof(rgbBuffer));
    if (cbBuffer<=0)
        {
        cbBuffer = 0;
        fEOF = TRUE;
        printf("Premature end of file encountered\n");
        exit(1);
        }
    }
    pbBuffer = rgbBuffer;
    return(fEOF);
    }




/*****************************************************************
* Name: UnGetLine()
*
* Action: This routine pushes the most recent line back into the
*      input buffer.
*
******************************************************************
*/
UnGetLine()
    {
    fUnGetLine = TRUE;
    szLine = rgbLine;
    }





/******************************************************************
* Name: GetLine()
*
* Action: This routine gets the next line of text out of the
*      input buffer.
*
*******************************************************************
*/
BOOL GetLine()
    {
    int cbLine;
    char bCh;

    if (fUnGetLine)
    {
    szLine = rgbLine;
    fUnGetLine = FALSE;
    return(FALSE);
    }

    cbLine = 0;
    szLine = rgbLine;
    *szLine = 0;
    if (!fEOF)
    {
    while(TRUE)
        {
        if (cbBuffer<=0)
        GetBuffer();
        while(--cbBuffer>=0)
        {
        bCh = *pbBuffer++;
        if (bCh=='\n' || ++cbLine>sizeof(rgbLine))
            {
            *szLine = 0;
            szLine = rgbLine;
            EatWhite();
            if (*szLine!=0)
            goto DONE;
            szLine = rgbLine;
            cbLine = 0;
            continue;
            }
        *szLine++ = bCh;
        }
        }
    }
    *szLine = 0;

DONE:
    szLine = rgbLine;
    return(fEOF);
    }




/****************************************************************
* Name: EatWhite()
*
* Action: This routine moves the input buffer pointer forward to
*      the next non-white character.
*
*****************************************************************
*/
EatWhite()
    {
    while (*szLine && (*szLine==' ' || *szLine=='\t'))
    ++szLine;
    }






/*******************************************************************
* Name: GetWord()
*
* Action: This routine gets the next word delimited by white space
*      from the input buffer.
*
********************************************************************
*/
GetWord(szWord, cbWord)
char *szWord;        /* Ptr to the destination area */
int cbWord;        /* The size of the destination area */
    {
    char bCh;

    EatWhite();
    while (--cbWord>0)
    {
    switch(bCh = *szLine++)
        {
        case 0:
        --szLine;
        goto DONE;
        case ' ':
        case '\t':
        --szLine;
        goto DONE;
        case ';':
        *szWord++ = bCh;
        goto DONE;
        default:
        *szWord++ = bCh;
        break;
        }
    }
DONE:
    *szWord = 0;
    }





/***************************************************************
* Name: MapToken()
*
* Action: This routine maps an ascii key word into an integer token.
*
* Returns: The token value.
*
*****************************************************************
*/
int MapToken(szWord)
char *szWord;        /* Ptr to the ascii keyword string */
    {
    static KEY rgKeys[] =
    {
    "FontBBox", TK_FONTBBOX,
    "StartFontMetrics", TK_STARTFONTMETRICS,
    "FontName", TK_FONTNAME,
    "Weight",   TK_WEIGHT,
    "ItalicAngle", TK_ITALICANGLE,
    "IsFixedPitch", TK_ISFIXEDPITCH,
    "UnderlinePosition", TK_UNDERLINEPOSITION,
    "UnderlineThickness", TK_UNDERLINETHICKNESS,
    "CapHeight", TK_CAPHEIGHT,
    "XHeight", TK_XHEIGHT,
    "Descender", TK_DESCENDER,
    "Ascender", TK_ASCENDER,
    "StartCharMetrics", TK_STARTCHARMETRICS,
    "EndCharMetrics", TK_ENDCHARMETRICS,
    "StartKernData", TK_STARTKERNDATA,
    "StartKernPairs", TK_STARTKERNPAIRS,
    "KPX", TK_KPX,
    "EndKernPairs", TK_ENDKERNPAIRS,
    "EndKernData", TK_ENDKERNDATA,
    "EndFontMetrics", TK_ENDFONTMETRICS,
    "EncodingScheme", TK_ENCODINGSCHEME,
#ifdef DBCS
    "CharWidth", TK_CHARWIDTH,
#endif // DBCS
        /* Microsoft extensions to AFM spec */
        "IsTrueType", TK_ISTRUETYPE,

    NULL, 0
    };

    KEY *pkey;

    pkey = rgKeys;
    while (pkey->szKey)
    {
    if (szIsEqual(szWord, pkey->szKey))
        {
        return(pkey->iValue);
        }
    ++pkey;
    }
    return(TK_UNDEFINED);
    }





/************************************************************
* Name: GetNumber()
*
* Action: This routine parses an ASCII decimal number from the
*      input file stream and returns its value.
*
**************************************************************
*/
int GetNumber()
    {
    int iVal;
    BOOL fNegative;

    fNegative = FALSE;

    iVal = 0;
    EatWhite();

#ifdef DBCS
    // it can be in hex notation (e.g. CH <nn> in characer metrics.)
    if (*szLine=='<')
    {
        for (szLine++; *szLine && *szLine != '>'; szLine++)
        {
            iVal <<= 4;
            if (*szLine>='0' && *szLine<='9')
                iVal += (*szLine - '0');
             else if (*szLine>='A' && *szLine<='F')
                iVal += (*szLine - 'A' + 10);
             else if (*szLine>='a' && *szLine<='f')
                iVal += (*szLine - 'a' + 10);
        }
        if (*szLine++ != '>')
            goto ERROR;
    }
    else {
#endif // DBCS

    if (*szLine=='-')
    {
    fNegative = TRUE;
    ++szLine;
    }
    if (*szLine<'0' || *szLine>'9')
    goto ERROR;
    while (*szLine>='0' && *szLine<='9')
    iVal = iVal * 10 + (*szLine++ - '0');
    if (fNegative)
    iVal = - iVal;

#ifdef DBCS
    }
#endif // DBCS

    if (*szLine==0 || *szLine==' ' || *szLine=='\t' || *szLine==';')
    return(iVal);

ERROR:
    printf("GetNumber: invalid number %s\n", szLine);
    printf("%s\n", rgbLine);
    exit(1);
    }





/******************************************************************
* Name: GetFloat()
*
* Action: This routine parses an ASCII floating point decimal number
*      from the input file stream and returns its value scaled
*      by a specified amount.
*
********************************************************************
*/
int GetFloat(iScale)
int iScale;        /* The amount to scale the value by */
    {
    long lVal;
    long lDivisor;
    BOOL fNegative;
    int iFraction;

    EatWhite();

    fNegative = FALSE;
    lVal = 0L;

    if (*szLine=='-')
    {
    fNegative = TRUE;
    ++szLine;
    }
    if (*szLine<'0' || *szLine>'9')
    goto ERROR;
    while (*szLine>='0' && *szLine<='9')
    lVal = lVal * 10 + (*szLine++ - '0');

    lDivisor = 1L;
    if (*szLine=='.')
    {
    ++szLine;
    while (*szLine>='0' && *szLine<='9')
        {
        lVal = lVal * 10 + (*szLine++ - '0');
        lDivisor = lDivisor * 10;
        }
    }
    lVal = (lVal * iScale) / lDivisor;

    if (fNegative)
    lVal = - lVal;

    if (*szLine==0 || *szLine==' ' || *szLine=='\t' || *szLine==';')
    return((short)lVal);

ERROR:
    printf("GetFloat: invalid number %s\n", szLine);
    printf("%s\n", rgbLine);
    exit(1);
    }






/*********************************************************************
* Name: GetToken()
*
* Action: Get the next token from the input stream.
*
**********************************************************************
*/
int GetToken()
    {
    char szWord[80];

    if (*szLine==0)
    if (GetLine())
        return(TK_EOF);
    GetWord(szWord, sizeof(szWord));
    return(MapToken(szWord));
    }


/****************************************************************
* Name: PrintLine()
*
* Action: Print a line of ASCII text with the white space stripped
*      out.
*
******************************************************************
*/
PrintLine(szLine)
char *szLine;
    {
    char szWord[80];

    while (*szLine)
    {
    GetWord(szWord, sizeof(szWord));
    printf("%s ", szWord);
    }
    printf("\n");
    }




/***************************************************************
* Name: KxSort()
*
* Action: Sort the pair kerning data using the quicksort algorithm.
*
*****************************************************************
*/
KxSort(pkx1, pkx2)
KX *pkx1;
KX *pkx2;
    {
    static int iPivot;
    int iKernAmount;
    KX *pkx1T;
    KX *pkx2T;

    if (pkx1>=pkx2)
    return;

    iPivot = pkx1->iKey;;
    iKernAmount = pkx1->iKernAmount;
    pkx1T = pkx1;
    pkx2T = pkx2;

    while (pkx1T < pkx2T)
    {
    while (pkx1T < pkx2T)
        {
        if (pkx2T->iKey < iPivot)
        {
        pkx1T->iKey = pkx2T->iKey;
        pkx1T->iKernAmount = pkx2T->iKernAmount;
        ++pkx1T;
        break;
        }
        else
        --pkx2T;
        }
    while (pkx1T < pkx2T)
        {
        if (pkx1T->iKey > iPivot)
        {
        pkx2T->iKey = pkx1T->iKey;
        pkx2T->iKernAmount = pkx1T->iKernAmount;
        --pkx2T;
        break;
        }
        else
        ++pkx1T;
        }
    }
    pkx2T->iKey = iPivot;
    pkx2T->iKernAmount = iKernAmount;
    ++pkx2T;
    if ((pkx1T - pkx1) < (pkx2 - pkx2T))
    {
    KxSort(pkx1, pkx1T);
    KxSort(pkx2T, pkx2);
    }
    else
    {
    KxSort(pkx2T, pkx2);
    KxSort(pkx1, pkx1T);
    }
    }





/******************************************************************
* Name: ParseKernPairs()
*
* Action: Parse the pairwise kerning data.
*
*******************************************************************
*/
ParseKernPairs()
    {
    int i;
    int iCh1;
    int iCh2;
    int iKernAmount;
    KP *pkp;
    int iToken;
    int cPairs;
    char szWord[80];

    cPairs = GetNumber();

    pkp = &afm.kp;
    pkp->cPairs = cPairs;
    for (i=0; i<cPairs; ++i)
    {
    if (GetLine())
        break;
    if (GetToken()!=TK_KPX)
        {
        UnGetLine();
        break;
        }
    GetWord(szWord, sizeof(szWord));
    iCh1 = GetCharCode(szWord);
    GetWord(szWord, sizeof(szWord));
    iCh2 = GetCharCode(szWord);
    iKernAmount = GetNumber();
    pkp->rgPairs[i].iKey = iCh2<<8 | iCh1;
    pkp->rgPairs[i].iKernAmount = iKernAmount;
    }

    GetLine();
    iToken = GetToken();

    if (fEOF)
    printf("GetPairs: Premature end of file encountered\n");
    else if (iToken!=TK_ENDKERNPAIRS)
    {
    printf("GetPairs: expected EndKernPairs\n");
    printf("%s\n", rgbLine);
    exit(1);
    }
    KxSort(&afm.kp.rgPairs[0], &afm.kp.rgPairs[afm.kp.cPairs - 1]);
    }




/********************************************************
* Name: ParseKernData()
*
* Action: Start processing the pairwise kerning data.
*
************************************************************
*/
ParseKernData()
    {
    int iToken;

    if (!GetLine())
    {
    if (GetToken()==TK_STARTKERNPAIRS)
        ParseKernPairs();
    else
        printf("ParseKernData: expected StartKernPairs\n");
    }
    else
    printf("ParseKernData: unexpected end of file\n");
    }




/***********************************************************
* Name: ParseFontName()
*
* Action: Move the font name from the input buffer into the afm
*      structure.
*
*************************************************************
*/
ParseFontName()
{
#ifdef DBCS
    int len;
    static KEY sfxKeys[] =
    {
    "-RKSJ-H", SHIFTJIS_CHARSET,
    "-RKSJ-V", SHIFTJIS_CHARSET,
    "-BIG5", CHINESEBIG5_CHARSET, // DEC
    "-BF", CHINESEBIG5_CHARSET, // DynaFont
    NULL, 0,
    };
#endif // DBCS
    EatWhite();
    szMove(afm.szFont, szLine, sizeof(afm.szFont));

#ifdef DBCS
    len = strlen(afm.szFont);
#endif // DBCS

    if (szIsEqual("Symbol", afm.szFont)  ||  
            szIsEqual("ZapfDingbats", afm.szFont))
        afm.iEncodingScheme = SYMBOL_CHARSET;
#ifdef DBCS
    else if (jWinCharset != -1)
        afm.iEncodingScheme = jWinCharset;
    else {

        KEY *pkey;
        int len, sfxlen;

        len = strlen( afm.szFont );
        pkey = sfxKeys;
        while (pkey->szKey) {

            sfxlen = strlen( pkey->szKey );
            if ( len > sfxlen ) {
                if (!strcmp( afm.szFont + len - sfxlen, pkey->szKey))
                    break;
            }
            ++pkey;
        }
        
        if ( pkey->szKey ) {
            afm.iEncodingScheme = pkey->iValue;
        }
    }

    fprintf( stderr, "CharSet = %d\n", afm.iEncodingScheme );
#else // DBCS
        afm.iEncodingScheme = ANSI_CHARSET;
#endif // DBCS
}


/***********************************************************
* Name: ParseMSFields()
*
* Action: Search a database of records organized as lines, using 
* the afm.szFont entry as a key. Once this key has been matched 
* this routine parses, checks, and moves the MS face and family 
* entries to the afm structure.
*
* The record structure is organized as follows:
*   -If line[0] = '#' it is a comment and should be ignored.
*   -PostScript face name followed by a colon delimiter.
*   -MicroSoft face name followed by a colon delimiter.
*   -MicroSoft family name followed by an EOLN.
*
*   e.g.   Symbol:Symbol:Roman   // no longer Decorative
*          Times-Bold:Tms Rmn:Roman
*
*************************************************************
*/

ParseMSFields()
{

#define IsNull(s)     s[0] == NULL

  char buffer[80], MSKey[80], MSFace[80], MSFamily[80], MSFont[80], *fgets();
  int fnd = FALSE;
  
  static KEY fmKeys[] =
  {
  "Roman", FF_ROMAN,
  "Swiss", FF_SWISS,
  "Decorative", FF_DECORATIVE,
  "Modern", FF_MODERN,
  "Script", FF_SCRIPT,
  NULL, 0
  };

  KEY *pkey;

  if (IsNull(afm.szFont)) {
    printf("afm: ParseMSFields() called without valid font name.\n");
    exit(1);
  }

  while (!feof(fhData))
  {
    if (fgets(buffer, 80, fhData) != NULL) 
    {
      WORD  nColons, i;

      /* eat comment lines */
      if (buffer[0] == '#')
         continue;

      // count number of colons in buffer 2 means the format of 
      // the afmtopfm.dat file is:
      // MSKey:MSFace:MSFamily
      // three means the format is :
      // MSKey:MSFont:MSFace:MSFamily

      for(nColons = i = 0 ; buffer[i] ; i++)
         if(buffer[i] == ':')
            nColons++;

      if(nColons == 2)
         sscanf(buffer, "%[^:]:%[^:]:%s", MSKey, MSFace, MSFamily);
      else  if(nColons == 3)
         sscanf(buffer, "%[^:]:%[^:]:%[^:]:%s", MSKey, MSFont, MSFace, MSFamily);
      else
         continue;

      if (!strcmp(MSKey, afm.szFont))
      {
        fnd = TRUE;
        if(nColons == 3)
           szMove(afm.szFont, MSFont, sizeof(afm.szFont));
        szMove(afm.szFace, MSFace, sizeof(afm.szFace));
        pkey = fmKeys;
        while (pkey->szKey) 
        {
          if (szIsEqual(MSFamily, pkey->szKey)) 
          {
            afm.iFamily = pkey->iValue;
            break;
          }
          ++pkey;
        }
        break;
      } 
    } 
  }  // end while

   /* check for and flag error conditions */
  if (!fnd) {
    printf("afm: no entries found for %s in database\n", afm.szFont);
    exit(1);
  }

  if (!afm.iFamily) {
    printf("MSFamily: invalid font family = %s\n", MSFamily);
    exit(1);
  }

  if (IsNull(afm.szFace)) {
    printf("MSFace: invalid or missing = %s\n", MSFace);
    exit(1);
  }

}


/******************************************************************
* Name: ParseWeight()
*
* Action: Parse the fonts weight and set the corresponding
*      entry in the afm structure.
*
*******************************************************************
*/

ParseWeight()
    {
    char szWord[80];
    int fnd = FALSE;
    static KEY wtKeys[] =
    {
    "Light", FW_LIGHT,            
    "LightCondensed", FW_LIGHT,
    "Book", FW_NORMAL,
    "Medium", FW_NORMAL,
    "Roman", FW_NORMAL,
    "Regular", FW_NORMAL,
    "Italic", FW_NORMAL,
    "MediumCondensed", FW_NORMAL,
    "Bold", FW_BOLD,
    "BoldItalic", FW_BOLD,
    "Demi", FW_BOLD,
    "Poster", FW_BOLD,
    "Heavy", FW_BOLD,
    "Black", FW_BOLD,
    "BlackCondensed", FW_BOLD,
    "BoldCondensed", FW_BOLD,
    NULL, 0
    };

    KEY *pkey;

    GetWord(szWord, sizeof(szWord));

    pkey = wtKeys;
    while (pkey->szKey)
    {
    if (szIsEqual(szWord, pkey->szKey))
        {
        afm.iWeight = pkey->iValue;
        fnd = TRUE;
        break;
        }
    ++pkey;
    }

     /* check for and flag error conditions */
    if (!fnd) 
        {
        printf("ParseWeight: unknown font weight = \"%s\"\n", szWord);
        afm.iWeight = FW_NORMAL;
        }
    }




/**************************************************************
* Name: ParseCharMetrics()
*
* Action: Parse the character metrics entry in the input file
*      and set the width and bounding box in the afm structure.
*
****************************************************************
*/
BOOL ParseCharMetrics()
{
    int cChars;
    int i;
    int iWidth;
    int iChar;
    RECT rcChar;
#ifdef DBCS
    int iCharTmp;
#endif // DBCS
    for(i = 0 ; i < 256 ; i++)
        afm.rgcm[i].iWidth = -1;

    cChars = GetNumber();
    for (i=0; i<cChars; ++i)
    {
        if (GetLine())
        {
            printf("ParseCharMetrics: unexpected end of file encountered\n");
            exit(1);
        }
#ifdef DBCS
        // comments can happen anywhere.
        if (!strncmp("Comment", szLine, 7))
        {
            i--;
            continue;
        }
        if (*szLine != 'C')
        {
            printf("ParseCharMetrics: number of charmetrics entries did not match.\n");
            printf("only %d entries has been seen so far.\n", i);
            exit(1);
        }
#endif // DBCS
        iChar = ParseCharCode();

#ifdef DBCS
        if ((iWidth = ParseCharWidth()) < 0)
            iWidth = -1;
#else // DBCS
        iWidth = ParseCharWidth();
#endif // DBCS
 
        if (afm.iFamily==FF_DECORATIVE  ||  szIsEqual(afm.szFace, "Symbol"))
        //  Symbol is now classified as a Roman font, but like other
        //  decorative fonts, its CharNames cannot be translated.
        //  This check only works if afm.szFace is initialized by the
        //  time this Function is called.
        {
            while (*szLine!=0)
            if (*szLine++ == ';')
                break;
        }
        else
#ifdef DBCS
        {
            // if N entry exists, override current charcode with the
            // specified by N.
            if ((iCharTmp = ParseCharName()) >= 0)
                iChar = iCharTmp;
        }
#else // DBCS
            iChar = ParseCharName();
#endif // DBCS

        ParseCharBox(&rcChar);

#ifdef DBCS
        // throw away DBCS character metrics entries.  currently
        // we uniformly handle those character with one default
        // character metrics so that individual entries in AFM file
        // are just useless for them.
        if (iChar >= 0 && iChar <= 255)
#else // DBCS
        if (iChar > 0)
#endif // DBCS
        {
            afm.rgcm[iChar].iWidth = iWidth;
            afm.rgcm[iChar].rc.top = rcChar.top;
            afm.rgcm[iChar].rc.left = rcChar.left;
            afm.rgcm[iChar].rc.right = rcChar.right;
            afm.rgcm[iChar].rc.bottom = rcChar.bottom;
        }
        else
        {
            // printf("warning, negative char value\n");
        }
    }
    //  charcode.c  maps the bullet char to 149
    //  set width of all undefined chars to that of bullet.

    iWidth = afm.rgcm[149].iWidth;
    rcChar.top = afm.rgcm[149].rc.top;
    rcChar.left = afm.rgcm[149].rc.left;
    rcChar.right = afm.rgcm[149].rc.right;
    rcChar.bottom = afm.rgcm[149].rc.bottom;

    for(i = 0 ; i < 256 ; i++)
    {
        if (afm.rgcm[i].iWidth == -1)
        {
            afm.rgcm[i].iWidth = iWidth;
            afm.rgcm[i].rc.top = rcChar.top;
            afm.rgcm[i].rc.left = rcChar.left;
            afm.rgcm[i].rc.right = rcChar.right;
            afm.rgcm[i].rc.bottom = rcChar.bottom;
        }
    }
        
    GetLine();
    if (GetToken()!=TK_ENDCHARMETRICS)
    {
        printf("ParseCharMetrics: expected EndCharMetrics\n");
        printf("%s\n", rgbLine);
        exit(1);
    }
}



/***************************************************************
* Name: ParseCharBox()
*
* Action: Parse the character's bounding box and return its
*      dimensions in the destination rectangle.
*
****************************************************************
*/
ParseCharBox(prc)
RECT *prc;        /* Pointer to the destination rectangle */
    {
    char szWord[16];

    GetWord(szWord, sizeof(szWord));
    if (szIsEqual("B", szWord))
    {
    prc->left = GetNumber();
    prc->bottom = GetNumber();
    prc->right = GetNumber();
    prc->top = GetNumber();
    }
    else
    {
    printf("ParseCharBox: missing character box\n");
    printf("%s\n", rgbLine);
    exit(1);
    }
    EatWhite();
    if (*szLine++ != ';')
    {
    printf("ParseCharBox: missing semicolon\n");
    printf("%s\n", rgbLine);
    exit(1);
    }
    }




/*********************************************************
* Name: ParseCharName()
*
* Action: Parse a character's name and return its numeric value.
*
***********************************************************
*/
int ParseCharName()
    {
    int iChar;
    char szWord[18];

    EatWhite();
#ifdef DBCS
    // please note N is an optional entry.  it does not
    //  necessarily exist.
    if (*szLine != 'N')
        return -1;
#endif // DBCS
    GetWord(szWord, sizeof(szWord));
    if (szIsEqual("N", szWord))
    {
    GetWord(szWord, sizeof(szWord));
    iChar = GetCharCode(szWord);
    }
    else
    {
    printf("ParseCharName: expected name field\n");
    printf("%s\n", rgbLine);
    exit(1);
    }
    EatWhite();
    if (*szLine++ != ';')
    {
    printf("ParseCharName: expected semicolon\n");
    printf("%s\n", rgbLine);
    exit(1);
    }
    return(iChar);
    }




/***********************************************************
* Name: ParseCharWidth()
*
* Action: Parse a character's width and return its numeric
*      value.
*
************************************************************
*/
int ParseCharWidth()
    {
    int iWidth;
    char szWord[16];

#ifdef DBCS
    // please note N is an optional entry.  it does not
    //  necessarily exist.
    EatWhite();
    if (*szLine != 'W')
        return -1;
#endif // DBCS
    GetWord(szWord, sizeof(szWord));
#ifdef DBCS
    if (szIsEqual("WX", szWord) || szIsEqual("W0X", szWord))
#else // DBCS
    if (szIsEqual("WX", szWord))
#endif // DBCS
    {
    iWidth = GetNumber();
    if (iWidth==0)
        {
        printf("ParseCharWidth: zero character width\n");
        printf("%s\n", rgbLine);
        exit(1);
        }
    EatWhite();
    if (*szLine++ != ';')
        {
        printf("ParseCharWidth: missing semicolon\n");
        printf("%s\n", rgbLine);
        exit(1);
        }
    }
    else
    {
    printf("ParseCharWidth: expected \"WX\"\n");
    printf("%s\n", rgbLine);
    exit(1);
    }
    return(iWidth);
    }




/*****************************************************************
* Name: ParseCharCode()
*
* Action: Parse the ascii form of a character's code point and
*      return its numeric value.
*
*****************************************************************
*/
int ParseCharCode()
    {
    int iChar;
    char szWord[16];

    iChar = 0;
    GetWord(szWord, sizeof(szWord));
#ifdef DBCS
    if (szIsEqual("C", szWord) || szIsEqual("CH", szWord))
#else // DBCS
    if (szIsEqual("C", szWord))
#endif // DBCS
    {
    iChar = GetNumber();
    if (iChar==0)
        {
        printf("ParseCharCode: invalid character code\n");
        printf("%s", rgbLine);
        exit(1);
        }
    EatWhite();
    if (*szLine++ != ';')
        {
        printf("ParseCharCode: missing semicolon\n");
        printf("%s\n", rgbLine);
        exit(1);
        }
    }
    return(iChar);
    }



/****************************************************************
* Name: ParseBounding Box()
*
* Action: Parse a character's bounding box and return its size in
*      the afm structure.
*
******************************************************************
*/
ParseBoundingBox()
    {
    afm.rcBBox.left = GetNumber();
    afm.rcBBox.bottom = GetNumber();
    afm.rcBBox.right = GetNumber();
    afm.rcBBox.top = GetNumber();
    }




/************************************************************
* Name: ParsePitchType()
*
* Action: Parse the pitch type and set the variable pitch
*      flag in the afm structure.
*
*********************************************************
*/
int ParsePitchType()
    {
    int iChar;
    char szWord[16];
#ifdef DBCS
    BOOL bTmp;
#endif // DBCS

    EatWhite();
    GetWord(szWord, sizeof(szWord));
#ifdef DBCS
    bTmp = szIsEqual("true", szWord);
    if (afm.fVariablePitch < 0)
        afm.fVariablePitch = !bTmp;
    else if ((afm.fVariablePitch == TRUE && bTmp)
        || (afm.fVariablePitch == FALSE && !bTmp))
    {
        printf("afm: CharWidth and IsFixedPitch conflicted\n");
        exit(1);
    }
#else // DBCS
    if (szIsEqual("true", szWord))
    afm.fVariablePitch = FALSE;
    else
    afm.fVariablePitch = TRUE;
#endif // DBCS
    }

/************************************************************
* Name: ParseTrueType()
*
* Action: Parse the IsTrueType field and set the fFlags
*      field in the afm structure.
*
*********************************************************
*/
int ParseTrueType()
    {
    int iChar;
    char szWord[16];

    EatWhite();
    GetWord(szWord, sizeof(szWord));
    if (szIsEqual("true", szWord))
    afm.fFlags |= FFLAGS_TRUETYPE;
    }


int ParseEncodingScheme()
    {
    int iChar;
    char szWord[16];

    EatWhite();
    GetWord(szWord, sizeof(szWord));

    // note, we must read the font name before the encoding!

    }



/***********************************************************
* Name: InitAfm()
*
* Action: Initialize the afm structure.
*
***********************************************************
*/
InitAfm()
    {
    afm.iFirstChar = 0x20;
    afm.iLastChar = 0x0ff;
    afm.iAvgWidth = 0;
    afm.iMaxWidth = 0;
    afm.iItalicAngle = 0;
    afm.iFamily = 0;
    afm.ulOffset = 0;
    afm.ulThick = 0;
#ifdef DBCS
    afm.fVariablePitch = -1;
#else // DBCS
    afm.fVariablePitch = FALSE;
#endif // DBCS
    afm.szFile[0] = 0;
    afm.szFont[0] = 0;
    afm.szFace[0] = 0;
    afm.iWeight = 400;
    afm.kp.cPairs = 0;
    afm.kt.cTracks = 0;
    }




main(argc, argv)
int argc;
char **argv;
    {
    int iToken;
    BOOL fPrint;
    BOOL fEndOfInput;
    FILE *fopen();

    InitAfm();

#if defined(DBCS)
    if (argc < 3 || argc > 4)
    {
    fprintf( stderr,
        "usage: afm {/c<charset>} <afm filename> <ms database filename>\n"
        "  charset is one of the followings.\n"
        "    0 - ANSI_CHARSET\n"
        "    2 - SYMBOL_CHARSET\n"
        "    128 - SHIFTJIS_CHARSET\n"
        "    129 - HANGEUL_CHARSET\n"
        "    134 - GB2312_CHARSET\n"
        "    136 - CHINESEBIG5_CHARSET\n" );
      
    exit(1);
    }
#else // DBCS
    if (argc != 3)
    {
    printf("USAGE: AFM <AFM filename> <MS database filename>\n");
    exit(1);
    }
#endif // DBCS

#if defined(DBCS)
    if ( *argv[1] == '/' && *(argv[1] + 1) == 'c') {
        jWinCharset = atoi(argv[1] + 2);
        ++argv;
    }
#endif // DBCS

    ++argv;
    szMove(afm.szFile, *argv, sizeof(afm.szFile));


    /* open AFM file for input */
    fhIn = open(afm.szFile, O_RDONLY);
    if (fhIn < 0)
    {
    printf("afm: Can't open %s\n", afm.szFile);
    exit(1);
    }

    ++argv;

    /* open data file for input */
    if ((fhData = fopen(*argv, "r")) == NULL) 
    {
    printf("afm: Can't open %s\n", *argv);
    exit(1);
    }

    fPrint = FALSE;
    fEndOfInput = FALSE;
    while (!fEndOfInput)
    {
    GetLine();
    iToken = GetToken();
    switch(iToken)
        {
        case TK_STARTFONTMETRICS:
        break;
        case TK_STARTKERNDATA:
        ParseKernData();
        break;

        case TK_FONTNAME:
        ParseFontName();
        ParseMSFields();
        break;

        case TK_WEIGHT:
        ParseWeight();
        break;

        case TK_ITALICANGLE:
        afm.iItalicAngle = GetFloat(10);
        break;

        case TK_ISFIXEDPITCH:
        ParsePitchType();
        break;

        case TK_ENCODINGSCHEME:
        break;

        case TK_UNDERLINEPOSITION:
        afm.ulOffset = abs(GetNumber());
        break;

        case TK_UNDERLINETHICKNESS:
        afm.ulThick = GetNumber();
        break;

        case TK_FONTBBOX:
        ParseBoundingBox();
        break;

        case TK_CAPHEIGHT:
            afm.iCapHeight = GetNumber();
        break;

        case TK_XHEIGHT:
        break;

        case TK_DESCENDER:
                afm.iDescent = GetNumber();
        break;
        case TK_ASCENDER:
                afm.iAscent = GetNumber();
        break;
        case TK_STARTCHARMETRICS:
        if (afm.iFamily == 0)
            {
            printf("Missing \"MSFamily\" value\n");
            exit(1);
            }
        ParseCharMetrics();
        break;
        case TK_ENDFONTMETRICS:
        fEndOfInput = TRUE;
        break;

#ifdef DBCS
        case TK_CHARWIDTH:
            if (afm.fVariablePitch < 0)
                afm.fVariablePitch = FALSE;
            else if (afm.fVariablePitch == TRUE)
            {
                printf("afm: CharWidth and IsFixedPitch conflicted\n");
                exit(1);
            }
            break;
#endif // DBCS
            case TK_ISTRUETYPE:
                ParseTrueType();
                break;
        }

    szLine = rgbLine;

    if (fPrint)
        PrintLine(szLine);
    }

#if defined(DBCS)
    if (afm.fVariablePitch < 0)
        afm.fVariablePitch = FALSE;
#endif // DBCS

    close(fhIn);
    FixCharWidths();
    SetAfm();
    MakeDf(FALSE);
    }

DumpWidthTable()
    {
    int i;

    for (i=0; i<256; ++i)
    printf("%d\n", afm.rgcm[i].iWidth);
    }



/******************************************************
* Name: GetCharMetrics()
*
* Action: Get the character metrics for a specified character.
*
*********************************************************
*/
GetCharMetrics(iChar, pcm)
int iChar;
CM *pcm;
    {
    CM *pcmSrc;

    pcmSrc = &afm.rgcm[iChar];
    pcm->iWidth = pcmSrc->iWidth;
    pcm->rc.top = pcmSrc->rc.top;
    pcm->rc.left = pcmSrc->rc.left;
    pcm->rc.bottom = pcmSrc->rc.bottom;
    pcm->rc.right = pcmSrc->rc.right;
    }



/*************************************************************
* Name: SetCharMetrics()
*
* Action: Set the character metrics for a specified character.
*
**************************************************************
*/
SetCharMetrics(iChar, pcm)
int iChar;
CM *pcm;
    {
    CM *pcmDst;

    pcmDst = &afm.rgcm[iChar];
    pcmDst->iWidth = pcm->iWidth;
    pcmDst->rc.top = pcm->rc.top;
    pcmDst->rc.left = pcm->rc.left;
    pcmDst->rc.bottom = pcm->rc.bottom;
    pcmDst->rc.right = pcm->rc.right;
    }




/************************************************************
* Name: GetSmallCM()
*
* Action: Compute the character metrics for small sized characters
*      such as superscripts.
*
*************************************************************
*/
GetSmallCM(iCh, pcm)
int iCh;
CM *pcm;
    {

    GetCharMetrics(iCh, pcm);
    pcm->iWidth = pcm->iWidth / 2;
    pcm->rc.bottom = pcm->rc.top + (pcm->rc.top - pcm->rc.bottom)/2;
    pcm->rc.right = pcm->rc.left + (pcm->rc.right - pcm->rc.left)/2;
    }




/*************************************************************
* Name: SetFractionMetrics()
*
* Action: Set the character metrics for a fractional character
*      which must be simulated.
*
**************************************************************
*/
SetFractionMetrics(iChar, iTop, iBottom)
int iChar;    /* The character code point */
int iTop;    /* The ascii numerator character */
int iBottom;    /* The denominator character */
    {
    int cxTop;        /* The width of the numerator */
    int cxBottom;   /* The width of the denominator */
    CM cm;
#define IFRACTIONBAR  167


    /* Set denominator width to 60 percent of bottom character */
    GetCharMetrics(iBottom, &cm);
    cxBottom = (((long)cm.iWidth) * 60L)/100L;

    /* Set numerator width to 40 percent of top character */
    GetCharMetrics(iTop, &cm);
    cxTop = (((long)cm.iWidth) * 40L)/100L;

    cm.iWidth = iTop + iBottom + IFRACTIONBAR;
    cm.rc.right = cm.rc.left + cm.iWidth;
    SetCharMetrics(iChar, &cm);

    }




/***********************************************************************
* Name: FixCharWidths()
*
* Action: Fix up the character widths for those characters which
*      must be simulated in the driver.
*
************************************************************************
*/
FixCharWidths()
    {
    CM cm1;
    int i;

#ifdef DBCS
    // this is not applicable to non-Latin character set, right?
    if (afm.iEncodingScheme != ANSI_CHARSET)
    {
        return;
    }
#endif // DBCS

    /* set non-breaking space to space */
    GetCharMetrics(' ', &cm1);
    SetCharMetrics(0xA0, &cm1);

    /* set logicalnot character */
    i = GetCharCode("logicalnot");
    GetCharMetrics(i, &cm1);
    SetCharMetrics(0xAC, &cm1);

    /* set sfthyphen character */
    i = GetCharCode("hyphen");
    GetCharMetrics(i, &cm1);
    SetCharMetrics(0xAD, &cm1);

    /* set overstore character */
    i = GetCharCode("macron");
    GetCharMetrics(i, &cm1);
    SetCharMetrics(0xAF, &cm1);

    /* set acute character */
    i = GetCharCode("acute");
    GetCharMetrics(i, &cm1);
    SetCharMetrics(0xB4, &cm1);

    }


/***************************************************************
* Name: SetAfm()
*
* Action: Set the character metrics in the afm to their default values.
*
*******************************************************************
*/
SetAfm()
    {
    int i, cx;

    afm.iFirstChar = 0x020;
#ifdef DBCS
    // since we do not hold individual width informationm, the
    // lastchar is 256 which is the last code point for SBCS
    // part of DBCS codes.
#endif // DBCS
    afm.iLastChar = 0x0ff;

    if (!afm.fVariablePitch)
    {
    cx = afm.rgcm[afm.iFirstChar].iWidth;
    for (i=afm.iFirstChar; i<=afm.iLastChar; ++i)
    {
#ifdef DBCS
        if (IS_DBCS_CHARSET(afm.iEncodingScheme))
            afm.rgcm[i].iWidth = DBCS_MAX_CHARWIDTH / 2;
        else
#endif // DBCS
        afm.rgcm[i].iWidth = cx;
    }
    }

    SetAvgWidth();
    SetMaxWidth();
    }



/******************************************************************
* Name: SetAvgWidth()
*
* Action: This routine computes the average character width
*      from the character metrics in the afm structure.
*
*******************************************************************
*/
SetAvgWidth()
{
    CM *rgcm;

    rgcm = afm.rgcm;

//    cb = (long) (afm.iLastChar - afm.iFirstChar + 1);
//    for (i=afm.iFirstChar; i<=afm.iLastChar; ++i)
//    {
//        cx += (long) rgcm[i].iWidth;
//    }
//    afm.iAvgWidth = cx/cb;

#ifdef DBCS
    // we have fixed ave character width for DBCS font
    if (IS_DBCS_CHARSET(afm.iEncodingScheme))
    {
        afm.iAvgWidth = DBCS_AVE_CHARWIDTH;
        return;
    }
#endif // DBCS

        //------------------
        // case #2 AveWidth
        // per OS/2 formula 
        //------------------

    afm.iAvgWidth = 
            (WORD)((((DWORD)rgcm[97].iWidth *  64 ) + // a
                    ((DWORD)rgcm[98].iWidth *  14 ) + // b
                    ((DWORD)rgcm[99].iWidth *  27 ) + // c
                    ((DWORD)rgcm[100].iWidth * 35 ) + // d
                    ((DWORD)rgcm[101].iWidth * 100) + // e
                    ((DWORD)rgcm[102].iWidth * 20 ) + // f
                    ((DWORD)rgcm[103].iWidth * 14 ) + // g
                    ((DWORD)rgcm[104].iWidth * 42 ) + // h
                    ((DWORD)rgcm[105].iWidth * 63 ) + // i
                    ((DWORD)rgcm[106].iWidth * 3  ) + // j
                    ((DWORD)rgcm[107].iWidth * 6  ) + // k
                    ((DWORD)rgcm[108].iWidth * 35 ) + // l
                    ((DWORD)rgcm[109].iWidth * 20 ) + // m
                    ((DWORD)rgcm[110].iWidth * 56 ) + // n
                    ((DWORD)rgcm[111].iWidth * 56 ) + // o
                    ((DWORD)rgcm[112].iWidth * 17 ) + // p
                    ((DWORD)rgcm[113].iWidth * 4  ) + // q
                    ((DWORD)rgcm[114].iWidth * 49 ) + // r
                    ((DWORD)rgcm[115].iWidth * 56 ) + // s
                    ((DWORD)rgcm[116].iWidth * 71 ) + // t
                    ((DWORD)rgcm[117].iWidth * 31 ) + // u 
                    ((DWORD)rgcm[118].iWidth * 10 ) + // v
                    ((DWORD)rgcm[119].iWidth * 18 ) + // w
                    ((DWORD)rgcm[120].iWidth * 3  ) + // x
                    ((DWORD)rgcm[121].iWidth * 18 ) + // y
                    ((DWORD)rgcm[122].iWidth * 2  ) + // z 
                    ((DWORD)rgcm[32].iWidth *  166))/ // sp
                    1000L);
}





/*****************************************************************
* Name: SetMaxWidth()
*
* Action: This routine computes the maximum character width from
*      the character metrics in the afm structure.
*
******************************************************************
*/
SetMaxWidth()
    {
    CM *rgcm;
    short cx;
    int i;

    rgcm = afm.rgcm;

#ifdef DBCS
    // we have fixed max character width for DBCS font
    if (IS_DBCS_CHARSET(afm.iEncodingScheme))
    {
        afm.iMaxWidth = DBCS_MAX_CHARWIDTH;
        return;
    }
#endif // DBCS

    cx = 0;
    for (i=afm.iFirstChar; i<=afm.iLastChar; ++i)
    if (rgcm[i].iWidth > cx)
        cx = rgcm[i].iWidth;
    afm.iMaxWidth = cx;
    }
