/**[f******************************************************************
 * pfm.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 * The following diagram shows the relationship between the font
 * information from the AFM file (FontBBox) and the values we
 * generate for the windows font metrics.  Note that the leading computed
 * for the font is divided into internal and external leading inside
 * the driver (it is stored in the dfInternalLeading field however)
 * using the rules found their for the proper amounts of external 
 * leading for the given point sizes.
 *
 *	+--------------------+	
 *	| 	     ^	     | ^
 *	|     Leading| 	     | | <-- FontBBox
 *	| 	     v	     | |
 *	| +---------------+  | |
 *	| |		  |  | |
 *	| |		  |  | | Ascent
 *	| |		  |  | |
 *	| | Em Square	  |  | |
 *	| |		  |  | |
 *	| | (1000x1000)   |  | |
 *	| |		  |  | v
 *	+-+---------------+--+
 *	| |0,0		  |  |
 *	| |		  |  |
 *	+-+---------------+--+
 *
 **f]*****************************************************************/

#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>

#define EM 1000
#include "pfm.h"

extern FONT *rgpFont[];

char rgbBuffer[8192];
int cbBuffer;
char *pbBuffer;

/****************************************************************************/
#ifdef DEBUG_ON
#define DBMSG(msg) printf msg
#else
#define DBMSG(msg)
#endif
/****************************************************************************/



WORD dfVersion = 0x0100;	/* Version 1.00 */
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
WORD dfPitchAndFamily;
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




/******************************************************************
* Name: ResetBuffer()
*
* Action: This function resets the output buffer.
*
*******************************************************************
*/
ResetBuffer()
    {
    pbBuffer = rgbBuffer;
    cbBuffer = 0;
    }



/****************************************************************
* Name: PutByte()
*
* Action: This function writes a byte to the output buffer.
*
*****************************************************************
*/
PutByte(iByte)
short int iByte;
    {
    *pbBuffer++ = iByte & 0x0ff;
    ++cbBuffer;
    }







/****************************************************************
* Name: PutRgb()
*
* Action: This function writes an array of bytes to the output buffer.
*
*****************************************************************
*/
PutRgb(pb, cb)
char *pb;
int cb;
    {
    while (--cb>=0)
	PutByte(*pb++);
    }





/****************************************************************
* Name: PutWord()
*
* Action: This function writes a word to the output buffer.
*
*****************************************************************
*/

PutWord(iWord)
short int iWord;
    {
    *pbBuffer++ = iWord & 0x0ff;
    *pbBuffer++ = (iWord >> 8) & 0x0ff;
    cbBuffer += 2;
    }




/****************************************************************
* Name: PutLong()
*
* Action: This function writes a long word to the output buffer.
*
*****************************************************************
*/
PutLong(lWord)
long lWord;
    {
    PutWord((int) (lWord & 0x0ffffL));
    lWord >>= 16;
    PutWord((int) (lWord & 0x0ffffL));
    }







/**************************************************************
* Name: SetDf()
*
* Action: This function sets the values in the device font structure
*	  from the values in the afm structure.
*
****************************************************************
*/
SetDf()
    {
    short *piSrc;
    short *piDst;
    int i;

#ifdef DBCS
    // well, does anyone care?
    szMove(dfCopyright, "Copyright 1986-1994 Microsoft Corporation", sizeof(dfCopyright));
#else // DBCS
    szMove(dfCopyright, "Copyright 1986 Microsoft Corporation", sizeof(dfCopyright));
#endif // DBCS

    /* A device realized vector font */

    dfType = 0x081;
    if (afm.fFlags & FFLAGS_TRUETYPE)
      dfType |= 0x200;     /* set resident TrueType bit */

//    dfCharSet = ANSI_CHARSET;
    //  afm.iFirstChar is hardcoded to be 32.

    dfCharSet = afm.iEncodingScheme;
#ifdef DBCS
    if (IS_DBCS_CHARSET(afm.iEncodingScheme))
        dfDefaultChar = 0;
    else
#else // DBCS
    dfDefaultChar = 149 - afm.iFirstChar;  
#endif // DBCS
    dfBreakChar = 0;


#if 1

    /*
     *
     * old old method.
     *
     * technically this correct, but on the basis of Windows and the ability
     * to have a negative font height request (ignoring internal leading)
     * leads to problems when fonts are requeset in the header. i.e. fonts
     * end up too large.
     *
     * dfInternalLeading = EM - (afm.iAscent - afm.iDescent);
     */

    // NEW METHOD
    //
    // we compute the leading as the distance from the top of the
    // ink (the top of the bounding box) to the top of the Em square
    //
    // we set the internal leading here, and then when the font is
    // realized we split the leading between the internal and the
    // external, based on the non-linear rules for computing external
    // leading.

    i = (afm.rcBBox.top - afm.rcBBox.bottom) - EM;

    if (i < 0) {
	// if this is negative clip it at zero
    	printf("Warning: negative internal leading: %d\n", i);
	dfInternalLeading = 0;
    } else
	dfInternalLeading = (WORD)i;

    // and the ascent is from the baseline to the top of the bounding box.

    dfAscent = afm.rcBBox.top;

#else

    // old method, always set internal leading to zero

    dfInternalLeading=0;

    dfAscent = afm.iAscent;
#endif

    dfPoints = 10;	/* Default to 80 column text */
    dfVertRes = 300;
    dfHorizRes = 300;

    dfExternalLeading = 196;

    dfItalic = afm.iItalicAngle !=0 ? 1 : 0;
    dfWeight = afm.iWeight;
    dfPitchAndFamily = afm.iFamily;
    dfFirstChar = afm.iFirstChar;
    dfLastChar = afm.iLastChar;

    dfAvgWidth = afm.iAvgWidth;
    dfMaxWidth = afm.iMaxWidth;
#ifdef DBCS
    if (afm.fVariablePitch == TRUE)
#else // DBCS
    if (afm.fVariablePitch)
#endif // DBCS
	dfPitchAndFamily |= 1;
    }




