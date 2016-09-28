/**[f******************************************************************
 * pfmdump.c -
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/


#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>

#define NULL 0
#define TRUE 1
#define FALSE 0


typedef int BOOL;
typedef char BYTE;
typedef short int WORD;
typedef long int DWORD;



char rgbBuffer[8192];
char *pbBuffer;
int cbBuffer;

WORD dfVersion;        /* Version 1.00 */
DWORD dfSize;
char dfCopyright[60];
WORD dfType;
WORD dfPoints;
WORD dfVertRes;
WORD dfHorizRes;
WORD dfAscent;
WORD dfInternalLeading;
WORD dfExternalLeading;
BYTE dfItalic;
BYTE dfUnderline;
BYTE dfStrikeOut;
WORD dfWeight;
BYTE dfCharSet;
WORD dfPixWidth;
WORD dfPixHeight;
BYTE dfPitchAndFamily;
WORD dfAvgWidth;
WORD dfMaxWidth;
BYTE dfFirstChar;
BYTE dfLastChar;
BYTE dfDefaultChar;
BYTE dfBreakChar;
WORD dfWidthBytes;
DWORD dfDevice;
DWORD dfFace;
DWORD dfBitsPointer;
DWORD dfBitsOffset;
WORD dfSizeFields;
DWORD dfExtMetricsOffset;
DWORD dfExtentTable;
DWORD dfOriginTable;
DWORD dfPairKernTable;
DWORD dfTrackKernTable;
DWORD dfDriverInfo;
DWORD dfReserved;


short etmSize;
short etmPointSize;
short etmOrientation;
short etmMasterHeight;
short etmMinScale;
short etmMaxScale;
short etmMasterUnits;
short etmCapHeight;
short etmXHeight;
short etmLowerCaseAscent;
short etmLowerCaseDecent;
short etmSlant;
short etmSuperScript;
short etmSubScript;
short etmSuperScriptSize;
short etmSubScriptSize;
short etmUnderlineOffset;
short etmUnderlineWidth;
short etmDoubleUpperUnderlineOffset;
short etmDoubleLowerUnderlineOffset;
short etmDoubleUpperUnderlineWidth;
short etmDoubleLowerUnderlineWidth;
short etmStrikeOutOffset;
short etmStrikeOutWidth;
WORD etmNKernPairs;
WORD etmNKernTracks;





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



ReadPfm(szFile)
char *szFile;
    {
    int fh;


    fh = open(szFile, O_RDWR | O_BINARY);
    if (fh<=0)
        {
        printf("pfmdump: Can't open %s\n", szFile);
        exit(1);
        }
    cbBuffer = read(fh, rgbBuffer, sizeof(rgbBuffer));
    if (cbBuffer<=0)
        {
        printf("pfmdump: Can't read %s\n", szFile);
        exit(1);
        }
    close(fh);
    pbBuffer = rgbBuffer;
    GetDf();
    }

int GetByte()
    {
    if (cbBuffer>0)
        {
        --cbBuffer;
        return(((short int )*pbBuffer++) & 0x0ff);
        }
    else
        return(0);
    }


int GetWord()
    {
    int iWord;

    if (cbBuffer>=2)
        {
        iWord = GetByte();
        iWord |= GetByte() << 8;
        }
    else
        iWord = 0;
    return(iWord);
    }

long GetLong()
    {
    long lWord;

    if (cbBuffer>=4)
        {
        lWord = *((long int *)pbBuffer)++;
        cbBuffer -= 4;
        }
    else
        lWord = 0L;

    return(lWord);
    }

GetRgb(pbDst, cbDst)
char *pbDst;
int cbDst;
    {
    if (cbDst <= cbBuffer)
        {
        while (cbDst>0)
            {
            *pbDst++ = *pbBuffer++;
            --cbDst;
            --cbBuffer;
            }
        }
    }

GetEtm()
    {
    pbBuffer = rgbBuffer + dfExtMetricsOffset;
    cbBuffer = sizeof(rgbBuffer) - dfExtMetricsOffset;

    etmSize = GetWord();
    etmPointSize = GetWord();
    etmOrientation = GetWord();
    etmMasterHeight = GetWord();
    etmMinScale = GetWord();
    etmMaxScale = GetWord();
    etmMasterUnits = GetWord();
    etmCapHeight = GetWord();
    etmXHeight = GetWord();
    etmLowerCaseAscent = GetWord();
    etmLowerCaseDecent = GetWord();
    etmSlant = GetWord();
    etmSuperScript = GetWord();
    etmSubScript = GetWord();
    etmSuperScriptSize = GetWord();
    etmSubScriptSize = GetWord();
    etmUnderlineOffset = GetWord();
    etmUnderlineWidth = GetWord();
    etmDoubleUpperUnderlineOffset = GetWord();
    etmDoubleLowerUnderlineOffset = GetWord();
    etmDoubleUpperUnderlineWidth = GetWord();
    etmDoubleLowerUnderlineWidth = GetWord();
    etmStrikeOutOffset = GetWord();
    etmStrikeOutWidth = GetWord();
    etmNKernPairs = GetWord();
    etmNKernTracks = GetWord();
    }

