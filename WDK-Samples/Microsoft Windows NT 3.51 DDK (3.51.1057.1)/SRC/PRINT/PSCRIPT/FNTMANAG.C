//--------------------------------------------------------------------------
//
// Module Name:  FNTMANAG.C
//
// Brief Description:  This module contains the PSCRIPT driver's
// DrvFontManagement function and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 07-May-1993
//
// Copyright (c) 1993 Microsoft Corporation
//--------------------------------------------------------------------------

#include "stdlib.h"
#include <string.h>
#include "pscript.h"
#include "enable.h"
#include "winbase.h"
#include "afm.h"

// declarations of external routines.

extern LONG iHipot(LONG, LONG);
extern void PSfindfontname(PDEVDATA, FONTOBJ*, XFORMOBJ*, WCHAR*, char*);

// declarations of routines residing within this module.

BOOL ForceLoadFont(PDEVDATA, FONTOBJ *, DWORD, HGLYPH *);
BOOL GrabFaceName(PDEVDATA, FONTOBJ *, CHAR *, DWORD);
PS_FIX GetPointSize(PDEVDATA, FONTOBJ *, XFORM *);

INT iExtractEncoding(PSZ pBuffer);



/******************************Public*Routine******************************\
*
* DrvQueryEXTTEXTMETRIC, support for this escape
*
* History:
*  20-May-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL  DrvQueryEXTTEXTMETRIC (PDEVDATA  pdev, ULONG iFace, EXTTEXTMETRIC *petm)
{
// make sure iFace is valid.

    if ((iFace == 0) || (iFace > (pdev->cDeviceFonts + pdev->cSoftFonts)))
    {
	SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

// copy the data out

    *petm = pdev->pfmtable[iFace - 1].pntfm->etm;
    return TRUE;
}




//--------------------------------------------------------------------------
// BOOL DrvFontManagement(pfo, iType, pvIn, cjIn, pvOut, cjOut)
// FONTOBJ    *pfo;
// DWORD       iType;
// PVOID       pvIn;
// DWORD       cjIn;
// PVOID       pvOut;
// DWORD       cjOut;
//
// This routine handles multiple font management related functions,
// depending on iType.
//
// Parameters:
//   pdev
//     Pointer to our DEVDATA structure.
//
// Returns:
//   This routine returns no value.
//
// History:
//   26-Apr-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

ULONG DrvFontManagement(pso, pfo, iType, cjIn, pvIn, cjOut, pvOut)
SURFOBJ    *pso;
FONTOBJ    *pfo;
DWORD       iType;
DWORD       cjIn;
PVOID       pvIn;
DWORD       cjOut;
PVOID       pvOut;
{
    PDEVDATA    pdev;

    // pso may be NULL if QUERYESCSUPPORT.

    if (iType != QUERYESCSUPPORT)
    {
        // get the pointer to our DEVDATA structure and make sure it is ours.

        pdev = (PDEVDATA) pso->dhpdev;

        if (bValidatePDEV(pdev) == FALSE)
        {
            RIP("PSCRIPT!DrvFontManagement: invalid pdev.\n");
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
    }

    // handle the different cases.

    switch (iType)
    {
        case QUERYESCSUPPORT:
            // when querying escape support, the function in question is
            // passed in the ULONG passed in pvIn.

            switch (*(PULONG)pvIn)
            {
                case QUERYESCSUPPORT:
                case DOWNLOADFACE:
                case GETFACENAME:
                case GETEXTENDEDTEXTMETRICS:

                return(1);

                default:
                    // return 0 if the escape in question is not supported.

		    return(0);
            }

        case DOWNLOADFACE:

            // call ForceLoadFont to do the work.

            return(ForceLoadFont(pdev, pfo, cjIn, (PHGLYPH)pvIn));

        case GETFACENAME:
            // call GrabFaceName to do the work.

            return(GrabFaceName(pdev, pfo, (CHAR *)pvOut, cjOut));

        case GETEXTENDEDTEXTMETRICS:

            return DrvQueryEXTTEXTMETRIC (
                       pdev,
                       pfo->iFace,
                       (EXTTEXTMETRIC *)pvOut
                       );

        default:
            return(FALSE);
    }
    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL ForceLoadFont(pdev, pfo)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
//
// This routine downloads the specified font to the printer, no
// questions asked.
//
// History:
//   07-May-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL ForceLoadFont(pdev, pfo, cjIn, phglyphs)
PDEVDATA    pdev;
FONTOBJ    *pfo;
DWORD       cjIn;
HGLYPH     *phglyphs;
{
    PNTFM       pntfm;
    BOOL        bDeviceFont;
    XFORM       fontxform;
    PS_FIX      psfxScaleFactor;
    ULONG       ulPointSize;
    DWORD       Type;

    // make sure we have our hglyph => ANSI translation table.
    // the table consists of 256 HGLYPHS, plus two WORDS at the
    // beginning.  The first WORD states whether to always download
    // the font, or just if it has not yet been done.  The second
    // WORD is simply padding for alignment.

    if (cjIn < (sizeof(HGLYPH) * 257))
    {
        RIP("PSCRIPT!ForceLoadFont: invalid cjIn.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    // get the point size, and fill in the font xform.

    psfxScaleFactor = GetPointSize(pdev, pfo, &fontxform);

    // is this a device font?

    bDeviceFont = (pfo->flFontType & DEVICE_FONTTYPE);

    // select the proper font name for the new font.  if this is a
    // device font, get the name from the NTFM structure.  if this
    // is a GDI font that we are caching, we will create a name for
    // it at the time we download it to the printer.

    if (bDeviceFont)
    {
        // get the font metrics for the specified font.

        pntfm = pdev->pfmtable[pfo->iFace - 1].pntfm;

// !!! NOTE NOTE the following assumption is invalid.  I need to look at the
// !!! first word of phglyph to decide whether to always download the font or
// !!! only download it if it has not yet been downloaded.

//!!! I am writing this with the assumption, that the application will worry
//!!! about printer memory.  In other words, I will just blindly download
//!!! a font when I am told to, and not worry about killing the printer.
//!!! Is this a valid assumption???

//!!! I am also assuming that I do not have to keep track of which fonts
//!!! have been downloaded.

        // if the font is a softfont, download it.

        if (pfo->iFace > pdev->cDeviceFonts)
        {

            // send the soft font to the output chanell,
            // convert pfb to ascii on the fly.

            if (!bDownloadSoftFont(pdev, pfo->iFace - pdev->cDeviceFonts - 1))
            {
                RIP("PSCRIPT!SelectFont: downloading of softfont failed.\n");
                return(FALSE);
            }


        }
    } else // must be a GDI font we will be caching.
        return DownloadFont(pdev, pfo, phglyphs, NULL, pfo->flFontType & TRUETYPE_FONTTYPE);


    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL GrabFaceName(pdev, pfo, pbuffer, cb)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// CHAR       *pbuffer;
// DWORD       cb;
//
// This routine returns the driver's internal facename (ie the name
// which is sent to the printer) to the caller.  pbuffer, is filled
// in with the face name, being sure to not write more than cb bytes
// to the buffer.
//
// History:
//   07-May-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL GrabFaceName(pdev, pfo, pbuffer, cb)
PDEVDATA    pdev;
FONTOBJ    *pfo;
CHAR       *pbuffer;
DWORD       cb;
{
    PNTFM       pntfm;
    BOOL        bDeviceFont;
    PIFIMETRICS pifi;
    PWSTR       pwstr;
    DWORD       cTmp;
    CHAR        szFaceName[MAX_STRING];
    PSZ         pszFaceName;
    XFORMOBJ   *pxo;
    POINTL      ptl;
    POINTFIX    ptfx;
    POINTPSFX   ptpsfx;
    PS_FIX      psfxPointSize;

    // get the Notional to Device transform.

    pxo = FONTOBJ_pxoGetXform(pfo);

    if (pxo == NULL)
    {
        RIP("PSCRIPT!GrabFaceName: pxo == NULL.\n");
        return(FALSE);
    }

    // get the font transform information.

    XFORMOBJ_iGetXform(pxo, &pdev->cgs.FontXform);

    // get the point size, and fill in the font xform.

    psfxPointSize = GetPointSize(pdev, pfo, &pdev->cgs.FontXform);

    // is this a device font?

    bDeviceFont = (pfo->flFontType & DEVICE_FONTTYPE);

    // select the proper font name for the new font.  if this is a
    // device font, get the name from the NTFM structure.  if this
    // is a GDI font that we are caching, we will create a name for
    // it at the time we download it to the printer.

    if (bDeviceFont)
    {
        // get the font metrics for the specified font.

        pntfm = pdev->pfmtable[pfo->iFace - 1].pntfm;

        // copy the font name to the buffer.

        strncpy(pbuffer, (char *)pntfm + pntfm->ntfmsz.loszFontName, cb);
    }
    else // must be a GDI font we will be caching.
    {
        if ( (pfo->flFontType & TRUETYPE_FONTTYPE) ||
             (pfo->flFontType & RASTER_FONTTYPE) )
        {
            // create the ASCII name for this font which will get used
            // to select this font in the printer.

            if (!(pifi = FONTOBJ_pifi(pfo)))
            {
                RIP("PSCRIPT!SelectFont: pifi failed.\n");
                return(FALSE);
            }

            pwstr = (PWSTR)((BYTE *)pifi + pifi->dpwszFaceName);
			PSfindfontname(pdev, pfo, pxo, pwstr, szFaceName);

            // copy to the output buffer.

            strncpy(pbuffer, szFaceName, cb);
        }
        else
        {
            RIP("PSCRIPT!GrabFaceName: invalid pfo->flFontType.\n");
            return(FALSE);
        }
    }

    return(TRUE);
}


//--------------------------------------------------------------------
// PS_FIX GetPointSize(pdev, pfo, pxform)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// XFORM      *pxform;
//
// This routine returns the point size for the specified font.
//
// History:
//   11-May-1993    -by-    Kent Settle     (kentse)
//  Broke out into a separate routine.
//--------------------------------------------------------------------

PS_FIX GetPointSize(pdev, pfo, pxform)
PDEVDATA    pdev;
FONTOBJ    *pfo;
XFORM      *pxform;
{
    XFORMOBJ   *pxo;
    ULONG       ulComplex;
    BOOL        bDeviceFont;
    POINTFIX    ptfx;
    POINTL      ptl;
    FIX         fxVector;
    IFIMETRICS *pifi;
    PS_FIX      psfxPointSize;

    // get the Notional to Device transform.  this is needed to
    // determine the point size.

    pxo = FONTOBJ_pxoGetXform(pfo);

    if (pxo == NULL)
    {
        RIP("PSCRIPT!GrabFaceName: pxo == NULL.\n");
        return((PS_FIX)-1);
    }

    ulComplex = XFORMOBJ_iGetXform(pxo, pxform);

    bDeviceFont = (pfo->flFontType & DEVICE_FONTTYPE);

    // determine the notional space point size of the new font.

    if (bDeviceFont)
    {
        // PSCRIPT font's em height is hardcoded to be 1000 (see quryfont.c).

        pdev->cgs.fwdEmHeight = ADOBE_FONT_UNITS;
    }
    else
    {
        // If its not a device font, we'll have to call back and ask.

        if (!(pifi = FONTOBJ_pifi(pfo)))
        {
            RIP("PSCRIPT!SelectFont: pifi failed.\n");
            return((PS_FIX)-1);
        }

        pdev->cgs.fwdEmHeight = pifi->fwdUnitsPerEm;
    }

    // apply the notional to device transform.

    ptl.x = 0;
    ptl.y = pdev->cgs.fwdEmHeight;

    XFORMOBJ_bApplyXform(pxo, XF_LTOFX, 1, &ptl, &ptfx);

    // now get the length of the vector.

    fxVector = iHipot(ptfx.x, ptfx.y);

    // make it a PS_FIX 24.8 number.

    fxVector <<= 4;

    psfxPointSize = (PS_FIX)(MulDiv(fxVector, PS_RESOLUTION,
                                   pdev->psdm.dm.dmPrintQuality));

    return(psfxPointSize);
}




/******************************Public*Routine******************************\
*
* BOOL bWritePFB
*
* adapted from KentSe's code
*
* History:
*  19-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bWritePFB(PDEVDATA pdev, CHAR * pPFB)
{
    CHAR       *pPFBTemp;
    DWORD       cbToWrite1, cbToWrite2, cbSegment;
    DWORD       cbPFA;
    DWORD       i;
    PFBHEADER   pfbheader;
    CHAR       *pSrc;
    CHAR       *pDest;
    CHAR       *pSave;



    cbPFA = 0;

    // The PFB file format is a sequence of segments, each of which has a
    // header part and a data part. The header format, defined in the
    // struct PFBHEADER below, consists of a one byte sanity check number
    // (128) then a one byte segment type and finally a four byte length
    // field for the data following data. The length field is stored in
    // the file with the least significant byte first.  read in each
    // PFBHEADER, then process the data following it until we are done.

    pPFBTemp = pPFB;

    while (TRUE)
    {
        // read in what should be a PFBHEADER.

        memcpy(&pfbheader, pPFBTemp, sizeof(PFBHEADER));

        // make sure we have the header.

        if (pfbheader.jCheck != CHECK_BYTE)
        {
            RIP("PSCRPTUI!PFBToPFA: PFB Header not found.\n");
            SetLastError(ERROR_INVALID_DATA);
            return(FALSE);
        }

        // if we have hit the end of the .PFB file, then we are done.

        if (pfbheader.jType == EOF_TYPE)
            break;

        // get the length of the data in this segment.

        cbSegment = ((DWORD)pfbheader.ushilength << 16) + pfbheader.uslolength;

        // get a pointer to the data itself for this segment.

        pSrc = pPFBTemp + sizeof(PFBHEADER);

        // create a buffer to do the conversion into.

        if (!(pDest = (CHAR *)LocalAlloc(LPTR, cbSegment * 3)))
        {
            RIP("PSCRPTUI!PFBToPFA: LocalAlloc for pDest failed.\n");
            return(FALSE);
        }

        // save the pointer for later use.

        pSave = pDest;

        if (pfbheader.jType == ASCII_TYPE)
        {
            // read in an ASCII block, convert CR's to CR/LF's and
            // write out to the .PFA file.

            cbToWrite2 = cbSegment;      // total count of bytes written to buffer.

            for (i = 0; i < cbSegment; i++)
            {
                if (0x0D == (*pDest++ = *pSrc++))
                {
                    *pDest++ = (BYTE)0x0A;
                    cbToWrite2++;
                }
            }
        }
        else if (pfbheader.jType == BINARY_TYPE)
        {
            // read in a BINARY block, convert it to HEX and write
            // out to the .PFA file.

            cbToWrite2 = cbSegment * 2;  // total count of bytes written to buffer.

            for (i = 0; i < cbSegment; i++)
            {
                *pDest++ = BinaryToHex((*pSrc >> 4) & 0x0F);
                *pDest++ = BinaryToHex(*pSrc & 0x0F);
                pSrc++;

                // output a CR/LF ever 64 bytes for readability.

                if ((i % 32) == 31)
                {
                    *pDest++ = (BYTE)0x0D;
                    *pDest++ = (BYTE)0x0A;
                    cbToWrite2 += 2;
                }
            }

            // add a final CR/LF if non 64 byte boundary.

            if ((cbSegment % 32) != 31)
            {
                *pDest++ = (BYTE)0x0D;
                *pDest++ = (BYTE)0x0A;
                cbToWrite2 += 2;
            }
        }
        else
        {
            RIP("PSCRPTUI!PFBToPFA: PFB Header type invalid.\n");
            SetLastError(ERROR_INVALID_DATA);
            LocalFree((LOCALHANDLE)pDest);
            return(FALSE);
        }

        // reset pointer to start of buffer.

        pDest = pSave;

        // write the buffer to the .PFA file.

        if (!bPSWrite(pdev, (PVOID)pDest, (DWORD)cbToWrite2))
        {
            RIP("PSCRPTUI!PFBToPFA: bPSWrite block to .PFA file failed.\n");
            LocalFree((LOCALHANDLE)pDest);
            return(FALSE);
        }

        // update the counter of BYTES written out to the file.

        cbPFA += cbToWrite2;

        // point to the next PFBHEADER.

        pPFBTemp += cbSegment + sizeof(PFBHEADER);

        // free up memory.

        LocalFree((LOCALHANDLE)pDest);
    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL PFBToPFA(pwstrPFAFile, pwstrPFBFile)
// PWSTR     pwstrPFAFile;
// PWSTR     pwstrPFBFile;
//
// This function takes a pointer to a destination .PFA file and a source
// .PFB file, then creates the .PFA from the .PFB file.
//
// Returns:
//   This function returns TRUE if the .PFA is successfully created,
//   otherwise it returns FALSE.
//
// History:
//  Tue 19-Apr-1994 -by- Bodin Dresevic [BodinD]
// update: modified to load the file on the fly to the output chanell

//   07-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------



BOOL bDownloadSoftFont(PDEVDATA pdev, DWORD iSoftFace)
{
// get the full path of the pfb file for this font:

    CHAR       *pPFB;
    BOOL        bReturn;

// now that we have full path let us use it:

    if (!(pPFB = MapFile(pdev->psfnode->asfe[iSoftFace].pwcPFB)))
    {
    #if DBG
        DbgPrint("PSCRPTUI!bDownloadSoftFont MapFile failed.\n");
    #endif

        return(FALSE);
    }

// pfb file could go away during this operation, must put in a try/except:

    try
    {
        bReturn = bWritePFB(pdev, pPFB);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        bReturn = FALSE;
    }

    UnmapViewOfFile((PVOID)pPFB);
    return bReturn;
}




//--------------------------------------------------------------------------
// PSTR LocateKeyword(pBuffer, pstrKeyword)
// PSTR    pBuffer;
// PSTR    pstrKeyword;
//
// This function takes a pointer to a buffer, and a pointer to a null
// terminated string.  It searches the buffer for the string and returns
// a pointer to the string if it is found.
//
// Returns:
//   This function returns a pointer to the keyword if it is found,
//   otherwise it returns NULL.
//
// History:
//   03-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

PSTR LocateKeyword(pBuffer, pstrKeyword)
PSTR    pBuffer;
PSTR    pstrKeyword;
{
    size_t cchKeyword = strlen(pstrKeyword);

    while(*pBuffer != '\0')
    {
        // search through the buffer until we find the keyword designator '/'.

        while(*pBuffer != '/')
            pBuffer++;

        if (!(strncmp(pstrKeyword, pBuffer, cchKeyword)))
            break;      // we found it.

        // not this keyword, continue the search.

        pBuffer ++;
    }

    // we did not find the keyword.

    if (*pBuffer == '\0')
        pBuffer = NULL;

    // we did find it, return a pointer to the '/' character at the
    // beginning of the keyword.

    return(pBuffer);
}



/******************************Public*Routine******************************\
*
* INT iExtractEncoding(pBuffer, pszFontName)
*
* Effects: peek into .pfb file to figure out if a soft font has a
*          standard or custom encoding.
*
* History:
*  08-Nov-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

static char pszEncoding[] = "/Encoding";
static char pszStandardEncoding[] = "StandardEncoding";

INT iExtractEncoding(PSZ pBuffer)
{
    if (!(pBuffer = LocateKeyword(pBuffer, pszEncoding)))
    {
        RIP("bExtractEncoding: /Encoding can not be found.\n");
        SetLastError(ERROR_INVALID_DATA);
        return ENCODING_ERROR;
    }

    // if we got to this point, pBuffer will be pointing to
    // "/Encoding ....", skip the keyword "/Encoding"

    pBuffer += (sizeof(pszEncoding) - 1); // sizeof includes terminating zero

    // skip any white space.

    while (*pBuffer == ' ')
        pBuffer++;

    // pBuffer is now pointing to the first letter of the string describing
    // encoding vector. The font will have standard encoding
    // if and only if this string is StandardEncoding.

    if (!strncmp(pBuffer,
                 pszStandardEncoding,
                 sizeof(pszStandardEncoding) - 1))
    {
        return ENCODING_STANDARD;
    }
    else
    {
        return ENCODING_CUSTOM;
    }
}


/******************************Public*Routine******************************\
*
* INT iGetEncoding(PWSTR pwcPFB)
*
*
* Effects: given a full path to pfb file, extract encoding
*
* Warnings:
*
* History:
*  10-Nov-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



INT iGetEncoding(PWSTR pwcPFB)
{

    CHAR       *pPFB;
    INT         iReturn = ENCODING_ERROR;

    if (!(pPFB = MapFile(pwcPFB)))
    {
    #if DBG
        DbgPrint("PSCRIPT: bDoNotRemapSoftFont: MapFile failed.\n");
    #endif

        return ENCODING_ERROR;
    }

// pfb file could go away during this operation, must put in a try/except:

    try
    {
        iReturn = iExtractEncoding(pPFB);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        iReturn = ENCODING_ERROR;
    }

    UnmapViewOfFile((PVOID)pPFB);
    return iReturn;
}