/**********************************************************
* Name: PutString()
*
* Action: This function writes a null terminated string
*	  to the output file.
*
**********************************************************
*/
PutString(sz)
char *sz;
    {
    int bCh;

    do
	{
	bCh = *pbBuffer++ = *sz++;
	++cbBuffer;
	} while (bCh);
    }



/***************************************************************
* Name: PutdeviceName()
*
* Action: This function writes the device name to the output file.
*
*************************************************************
*/
PutDeviceName(szDevice)
char *szDevice;
    {
    dfDevice = cbBuffer;
    PutString(szDevice);
    }




/***************************************************************
* Name: PutFaceName()
*
* Action: This function writes the font's face name to the output file.
*
*************************************************************
*/
PutFaceName()
    {
    dfFace = cbBuffer;
    PutString(afm.szFace);
    }







/***************************************************************
* Name: PutFontName()
*
* Action: This function writes the font name to the output file.
*
*************************************************************
*/

PutFontName()
    {
    dfDriverInfo = cbBuffer;
    PutString(afm.szFont);
    }






/**************************************************************
* Name: MakeDf()
*
* Action: This function writes the device font info structure
*	  to the output file.
*
* Method: This function makes two passes over the data. On the
*	  first pass it collects offset data as it places data
*	  in the output buffer. On the second pass, it first
*	  resets the output buffer and then writes the data
*	  to the output buffer again with the offsets computed
*	  from pass 1.
*
**************************************************************
*/
int MakeDf(fPass2)
BOOL fPass2;		/* TRUE if this is the second pass */
    {
    int iMarker;
    int i;

    ResetBuffer();
    SetDf();

    PutWord(dfVersion);
    PutLong(dfSize);
    PutRgb(dfCopyright, 60);
    PutWord(dfType);
    PutWord(dfPoints);
    PutWord(dfVertRes);
    PutWord(dfHorizRes);
    PutWord(dfAscent);
    PutWord(dfInternalLeading);
    PutWord(dfExternalLeading);
    PutByte(dfItalic);
    PutByte(dfUnderline);
    PutByte(dfStrikeOut);
    PutWord(dfWeight);
    PutByte(dfCharSet);
    PutWord(dfPixWidth);
    PutWord(dfPixHeight);
    PutByte(dfPitchAndFamily);
    PutWord(dfAvgWidth);
    PutWord(dfMaxWidth);
    PutByte(dfFirstChar);
    PutByte(dfLastChar);
    PutByte(dfDefaultChar);
    PutByte(dfBreakChar);
    PutWord(dfWidthBytes);
    PutLong(dfDevice);
    PutLong(dfFace);
    PutLong(dfBitsPointer);
    PutLong(dfBitsOffset);


    iMarker = cbBuffer;

    PutWord(dfSizeFields);
    PutLong(dfExtMetricsOffset);
    PutLong(dfExtentTable);
    PutLong(dfOriginTable);
    PutLong(dfPairKernTable);
    PutLong(dfTrackKernTable);
    PutLong(dfDriverInfo);
    PutLong(dfReserved);

    dfSizeFields = cbBuffer - iMarker;

#if FALSE
    printf("dfVersion=%d\n",dfVersion);
    printf("dfSize=%ld\n",dfSize);
    printf("dfCopyright=%s\n",dfCopyright);
    printf("dfType=%d\n",dfType);
    printf("dfPoints=%d\n",dfPoints);
    printf("dfVertRes=%d\n",dfVertRes);
    printf("dfHorizRes=%d\n",dfHorizRes);
    printf("dfAscent=%d\n",dfAscent);
    printf("dfInternalLeading=%d\n",dfInternalLeading);
    printf("dfExternalLeading=%d\n",dfExternalLeading);
    printf("dfItalic=%d\n",dfItalic);
    printf("dfUnderline=%d\n",dfUnderline);
    printf("dfStrikeOut=%d\n",dfStrikeOut);
    printf("dfWeight=%d\n",dfWeight);
    printf("dfCharSet=%d\n",dfCharSet);
    printf("dfPixWidth=%d\n",dfPixWidth);
    printf("dfPixHeight=%d\n",dfPixHeight);
    printf("dfPitchAndFamily=%d\n",dfPitchAndFamily);
    printf("dfAvgWidth=%d\n",dfAvgWidth);
    printf("dfMaxWidth=%d\n",dfMaxWidth);
    printf("dfFirstChar=%d\n",dfFirstChar);
    printf("dfLastChar=%d\n",dfLastChar);
    printf("dfDefaultChar=%d\n",dfDefaultChar);
    printf("dfBreakChar=%d\n",dfBreakChar);
    printf("dfWidthBytes=%d\n",dfWidthBytes);
    printf("dfDevice=%ld\n",dfDevice);
    printf("dfFace=%ld\n",dfFace);
    printf("dfBitsPointer=%ld\n",dfBitsPointer);
    printf("dfBitsOffset=%ld\n",dfBitsOffset);
    printf("dfSizeFields=%d\n",dfSizeFields);
    printf("dfExtMetricsOffset=%ld\n",dfExtMetricsOffset);
    printf("dfExtentTable=%ld\n",dfExtentTable);
    printf("dfOriginTable=%ld\n",dfOriginTable);
    printf("dfPairKernTable=%ld\n",dfPairKernTable);
    printf("dfTrackKernTable=%ld\n",dfTrackKernTable);
    printf("dfDriverInfo=%ld\n",dfDriverInfo);
    printf("dfReserved=%ld\n",dfReserved);
#endif


    /* Put the extended text metrics table */
    dfExtMetricsOffset = cbBuffer;

    PutEtm();

    PutDeviceName("PostScript");
    PutFaceName();
    PutFontName();



    /* Put the extent table */
    dfExtentTable = cbBuffer;
    PutExtentTable();

    /* Put the origion table */
    dfOriginTable = cbBuffer;
    PutOriginTable();


    /* Put the pair kerning table */
    PutPairKernTable();

    /* Put the track kerning table */
    PutTrackKernTable();

    dfOriginTable = NULL;

    if (!fPass2)
	{
	dfSize = cbBuffer;
	MakeDf(TRUE);
	}
    else
	WritePfm();
    }