GetDf()
    {
    dfVersion = GetWord();
    dfSize = GetLong();
    GetRgb(dfCopyright, 60);
    dfType = GetWord();
    dfPoints = GetWord();
    dfVertRes = GetWord();
    dfHorizRes = GetWord();
    dfAscent = GetWord();
    dfInternalLeading = GetWord();
    dfExternalLeading = GetWord();
    dfItalic = GetByte();
    dfUnderline = GetByte();
    dfStrikeOut = GetByte();
    dfWeight = GetWord();
    dfCharSet = GetByte();
    dfPixWidth = GetWord();
    dfPixHeight = GetWord();
    dfPitchAndFamily = GetByte();
    dfAvgWidth = GetWord();
    dfMaxWidth = GetWord();
    dfFirstChar = GetByte();
    dfLastChar = GetByte();
    dfDefaultChar = GetByte();
    dfBreakChar = GetByte();
    dfWidthBytes = GetWord();
    dfDevice = GetLong();
    dfFace = GetLong();
    dfBitsPointer = GetLong();
    dfBitsOffset = GetLong();
    dfSizeFields = GetWord();
    dfExtMetricsOffset = GetLong();
    dfExtentTable = GetLong();
    dfOriginTable = GetLong();
    dfPairKernTable = GetLong();
    dfTrackKernTable = GetLong();
    dfDriverInfo = GetLong();
    dfReserved = GetLong();
    }


PrintDeviceName()
    {
    if (dfDevice)
        {
        pbBuffer = rgbBuffer + dfDevice;
        printf("Device name = \"%s\"\n", pbBuffer);
        }
    else
        printf("Device name = \"\"\n");


    }

PrintFaceName()
    {
    if (dfFace)
        {
        pbBuffer = rgbBuffer + dfFace;
        printf("Face name = \"%s\"\n", pbBuffer);
        }
    else
        printf("Face name = \"\"\n");
    }

PrintFontName()
    {
    if (dfDriverInfo)
        {
        pbBuffer = rgbBuffer + dfDriverInfo;
        printf("Font name = \"%s\"\n", pbBuffer);
        }
    else
        printf("Font name = \"\"\n");

    }




PrintExtentTable()
    {
    int i;
    int cb;
    int cbLine;
    int iFirst;
    int iLast;


    pbBuffer = rgbBuffer + dfExtentTable;
    cbBuffer = sizeof(rgbBuffer) - dfExtentTable;


    printf("\n\n");
    printf("===== Extent Table ====\n");
    printf("\n");
    iFirst = dfFirstChar & 0x0ff;
    iLast = dfLastChar & 0x0ff;


    while (iFirst <= iLast)
        {
        printf("%02x: ", iFirst);
        if ((iLast - iFirst + 1) > 16)
            cbLine = 16;
        else
            cbLine = iLast - iFirst + 1;

        for (i = 0; i<cbLine; ++i)
            printf("%d ", GetWord());
        printf("\n");
        iFirst += cbLine;
        }

    }
PrintPairKernTable()
    {
    int i;
    int cb;
    int cbLine;
    int cPairs;


    if (!dfPairKernTable)
        return;
    pbBuffer = rgbBuffer + dfPairKernTable;
    cbBuffer = sizeof(rgbBuffer) - dfPairKernTable;


    printf("\n\n");
    printf("===== Pair Kern Table ====\n");
    printf("\n");
    cPairs = GetWord();
    printf("Number of Pairs = %d\n", cPairs);

    for (i=0; i<cPairs; ++ i)
        {
        printf("iKey = %04x  ", GetWord());
        printf("iKernAmount = %d\n", GetWord());
        }
    printf("\n\n");

    }




PrintEtm()
    {
    GetEtm();

    printf("\n");
    printf("==== Extended text metrics ====\n");
    printf("\n");

    printf("etmSize = %d\n", etmSize);
    printf("etmPointSize = %d\n", etmPointSize);
    printf("etmOrientation = %d\n", etmOrientation);
    printf("etmMasterHeight = %d\n", etmMasterHeight);
    printf("etmMinScale = %d\n", etmMinScale);
    printf("etmMaxScale = %d\n", etmMaxScale);
    printf("etmMasterUnits = %d\n", etmMasterUnits);
    printf("etmCapHeight = %d\n", etmCapHeight);
    printf("etmXHeight = %d\n", etmXHeight);
    printf("etmLowerCaseAscent = %d\n", etmLowerCaseAscent);
    printf("etmLowerCaseDecent = %d\n", etmLowerCaseDecent);
    printf("etmSlant = %d\n", etmSlant);
    printf("etmSuperScript = %d\n", etmSuperScript);
    printf("etmSubScript = %d\n", etmSubScript);
    printf("etmSuperScriptSize = %d\n", etmSuperScriptSize);
    printf("etmSubScriptSize = %d\n", etmSubScriptSize);
    printf("etmUnderlineOffset = %d\n", etmUnderlineOffset);
    printf("etmUnderlineWidth = %d\n", etmUnderlineWidth);
    printf("etmDoubleUpperUnderlineOffset = %d\n", etmDoubleUpperUnderlineOffset);
    printf("etmDoubleLowerUnderlineOffset = %d\n", etmDoubleLowerUnderlineOffset);
    printf("etmDoubleUpperUnderlineWidth = %d\n", etmDoubleUpperUnderlineWidth);
    printf("etmDoubleLowerUnderlineWidth = %d\n", etmDoubleLowerUnderlineWidth);
    printf("etmStrikeOutOffset = %d\n", etmStrikeOutOffset);
    printf("etmStrikeOutWidth = %d\n", etmStrikeOutWidth);
    printf("etmNKernPairs = %d\n", etmNKernPairs);
    printf("etmNKernTracks = %d\n", etmNKernTracks);
    printf("\n");
    }



PrintDf()
    {


    printf("\n");
    printf("dfVersion = %04xH\n", dfVersion);
    printf("dfSize = %d\n", dfSize);
    printf("dfCopyright = \"%s\"\n", dfCopyright);
    printf("dfType = %04x\n", dfType);
    printf("dfPoints = %d\n", dfPoints);
    printf("dfVertRes = %d\n", dfVertRes);
    printf("dfHorizRes = %d\n", dfHorizRes);
    printf("dfAscent = %d\n", dfAscent);
    printf("dfInternalLeading = %d\n", dfInternalLeading);
    printf("dfExternalLeading = %d\n", dfExternalLeading);
    printf("dfItalic = %d\n", dfItalic);
    printf("dfUnderline = %d\n", dfUnderline);
    printf("dfStrikeOut = %d\n", dfStrikeOut);
    printf("dfWeight = %d\n", dfWeight);
    printf("dfCharSet = %02xH\n", dfCharSet & 0x0ff);
    printf("dfPixWidth = %d\n", dfPixWidth);
    printf("dfPixHeight = %d\n", dfPixHeight);
    printf("dfPitchAndFamily = %02xH\n", dfPitchAndFamily & 0x0ff);
    printf("dfAvgWidth = %d\n", dfAvgWidth);
    printf("dfMaxWidth = %d\n", dfMaxWidth);
    printf("dfFirstChar = %02xH\n", dfFirstChar & 0x0ff);
    printf("dfLastChar = %02xH\n", dfLastChar & 0x0ff);
    printf("dfDefaultChar = %02xH\n", dfDefaultChar & 0x0ff);
    printf("dfBreakChar = %02xH\n", dfBreakChar);
    printf("dfWidthBytes = %d\n", dfWidthBytes);
    printf("dfDevice = %08lxH\n", dfDevice);
    printf("dfFace = %08lxH\n", dfFace);
    printf("dfBitsPointer = %08lxH\n", dfBitsPointer);
    printf("dfBitsOffset = %08lxH\n", dfBitsOffset);
    printf("dfSizeFields = %d\n", dfSizeFields);
    printf("dfExtMetricsOffset = %08lxH\n", dfExtMetricsOffset);
    printf("dfExtentTable = %08lxH\n", dfExtentTable);
    printf("dfOriginTable = %08lxH\n", dfOriginTable);
    printf("dfPairKernTable = %08lxH\n", dfPairKernTable);
    printf("dfTrackKernTable = %08lxH\n", dfTrackKernTable);
    printf("dfDriverInfo = %08lxH\n", dfDriverInfo);
    printf("dfReserved = %08lxH\n", dfReserved);


    }




DumpBuffer()
    {
    char *pb;
    int i;
    int cb;
    int cbRow;


    pb = pbBuffer;


    cb = cbBuffer;
    while (cbBuffer>0)
        {
        if (cbBuffer > 16)
            cbRow = 16;
        else
            cbRow = cb;
        cbRow = cbRow / 2;

        for (i=0; i<cbRow; ++i)
            printf("%04x ", GetWord());
        printf("\n");
        }
    printf("\n\n");
    }

ValidateName(sz)
char *sz;
    {
    while (*sz!=0 && *sz!='.')
        ++sz;
    if (!szIsEqual(".pfm", sz))
        {
        printf("pfmdump: invalid pfm file\n");
        exit(1);
        }
    }

main(argc, argv)
int argc;
char **argv;
    {
    if (argc!=2)
        {
        printf("pfmdump: wrong number of arguments\n");
        exit(1);
        }

    ++argv;

    ValidateName(*argv);

    ReadPfm(*argv);

    printf("\n");
    printf("WINDOWS PRINTER FONT METRICS: %s\n", *argv);

    printf("\n");
    PrintDeviceName();
    PrintFaceName();
    PrintFontName();

    PrintDf();

    PrintEtm();
    if (dfPitchAndFamily & 1)
        PrintExtentTable();
    PrintPairKernTable();


    }