#ifdef UNDEFINED

int MakeDirEntry(fPass2, iFont)
BOOL fPass2;		/* TRUE if this is the second pass */
int iFont;
    {
    int iMarker;
    int i;

    ResetBuffer();
    SetDf();

    dfDriverInfo = 0L;
    dfPairKernTable = 0L;
    dfTrackKernTable = 0L;

    PutWord(dfSize);
    PutWord(iFont+1);
    PutWord(dfType);
    PutWord(dfPoints);
    PutWord(dfVertRes);
    PutWord(dfHorizRes);
    PutWord(dfAscent);
    PutWord(dfInternalLeading);
    PutWord(dfExternalLeading);
    PutByte(dfItalic);
    PutByte(dfUnderline);
    PutByte(dfStrikeOut);
    PutWord(dfWeight);
    PutByte(dfCharSet);
    PutWord(dfPixWidth);
    PutWord(dfPixHeight);
    PutByte(dfPitchAndFamily);
    PutWord(dfAvgWidth);
    PutWord(dfMaxWidth);
    PutByte(dfFirstChar);
    PutByte(dfLastChar);
    PutByte(dfDefaultChar);
    PutByte(dfBreakChar);
    PutWord(dfWidthBytes);

    PutLong(dfDevice - 4L);
    PutLong(dfFace - 4L);
    PutLong(dfBitsPointer);
    PutLong(dfBitsOffset);
    PutWord(dfSizeFields);
    PutLong(dfExtMetricsOffset);
    PutLong(dfExtentTable);
    PutLong(dfOriginTable);
    PutLong(dfPairKernTable);
    PutLong(dfTrackKernTable);
    PutLong(dfDriverInfo);
    PutLong(dfReserved);



    PutDeviceName("PostScript");
    PutFaceName(iFont);


    dfOriginTable = NULL;

    if (!fPass2)
	{
	dfSize = cbBuffer;
	MakeDirEntry(TRUE, iFont);
	}
    }


int OpenDir(iDir, cFonts)
int iDir;
int cFonts;
    {
    char szFile[80];
    int fh;

    sprintf(szFile, "fdir%02d.bin", iDir);

    fh = open(szFile, O_CREAT | O_TRUNC | O_RDWR, S_IREAD | S_IWRITE);
    if (fh<=0)
	{
	printf("Can't create: %s\n", szFile);
	exit(1);
	}
    close(fh);
    fh = open(szFile, O_RDWR | O_BINARY | O_TRUNC);
    ResetBuffer();
    PutWord(cFonts);
    WriteBuffer(fh);

    return(fh);
    }


CloseDir(fh)
int fh;
    {
    if (fh>0)
	close(fh);
    }



WriteBuffer(fh)
int fh;
    {
    if ((cbBuffer > 0) && (fh >= 0))
	write(fh, rgbBuffer, cbBuffer);
    }




#endif




/*******************************************************************
* Name: PutPairKernTable()
*
* Action: Send the pairwise kerning table to the output file.
*
********************************************************************
*/
PutPairKernTable(iFont)
int iFont;
{
    int i;

    if (afm.kp.cPairs > 0){
		dfPairKernTable = cbBuffer;
		DBMSG(("Pair Kern Table:\n# pairs=%d\n",afm.kp.cPairs));
		PutWord(afm.kp.cPairs);
		for (i=0; i<afm.kp.cPairs; ++i){
		    PutWord(afm.kp.rgPairs[i].iKey);
		    PutWord(afm.kp.rgPairs[i].iKernAmount);
		    DBMSG(("key=%x, ",afm.kp.rgPairs[i].iKey));
			DBMSG(("kern amount=%d\n",afm.kp.rgPairs[i].iKernAmount));
		}
	}else dfPairKernTable = NULL;
}






PutOriginTable(iFont)
int iFont;
    {
    dfOriginTable = 0L;
    }





/******************************************************************
* Name: PutTrackKernTable()
*
* Action: Send the track kerning table to the output file.
*
*******************************************************************
*/
PutTrackKernTable()
    {
    int i;

    if (afm.kt.cTracks == 0)
	{
	dfTrackKernTable = NULL;
	return;
	}


    dfTrackKernTable = cbBuffer;
    PutWord(afm.kt.cTracks);
    for (i=0; i<afm.kt.cTracks; ++i)
	{
	PutWord(afm.kt.rgTracks[i].iDegree);
	PutWord(afm.kt.rgTracks[i].iPtMin);
	PutWord(afm.kt.rgTracks[i].iKernMin);
	PutWord(afm.kt.rgTracks[i].iPtMax);
	PutWord(afm.kt.rgTracks[i].iKernMax);
	}
    }





/***************************************************************
* Name: PutExtentTable()
*
* Action: Send the character extent information to the output file.
*
****************************************************************
*/
PutExtentTable()
    {
    int i;

    if (dfPitchAndFamily & 1)
	{
	dfExtentTable = cbBuffer;
	for (i=afm.iFirstChar; i<=afm.iLastChar; ++i)
	    PutWord(afm.rgcm[i].iWidth);
	}
    else
	dfExtentTable = (long) NULL;

    }


/***********************************************************
* Name: WriteBuffer()
*
* Action: Flush the ouput buffer to the file.  Note that this
*	  function is only called after the entire pfm structure
*	  has been built in the output buffer.
*
************************************************************
*/
WritePfm()
    {
    char *pbSrc;
    char *pbDst;
    char szFile[80];
    int fh;

    pbSrc = afm.szFile;
    pbDst = szFile;
    while (*pbSrc!=0 && *pbSrc!='.')
	*pbDst++ = *pbSrc++;
    *pbDst++ = '.';
    *pbDst++ = 'p';
    *pbDst++ = 'f';
    *pbDst++ = 'm';
    *pbDst = 0;


    if ((fh = open(szFile, O_CREAT | O_TRUNC | O_RDWR, S_IREAD | S_IWRITE))<=0)
	{
	printf("Can't create: %s\n", szFile);
	exit(1);
	}
    close(fh);
    fh = open(szFile, O_RDWR | O_BINARY | O_TRUNC);
    if ((cbBuffer > 0) && (fh >= 0))
	write(fh, rgbBuffer, cbBuffer);
    close(fh);
    }
