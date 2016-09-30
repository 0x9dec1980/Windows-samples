//--------------------------------------------------------------------------
//
// Module Name:  PPD.C
//
// Brief Description:  This module contains the PSCRIPT driver's PPD
// Compiler.
//
// Author:  Kent Settle (kentse)
// Created: 20-Mar-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
// This module contains routines which will take an Adobe PPD (printer
//--------------------------------------------------------------------------

#include "string.h"
#include "pscript.h"
#include "afm.h"

// Turn on PPD_DEBUG_FLAG to get extremely verbose
// debug output from the parser.

#define PPD_DEBUG_FLAG 0
#if PPD_DEBUG_FLAG
#define PPDDBG(arg) DbgPrint arg
#else
#define PPDDBG(arg)
#endif

#define MAX_PS_NAME     256

// declarations of routines residing within this module.

VOID GetWord(char *, int, PPARSEDATA);
VOID EatWhite(PPARSEDATA);
BOOL GetLine(PPARSEDATA);
int GetKeyword(TABLE_ENTRY *, PPARSEDATA);
int GetNumber(PPARSEDATA);
int GetFloat(int, PPARSEDATA);
int MapToken(char *, TABLE_ENTRY  *pTable);

int GetString(char *, PPARSEDATA);
int GetQuotedValue(char *, int, PPARSEDATA);
void GetDimension(PAPERDIM *, PPARSEDATA);
void GetImageableArea(RECTL *, PPARSEDATA);
int szLength(char *);
VOID BuildNTPD(PNTPD, PTMP_NTPD);
DWORD SizeNTPD(PNTPD, PTMP_NTPD);
VOID GetOptionString(PSTR, DWORD, PPARSEDATA);
int GetOptionIndex(TABLE_ENTRY *, PPARSEDATA);
BOOL szIsEqual(char *, char *);
BOOL GetBuffer(PPARSEDATA);
int NameComp(CHAR *, CHAR *);
VOID ParseProtocols(PPARSEDATA, PNTPD);

// external declarations.

extern TABLE_ENTRY KeywordTable[];
extern TABLE_ENTRY SecondKeyTable[];
extern TABLE_ENTRY FontTable[];

BOOL isterminator(char a)
{
        return ((!a) || (a == '/'));
}

/* Case sensitive string comparsion up to terminators '/' or '\0' */
int keycmp(CHAR *pname1, CHAR *pname2)
{
    for (; *pname1 == *pname2; ++pname1, ++pname2)
        if (isterminator(*pname1)) return 0;
         
    return isterminator(*pname1) && isterminator(*pname2) ?
            0 : *pname1 - *pname2;
}


void keycpyn(char *s, char *t, int n, BOOL dotranslate)
{
    int i;
    char* p;
    BOOL translated = FALSE;

    p = s;
    i = n;
    
    while (*t && i > 1) {

        if (*t == '/') {    
            if (!dotranslate) break;

            if (!translated) {
                translated = TRUE;
                p = s;      /* throw out parsed key */
                i = n;
            }
        } else {
            *p = *t;
            ++p;
            --i;
        }
        ++t;
    }

    /* Null out last char */
    *p = '\0';

}       

PSFORM* MatchFormToPPD(PNTPD pntpd, LPWSTR lpwname)
/* Return pointer to PSFORM struct if lpwname matches a ppd pagesize,
    else return NULL.
*/
{
    DWORD i;
    PSFORM *pPSForm;
    CHAR formname[CCHFORMNAME];

    WideCharToMultiByte(CP_ACP, 0, lpwname, -1, formname, CCHFORMNAME, NULL, NULL);
    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);
    for (i = 0; i < pntpd->cPSForms; i++) {
        if (!keycmp(formname, (CHAR *)pntpd + pPSForm->loFormName)) return pPSForm;
        pPSForm++;
    }
    return (PSFORM *) NULL;
}

//--------------------------------------------------------------------------
//
// VOID InitNTPD(pntpd);
// PNTPD    pntpd;
//
// Fills in the NTPD structure with initial values.
//
// Returns:
//   This routine returns no value.
//
// History:
//   22-Mar-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID InitNTPD(PNTPD lpppd_stub, PTMP_NTPD lpppd_raw)
{
    memset(lpppd_raw, 0, sizeof(TMP_NTPD));
    memset(lpppd_stub, 0, sizeof(NTPD));
    lpppd_stub->cjThis = sizeof(NTPD);
    lpppd_stub->ulVersion = (ULONG)NTPD_VERSION;
    lpppd_stub->iDefResolution = DEF_RESOLUTION;
    lpppd_stub->LangLevel = 1;
    lpppd_stub->wLandscapeOrientation = LSO_ANY;
    lpppd_stub->bJCLResolution = FALSE;
}


//--------------------------------------------------------------------------
//
// VOID ParsePPD;
//
// Parses the PPD file, building the TMP_NTPD structure as it goes.
//
// Returns:
//   This routine returns no value.
//
// History:
//  03/31/95 -davidx-
//      Parse additional keywords *JCLResolution and
//      *DefaultJCLResolution.
//
//  03/30/95 -davidx-
//      Pasre additional keyword *LandscapeOrientation.
//
//   03-Apr-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------
void ParsePPD(HANDLE hppdfile, PNTPD lpppd_stub, PTMP_NTPD lpppd_raw)
{
    int         iKeyword;
    int         i;
    int         j;
    char        szWord[256];
    PARSEDATA   bufs;
    PPARSEDATA  pbufs;


    bufs.hFile = hppdfile;
    bufs.cbBuffer = 0;
    bufs.fEOF = FALSE;
    bufs.fUnGetLine = FALSE;

    pbufs = &bufs;

    while (TRUE)
    {
        // get the next line from the PPD file.

        if (GetLine(pbufs)) {

            PPDDBG(("Normal End of File.\n"));
            break;
        }

        // get the next Keyword from the PPD file.

        iKeyword = GetKeyword(KeywordTable, pbufs);

        // we are done if end of file.

        if (iKeyword == TK_EOF)
            break;

        // there will actually be a lot of Keywords we don't care
        // about.  for speed's sake, let's trap them here.

        if (iKeyword == TK_UNDEFINED)
            continue;

        switch (iKeyword) {
        case COLORDEVICE:

            GetWord(szWord, sizeof(szWord), pbufs);
            if (!(strncmp(szWord, "True", 4))) {

                PPDDBG(("Device is Color.\n"));
                lpppd_stub->flFlags |= COLOR_DEVICE;
            } else {

                PPDDBG(("Device is Black & White.\n"));
            }
            break;

        case VARIABLEPAPER:

            GetWord(szWord, sizeof(szWord), pbufs);
            if (!(strncmp(szWord, "True", 4))) {

                PPDDBG(("Device supports Variable Paper.\n"));
                lpppd_stub->flFlags |= VARIABLE_PAPER;
            } else {

                PPDDBG(("Device does not support Variable Paper.\n"));
            }
            break;

        case ENDOFFILE:

            GetWord(szWord, sizeof(szWord), pbufs);
            if (!(strncmp(szWord, "False", 5))) {

                PPDDBG(("Device does not want Ctrl-D.\n"));
                lpppd_stub->flFlags |= NO_ENDOFFILE;
            } else {

                PPDDBG(("Device does want Ctrl-D.\n"));
                lpppd_stub->flFlags &= ~NO_ENDOFFILE;
            }
            break;

        case DEFAULTMANUALFEED:

            GetWord(szWord, sizeof(szWord), pbufs);
            if (!(strncmp(szWord, "True", 4))) {

                PPDDBG(("Device defaults to Manual Feed.\n"));
                lpppd_stub->flFlags |= MANUALFEED_ON;
            } else {

                PPDDBG(("Device does not default to Manual Feed.\n"));
            }
            break;

        case PROTOCOLS:

            ParseProtocols(pbufs, lpppd_stub);
            break;

        case NICKNAME:

            lpppd_raw->cbPrinterName =
                GetString(lpppd_raw->szPrinterName, pbufs);

            // make room for UNICODE printer name.

            lpppd_raw->cbPrinterName *= 2;
            PPDDBG(("PrinterName = %s\n", lpppd_raw->szPrinterName));
            break;

        case PRTVM:

            // fill in the free virtual memory in kilobytes.

            i = GetNumber(pbufs);
            lpppd_stub->cbFreeVM = (i >> 10);
            PPDDBG(("FreeVM = %d KB.\n", lpppd_stub->cbFreeVM));
            break;

        case LANGUAGELEVEL:

            // fill in the language level.  default to level 1 if
            // level 2 is not specified.

            i = GetNumber(pbufs);

            if (i != 2)
                i = 1;

            lpppd_stub->LangLevel = (DWORD)i;
            PPDDBG(("LanguageLevel = %d.\n", lpppd_stub->LangLevel));
            break;

        case DEFAULTJCLRESOLUTION:
        case DEFAULTRESOLUTION:

            lpppd_stub->iDefResolution = (USHORT)GetNumber(pbufs);

            // Once we decide the printer uses PJL for setting
            // resolution, we'll stick to it even when we encounter
            // *DefaultResolution keyword later on in the PPD.

            if (iKeyword == DEFAULTJCLRESOLUTION)
                lpppd_stub->bJCLResolution = TRUE;
            break;

        case SETRESOLUTION:
        case RESOLUTION:
        case JCLRESOLUTION:

            {

            // Get the resolution.

            USHORT res = GetNumber(pbufs);

            // Find out if the same resolution has appeared
            // previously. This is not likely to happen in
            // a PPD. But we should handle it just in case.

            i = lpppd_stub->cResolutions;

            while (--i >= 0)
                if (lpppd_raw->siResolutions[i].usIndex == res)
                    break;

            if (i < 0) {

                // The resolution hasn't appeared before.
                // Add it as the last entry. Make sure there
                // aren't too many resolutions.

                i = lpppd_stub->cResolutions;

                if (i >= MAX_RESOLUTIONS) {

                    DBGPS(DBGPS_LEVEL_WARNING,
                        ("Too many resolutions.\n"));
                    break;
                }

                lpppd_stub->cResolutions ++;
            }

            // Save the resolution.

            lpppd_raw->siResolutions[i].usIndex = res;

            // Determine whether to use JCL for setting resolution

            lpppd_stub->bJCLResolution = (iKeyword == JCLRESOLUTION);

            // Get the string for setting the resolution.

            if (iKeyword == JCLRESOLUTION) {

                // *JCLResolution is followed by a QuotedValue.

                if (GetQuotedValue(
                        lpppd_raw->siResolutions[i].szString,
                        sizeof(lpppd_raw->siResolutions[i].szString),
                        pbufs) < 0)
                {
                    DBGPS(DBGPS_LEVEL_ERROR,
                        ("Invalid value following *JCLResolution.\n"));
                    lpppd_raw->siResolutions[i].szString[0] = '\0';
                }

            } else {

                GetString(lpppd_raw->siResolutions[i].szString, pbufs);

            }

            }
            break;

        case SCREENFREQ:

            // the screen frequency is stored within quotes.
            // advance to first quotation mark, then one character past.

            while (*(pbufs->szLine) != '"')
                pbufs->szLine++;
            pbufs->szLine++;

            lpppd_stub->iScreenFreq = (USHORT)GetFloat(10, pbufs);
            PPDDBG(("ScreenFrequency * 10 = %d\n", lpppd_stub->iScreenFreq));
            break;

        case SCREENANGLE:

            // the screen angle is stored within quotes.
            // advance to first quotation mark, then one character past.

            while (*(pbufs->szLine) != '"')
                pbufs->szLine++;
            pbufs->szLine++;

            lpppd_stub->iScreenAngle = (USHORT)GetFloat(10, pbufs);
            PPDDBG(("ScreenAngle * 10 = %d\n", lpppd_stub->iScreenAngle));
            break;

        case TRANSFER:

            // GetOptionIndex will get the string defining the type
            // of transfer function.  Normalized is the one we
            // care about.

            i = GetOptionIndex(SecondKeyTable, pbufs);
            if (i == NORMALIZED) {

                lpppd_raw->cbTransferNorm =
                    GetString(lpppd_raw->szTransferNorm, pbufs);

                PPDDBG(("TransferNormalized = %s\n",
                    lpppd_raw->szTransferNorm));
            } else if (i == NORM_INVERSE) {

                lpppd_raw->cbInvTransferNorm =
                    GetString(lpppd_raw->szInvTransferNorm, pbufs);

                PPDDBG(("InvTransferNormalized = %s\n",
                    lpppd_raw->szInvTransferNorm));
            }

            break;

        case DUPLEX:

            // GetOptionIndex will get the string defining the type
            // of duplex function.

            i = GetOptionIndex(SecondKeyTable, pbufs);

            switch (i) {

            case OPTION_FALSE:

                lpppd_raw->cbDuplexNone =
                    GetString(lpppd_raw->szDuplexNone, pbufs);

                PPDDBG(("szDuplexNone = %s.\n", lpppd_raw->szDuplexNone));
                break;

            case OPTION_TRUE:

                lpppd_raw->cbDuplexNoTumble =
                    GetString(lpppd_raw->szDuplexNoTumble, pbufs);

                PPDDBG(("szDuplexNoTumble = %s.\n",
                    lpppd_raw->szDuplexNoTumble));
                break;

            case OPTION_NONE:

                lpppd_raw->cbDuplexNone =
                    GetString(lpppd_raw->szDuplexNone, pbufs);

                PPDDBG(("szDuplexNone = %s.\n", lpppd_raw->szDuplexNone));
                break;

            case DUPLEX_TUMBLE:

                lpppd_raw->cbDuplexTumble =
                    GetString(lpppd_raw->szDuplexTumble, pbufs);

                PPDDBG(("szDuplexTumble = %s.\n", lpppd_raw->szDuplexTumble));
                break;

            case DUPLEX_NO_TUMBLE:

                lpppd_raw->cbDuplexNoTumble =
                    GetString(lpppd_raw->szDuplexNoTumble, pbufs);

                PPDDBG(("szDuplexNoTumble = %s.\n",
                    lpppd_raw->szDuplexNoTumble));
                break;
            }

            break;

        case COLLATE:

            // GetOptionIndex will get the string defining the type
            // of collate function.

            i = GetOptionIndex(SecondKeyTable, pbufs);
            if (i == OPTION_TRUE) {

                lpppd_raw->cbCollateOn =
                    GetString(lpppd_raw->szCollateOn, pbufs);

                PPDDBG(("CollateOn = %s\n", lpppd_raw->szCollateOn));
            } else if (i == OPTION_FALSE) {

                lpppd_raw->cbCollateOff =
                    GetString(lpppd_raw->szCollateOff, pbufs);

                PPDDBG(("CollateOff = %s\n", lpppd_raw->szCollateOff));
            }

            break;

        case DEFAULTPAGESIZE:

            // GetOptionIndex, will get the string defining the defaultpagesize,
            // and return the corresponding value from PaperTable.

            GetOptionString(
                lpppd_raw->szDefaultForm,
                sizeof(lpppd_raw->szDefaultForm),
                pbufs);

            PPDDBG(("DefaultForm = %s.\n", lpppd_raw->szDefaultForm));
            break;

        case PAGESIZE:

            // increment the paper size count.

            i = lpppd_stub->cPSForms;

            if (i >= MAX_PAPERSIZES) {

                DBGPS(DBGPS_LEVEL_WARNING, ("Too Many PaperSizes.\n"));
                break;
            }

            lpppd_stub->cPSForms++;

            // get the form name.

            GetOptionString(lpppd_raw->FormEntry[i].szName,
                        sizeof(lpppd_raw->FormEntry[i].szName), pbufs);

            // now get the form invocation string to send to the printer.

            GetString(lpppd_raw->FormEntry[i].szInvocation, pbufs);

            break;

        case PAGEREGION:

            // increment the page region count.

            i = lpppd_stub->cPageRegions;

            if (i >= MAX_PAPERSIZES) {

                DBGPS(DBGPS_LEVEL_WARNING, ("Too Many PageRegions.\n"));
                break;
            }
            lpppd_stub->cPageRegions++;

            // get the form name, and the invocation string to
            // set the page region.

            GetOptionString(lpppd_raw->PageRegion[i].szName,
                        sizeof(lpppd_raw->PageRegion[i].szName), pbufs);

            PPDDBG(("PageRegion %s.\n", lpppd_raw->PageRegion[i].szName));

            // now get the pageregion invocation string.

            GetString(lpppd_raw->PageRegion[i].szInvocation, pbufs);

            break;

        case IMAGEABLEAREA:

            // increment the imageablearea count.

            i = lpppd_raw->cImageableAreas;

            if (i >= MAX_PAPERSIZES) {

                DBGPS(DBGPS_LEVEL_WARNING, ("Too Many ImageableAreas.\n"));
                break;
            }

            lpppd_raw->cImageableAreas++;

            // get the form name.

            GetOptionString(lpppd_raw->ImageableArea[i].szForm,
                            sizeof(lpppd_raw->ImageableArea[i].szForm),
                            pbufs);

            // now get the rectangle of the imageablearea.

            GetImageableArea(&lpppd_raw->ImageableArea[i].rect, pbufs);

            PPDDBG(("ImageableArea %s = %d %d %d %d\n",
                lpppd_raw->ImageableArea[i].szForm,
                lpppd_raw->ImageableArea[i].rect.left,
                lpppd_raw->ImageableArea[i].rect.top,
                lpppd_raw->ImageableArea[i].rect.right,
                lpppd_raw->ImageableArea[i].rect.bottom));
            break;

        case PAPERDIMENSION:

            // increment the paperdimension count.

            i = lpppd_raw->cPaperDimensions;

            if (i >= MAX_PAPERSIZES) {

                DBGPS(DBGPS_LEVEL_WARNING, ("Too Many PaperDimensions.\n"));
                break;
            }

            lpppd_raw->cPaperDimensions++;

            GetOptionString(lpppd_raw->PaperDimension[i].szForm,
                            sizeof(lpppd_raw->PaperDimension[i].szForm),
                            pbufs);

            // now get the rectangle of the paper itself.

            GetDimension(&lpppd_raw->PaperDimension[i], pbufs);

            PPDDBG(("PaperDimension %s = %d %d\n",
                lpppd_raw->PaperDimension[i].szForm,
                lpppd_raw->PaperDimension[i].sizl.cx,
                lpppd_raw->PaperDimension[i].sizl.cy));
            break;

        case DEFAULTOUTPUTBIN:

            GetOptionString(lpppd_raw->szDefaultOutputBin,
                sizeof(lpppd_raw->szDefaultOutputBin), pbufs);

            PPDDBG(("DefaultOutputBin = %s\n", lpppd_raw->szDefaultOutputBin));
            break;


        case OUTPUTBIN:

            // increment the output bin count.

            i = lpppd_stub->cOutputBins;
            i++;
            if (i > MAX_BINS) {

                DBGPS(DBGPS_LEVEL_WARNING, ("Too Many OutputBins.\n"));
                break;
            }
            lpppd_stub->cOutputBins = (USHORT)i;

            i--;

            GetOptionString(lpppd_raw->siOutputBin[i].szName,
                sizeof(lpppd_raw->siOutputBin[i].szName), pbufs);

            PPDDBG(("OutputBin Name = %s\n", lpppd_raw->siOutputBin[i].szName));

            // now get the string to send to the printer to set the
            // outputbin.

            GetString(lpppd_raw->siOutputBin[i].szInvocation, pbufs);
            break;

        case DEFAULTINPUTSLOT:

            GetOptionString(lpppd_raw->szDefaultInputSlot,
                            sizeof(lpppd_raw->szDefaultInputSlot), pbufs);

            PPDDBG(("DefaultInputSlot = %s\n", lpppd_raw->szDefaultInputSlot));
            break;


        case INPUTSLOT:

            // increment the output bin count.

            i = lpppd_stub->cInputSlots;
            i++;
            if (i > MAX_BINS) {

                DBGPS(DBGPS_LEVEL_WARNING, ("Too Many InputSlots.\n"));
                break;
            }
            lpppd_stub->cInputSlots = (USHORT)i;

            i--;

            GetOptionString(lpppd_raw->siInputSlot[i].szName,
                            sizeof(lpppd_raw->siInputSlot[i].szName), pbufs);

            PPDDBG(("InputSlot Name = %s\n", lpppd_raw->siInputSlot[i].szName));

            // now get the string to send to the printer to set the
            // inputslot.

            GetString(lpppd_raw->siInputSlot[i].szInvocation, pbufs);
            break;

        case MANUALFEED:

            // GetOptionIndex will get the string defining the type
            // of ManualFeed function.

            i = GetOptionIndex(SecondKeyTable, pbufs);

            if (i == OPTION_TRUE) {

                // get and save the string to set manual feed to TRUE
                // for the given printer.

                lpppd_raw->cbManualTRUE =
                    GetString(lpppd_raw->szManualTRUE, pbufs);
            } else if (i == OPTION_FALSE) {

                // get and save the string to set manual feed to FALSE
                // for the given printer.

                lpppd_raw->cbManualFALSE =
                    GetString(lpppd_raw->szManualFALSE, pbufs);
            }

            break;

        case DEFAULTFONT:

            PPDDBG(("ParsePPD: *DefaultFont parsed\n"));
            break;

        case DEVICE_FONT:

            // get the index of the font.

            j = GetOptionIndex(FontTable, pbufs);

            if (j == TK_UNDEFINED) {

                PPDDBG(("ParsePPD: font not found.\n"));
                break;
            }

            // increment the font counter if the font was valid.

            i = lpppd_stub->cFonts;
            i++;

            if (i > MAX_FONTS) {

                DBGPS(DBGPS_LEVEL_WARNING, ("Too Many Fonts.\n"));
                break;
            }

            lpppd_stub->cFonts = (USHORT)i;

            // GetOptionIndex, will get the string defining the font,
            // and return the corresponding value from FontTable.

            i--;

            lpppd_raw->bFonts[i] = (BYTE)j;
            PPDDBG(("Font Value = %d\n", j));
            break;

        case LANDSCAPEORIENTATION:

            // Which way to rotate for landscape output

            GetWord(szWord, sizeof(szWord), pbufs);

            lpppd_stub->wLandscapeOrientation =
                (strncmp(szWord, "Plus90", 6) == 0) ?
                    LSO_PLUS90 :
                    (strncmp(szWord, "Minus90", 7) == 0) ?
                        LSO_MINUS90 :
                        LSO_ANY;

            break;

        default:
            break;
    }
}
}


//--------------------------------------------------------------------------
//
// VOID BuildNTPD(pntpd, ptmp);
// PNTPD        pntpd;
// PTMP_NTPD    ptmp;
//
// Fills in the NTPD structure with values derived from the TMP_NTPD
// structure.
//
// Returns:
//   This routine returns no value.
//
// History:
//   25-Mar-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID BuildNTPD(pntpd, ptmp)
PNTPD       pntpd;
PTMP_NTPD   ptmp;
{
    DWORD           i, j;
    BYTE           *pfont;
    PSRESOLUTION   *pRes;
    PSFORM         *pForm;
    PSOUTPUTBIN    *pBin;
    PSINPUTSLOT    *pSlot;

    // start by adding the printer name to the end of the NTPD structure.
    // the printer name is stored as a UNICODE string, so it needs to be
    // WCHAR aligned.

    PPDDBG(("Entering BuildNTPD: cjThis = %d.\n", pntpd->cjThis));

    pntpd->cjThis = WCHARALIGN(pntpd->cjThis);

    // make sure alignment is proper.

    ASSERTPS(((pntpd->cjThis % sizeof(WCHAR)) == 0),
        "pntpd->lowszPrinterName not properly aligned.\n");

    pntpd->lowszPrinterName = pntpd->cjThis;

    strcpy2WChar((PWSTR)((PSTR)pntpd + pntpd->lowszPrinterName),
                 ptmp->szPrinterName);

    PPDDBG(("PrinterName = %ws\n",
        (PWSTR) ((PSTR) pntpd + pntpd->lowszPrinterName)));

    // now add the set resolution strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cResolutions PSRESOLUTION
    // structures are stored at the end of the NTPD structure.  within
    // each PSRESOLUTION structure is an offset to the string corresponding
    // to the resolution in question.

    // the start of the PSRESOLUTION array must be DWORD aligned.

    pntpd->cjThis += ptmp->cbPrinterName;
    pntpd->cjThis = DWORDALIGN(pntpd->cjThis);

    // make sure alignment is proper.

    ASSERTPS(((pntpd->cjThis % sizeof(DWORD)) == 0),
             "pntpd->loResolution not properly aligned.\n");

    pntpd->loResolution = pntpd->cjThis;

    PPDDBG(("post cbPrinterName: cjThis = %d.\n", pntpd->cjThis));

    if (pntpd->cResolutions != 0) {

        // add the PSRESOLUTION array to the end of the NTPD structure.

        pntpd->cjThis += pntpd->cResolutions * sizeof(PSRESOLUTION);

        pRes = (PSRESOLUTION *)((CHAR *)pntpd + pntpd->loResolution);

        // for each resolution, fill in the PSRESOLUTION structure.
        // then add the string itself to the end of the NTPD structure.

        for (i = 0; i < (DWORD)pntpd->cResolutions; i++)
        {
            pRes[i].iValue = (DWORD)ptmp->siResolutions[i].usIndex;
            pRes[i].loInvocation = pntpd->cjThis;

            j = szLength(ptmp->siResolutions[i].szString);
            memcpy((CHAR *)pntpd + pntpd->cjThis,
                   ptmp->siResolutions[i].szString, j);
            pntpd->cjThis += j;
        }
    }

    PPDDBG(("post resolutions: cjThis = %d.\n", pntpd->cjThis));

    // add the transfernormalized string to the end of the structure.
    // check to make sure we have a string to copy.

    i = (USHORT)ptmp->cbTransferNorm;

    if (i != 0) {

        pntpd->loszTransferNorm = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((CHAR *)pntpd + pntpd->loszTransferNorm,
               ptmp->szTransferNorm, i);
    }

    PPDDBG(("post cbTransferNorm: cjThis = %d.\n", pntpd->cjThis));

    if (i != 0) {

        PPDDBG(("Transfer Normalized Not Found.\n"));
    } else {

        PPDDBG(("Transfer Normalized = %s\n", ptmp->szTransferNorm));
    }

    // add the inverse transfernormalized string to the end of the structure.
    // check to make sure we have a string to copy.

    if (i = ptmp->cbInvTransferNorm) {

        pntpd->loszInvTransferNorm = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((CHAR *)pntpd + pntpd->loszInvTransferNorm,
               ptmp->szInvTransferNorm, i);
    }

    PPDDBG(("post cbInvTransferNorm: cjThis = %d.\n", pntpd->cjThis));
    if (i != 0) {

        PPDDBG(("Inverse Transfer Normalized Not Found.\n"));
    } else {

        PPDDBG(("Inverse Transfer Normalized = %s\n", ptmp->szTransferNorm));
    }

    // fill in the default form name.

    pntpd->loDefaultForm = pntpd->cjThis;
    strcpy((CHAR *)pntpd + pntpd->loDefaultForm, ptmp->szDefaultForm);
    pntpd->cjThis += szLength(ptmp->szDefaultForm);

    // add the form names list to the end of the structure.

    pntpd->cjThis = DWORDALIGN(pntpd->cjThis);

    PPDDBG(("post szDefaultForm: cjThis = %d.\n", pntpd->cjThis));

    // make sure alignment is proper.

    ASSERTPS(((pntpd->cjThis % sizeof(DWORD)) == 0),
             "pntpd->loPSFORMArray not properly aligned.\n");

    pntpd->loPSFORMArray = pntpd->cjThis;
    pntpd->cjThis += pntpd->cPSForms * sizeof(PSFORM);

    PPDDBG(("post cPSForms: cjThis = %d.\n", pntpd->cjThis));

    pForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

    // for each form, fill in the offset to the form name string, the
    // offset to the invocation string, then the strings themselves.

    for (i = 0; i < pntpd->cPSForms; i++) {

        pForm[i].loFormName = pntpd->cjThis;
        strcpy((CHAR *)pntpd + pntpd->cjThis, ptmp->FormEntry[i].szName);
        pntpd->cjThis += szLength(ptmp->FormEntry[i].szName);

        pForm[i].loSizeInvo = pntpd->cjThis;
        strcpy((CHAR *)pntpd + pntpd->cjThis, ptmp->FormEntry[i].szInvocation);
        pntpd->cjThis += szLength(ptmp->FormEntry[i].szInvocation);

        PPDDBG(("FormEntry %s = %s.\n", ptmp->FormEntry[i].szName,
            ptmp->FormEntry[i].szInvocation));
    }

    PPDDBG(("post all forms: cjThis = %d.\n", pntpd->cjThis));

    // fill in the page region information. an offset to each string is set
    // in the corresponding PSFORM struct. then add the string itself to the
    // end of the NTPD structure.

    for (i = 0; i < pntpd->cPageRegions; i++)
    {
        for (j = 0; j < pntpd->cPSForms; j++)
        {
            if (!(NameComp((CHAR *)pntpd + pForm[j].loFormName,
                         ptmp->PageRegion[i].szName)))
            {
                pForm[j].loRegionInvo = pntpd->cjThis;
                strcpy((CHAR *)pntpd + pntpd->cjThis,
                       ptmp->PageRegion[i].szInvocation);
                pntpd->cjThis += szLength(ptmp->PageRegion[i].szInvocation);

                PPDDBG(("PageRegion[%d] = %s\n",
                    i, ptmp->PageRegion[i].szName));
            }
        }
    }

    PPDDBG(("post page regions: cjThis = %d.\n", pntpd->cjThis));

    // fill in the imageablearea information.  an RECTL for each form is
    // included within the PSFORM struct.

    for (i = 0; i < ptmp->cImageableAreas; i++)
    {
        for (j = 0; j < pntpd->cPSForms; j++)
        {
            if (!(NameComp((CHAR *)pntpd + pForm[j].loFormName,
                         ptmp->ImageableArea[i].szForm)))
            {
                pForm[j].imagearea = ptmp->ImageableArea[i].rect;

                PPDDBG(("ImageableArea %s = %d %d %d %d.\n",
                    ptmp->PageRegion[i].szName,
                    pForm[j].imagearea.left, pForm[j].imagearea.top,
                    pForm[j].imagearea.right, pForm[j].imagearea.bottom));
            }
        }
    }

    PPDDBG(("post imageableareas: cjThis = %d.\n", pntpd->cjThis));

    // fill in the paper dimension information.  a SIZEL for each form is
    // included within the PSFORM struct.

    for (i = 0; i < ptmp->cPaperDimensions; i++)
    {
        for (j = 0; j < pntpd->cPSForms; j++)
        {
            if (!(NameComp((CHAR *)pntpd + pForm[j].loFormName,
                         ptmp->PaperDimension[i].szForm)))
            {
                pForm[j].sizlPaper = ptmp->PaperDimension[i].sizl;

                PPDDBG(("PaperDimension %s = %d %d.\n",
                    ptmp->PaperDimension[i].szForm,
                    pForm[j].sizlPaper.cx, pForm[j].sizlPaper.cy));
            }
        }
    }

    PPDDBG(("post paper dimensions: cjThis = %d.\n", pntpd->cjThis));

    // now add the outputbin strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cOutputBins PSOUTPUTBIN
    // structures are stored at the end of the NTPD structure.  within
    // each PSOUTPUTBIN structure are offsets to the strings corresponding
    // to the output bin and invocation.  if there is only the default
    // outputbin defined, cOutputBins will be zero.  otherwise it is assumed
    // there will be at least two output bins defined.  if there is only one
    // defined, it will be the same as the default.

    pntpd->loDefaultBin = pntpd->cjThis;
    strcpy((CHAR *)pntpd + pntpd->loDefaultBin, ptmp->szDefaultOutputBin);
    pntpd->cjThis += szLength(ptmp->szDefaultOutputBin);
    pntpd->cjThis = DWORDALIGN(pntpd->cjThis);

    PPDDBG(("post szDefaultOutputBin: cjThis = %d.\n", pntpd->cjThis));

    if (pntpd->cOutputBins > 0) {

        // make sure alignment is proper.

        ASSERTPS(((pntpd->cjThis % sizeof(DWORD)) == 0),
             "pntpd->loPSOutputBins not properly aligned.\n");

        // add the PSOUTPUTBIN array to the end of the NTPD structure.

        pntpd->loPSOutputBins = pntpd->cjThis;

        pntpd->cjThis += pntpd->cOutputBins * sizeof(PSOUTPUTBIN);

        PPDDBG(("post cOutputBins: cjThis = %d.\n", pntpd->cjThis));

        pBin = (PSOUTPUTBIN *)((CHAR *)pntpd + pntpd->loPSOutputBins);

        // for each outputbin, fill in the PSOUTPUTBIN structure.
        // then add the string itself to the end of the NTPD structure.

        for (i = 0; i < pntpd->cOutputBins; i++) {

            // copy the output bin name and the invocation string.

            pBin[i].loBinName = pntpd->cjThis;
            strcpy((CHAR *)pntpd + pntpd->cjThis, ptmp->siOutputBin[i].szName);
            pntpd->cjThis += szLength(ptmp->siOutputBin[i].szName);

            pBin[i].loBinInvo = pntpd->cjThis;
            strcpy((CHAR *)pntpd + pntpd->cjThis,
                   ptmp->siOutputBin[i].szInvocation);
            pntpd->cjThis += szLength(ptmp->siOutputBin[i].szInvocation);

            PPDDBG(("OutputBin[%d] = %s\n", i,
                (CHAR *)pntpd + pBin[i].loBinName));
        }
    }

    PPDDBG(("post all output bins: cjThis = %d.\n", pntpd->cjThis));

    // now add the inputslot strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cInputSlots PSINPUTSLOT
    // structures are stored at the end of the NTPD structure.  within
    // each PSINPUTSLOT structure are offsets to the strings corresponding
    // to the slot name and invocation.  if there is only the default inputslot
    // defined, cInputSlots will be zero.  otherwise it is assumed there
    // will be at least two input slots defined.  if there is only one
    // defined, it will be the same as the default.

    pntpd->loDefaultSlot = pntpd->cjThis;
    strcpy((CHAR *)pntpd + pntpd->loDefaultSlot, ptmp->szDefaultInputSlot);
    pntpd->cjThis += szLength(ptmp->szDefaultInputSlot);
    pntpd->cjThis = DWORDALIGN(pntpd->cjThis);

    PPDDBG(("post szDefaultInputSlot: cjThis = %d.\n", pntpd->cjThis));

    if (pntpd->cInputSlots > 0) {

        // make sure alignment is proper.

        ASSERTPS(((pntpd->cjThis % sizeof(DWORD)) == 0),
             "pntpd->loPSInputSlots properly aligned.\n");

        // add the PSINPUTSLOT array to the end of the NTPD structure.

        pntpd->loPSInputSlots = pntpd->cjThis;

        pntpd->cjThis += pntpd->cInputSlots * sizeof(PSINPUTSLOT);

        PPDDBG(("post cInputSlots: cjThis = %d.\n", pntpd->cjThis));

        pSlot = (PSINPUTSLOT *)((CHAR *)pntpd + pntpd->loPSInputSlots);

        // for each inputslot, fill in the PSINPUTSLOT structure.
        // then add the string itself to the end of the NTPD structure.

        for (i = 0; i < pntpd->cInputSlots; i++)
        {
            // copy the input slot name and the invocation string.

            pSlot[i].loSlotName = pntpd->cjThis;
            strcpy((CHAR *)pntpd + pntpd->cjThis, ptmp->siInputSlot[i].szName);
            pntpd->cjThis += szLength(ptmp->siInputSlot[i].szName);

            pSlot[i].loSlotInvo = pntpd->cjThis;
            strcpy((CHAR *)pntpd + pntpd->cjThis,
                   ptmp->siInputSlot[i].szInvocation);
            pntpd->cjThis += szLength(ptmp->siInputSlot[i].szInvocation);

            PPDDBG(("InputSlot[%d] = %s\n", i,
                (CHAR *)pntpd + pSlot[i].loSlotName));
        }
    }

    PPDDBG(("post all input slots: cjThis = %d.\n", pntpd->cjThis));

    // add the manualfeedtrue string to the end of the structure.
    // check to make sure we found a string first.

    if (i = ptmp->cbManualTRUE)
    {
        pntpd->loszManualFeedTRUE = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((CHAR *)pntpd + pntpd->loszManualFeedTRUE,
               ptmp->szManualTRUE, i);
    }

    PPDDBG(("post cbManualTRUE: cjThis = %d.\n", pntpd->cjThis));

    if (i != 0) {
        
        PPDDBG(("ManualTRUE not found.\n"));
    } else {

        PPDDBG(("ManualTRUE = %s\n", ptmp->szManualTRUE));
    }

    // add the manualfeedfalse string to the end of the structure.
    // check to make sure we found a string first.

    if (i = ptmp->cbManualFALSE)
    {
        pntpd->loszManualFeedFALSE = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((CHAR *)pntpd + pntpd->loszManualFeedFALSE,
               ptmp->szManualFALSE, i);
    }

    PPDDBG(("post cbManualFALSE: cjThis = %d.\n", pntpd->cjThis));
    if (i != 0) {

        PPDDBG(("ManualFALSE not found.\n"));
    } else {

        PPDDBG(("ManualFALSE = %s\n", ptmp->szManualFALSE));
    }

    // add the duplex none string to the end of the structure.
    // check to make sure we found a string first.

    if (i = ptmp->cbDuplexNone)
    {
        pntpd->loszDuplexNone = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((PSTR)pntpd + pntpd->loszDuplexNone, ptmp->szDuplexNone, i);
    }

    PPDDBG(("post cbDuplexNone: cjThis = %d.\n", pntpd->cjThis));
    if (i != 0) {

        PPDDBG(("DuplexNone not found.\n"));
    } else {
        
        PPDDBG(("DuplexNone = %s\n", ptmp->szDuplexNone));
    }

    // add the duplex tumble string to the end of the structure.
    // check to make sure we found a string first.

    if (i = ptmp->cbDuplexTumble)
    {
        pntpd->loszDuplexTumble = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((PSTR)pntpd + pntpd->loszDuplexTumble, ptmp->szDuplexTumble, i);
    }

    PPDDBG(("post cbDuplexTumble: cjThis = %d.\n", pntpd->cjThis));
    if (i != 0) {

        PPDDBG(("DuplexTumble not found.\n"));
    } else {

        PPDDBG(("DuplexTumble = %s\n", ptmp->szDuplexTumble));
    }

    // add the duplex no tumble string to the end of the structure.
    // check to make sure we found a string first.

    if (i = ptmp->cbDuplexNoTumble)
    {
        pntpd->loszDuplexNoTumble = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((PSTR)pntpd + pntpd->loszDuplexNoTumble,
            ptmp->szDuplexNoTumble,
            i);
    }

    PPDDBG(("post cbDuplexNoTumble: cjThis = %d.\n", pntpd->cjThis));
    if (i != 0) {

        PPDDBG(("DuplexNoTumble not found.\n"));
    } else {

        PPDDBG(("DuplexNoTumble = %s\n", ptmp->szDuplexNoTumble));
    }

    // add the collate on string to the end of the structure.
    // check to make sure we found a string first.

    if (i = ptmp->cbCollateOn)
    {
        pntpd->loszCollateOn = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((PSTR)pntpd + pntpd->loszCollateOn, ptmp->szCollateOn, i);
    }

    PPDDBG(("post cbCollateOn: cjThis = %d.\n", pntpd->cjThis));
    if (i != 0) {

        PPDDBG(("CollateOn not found.\n"));
    } else {

        PPDDBG(("CollateOn = %s\n", ptmp->szCollateOn));
    }

    // add the collate off string to the end of the structure.
    // check to make sure we found a string first.

    if (i = ptmp->cbCollateOff)
    {
        pntpd->loszCollateOff = pntpd->cjThis;
        pntpd->cjThis += i;

        memcpy((PSTR)pntpd + pntpd->loszCollateOff, ptmp->szCollateOff, i);
    }

    PPDDBG(("post cbCollateOff: cjThis = %d.\n", pntpd->cjThis));
    if (i != 0) {

        PPDDBG(("CollateOff not found.\n"));
    } else {

        PPDDBG(("CollateOff = %s\n", ptmp->szCollateOff));
    }

    // now add the fonts to the end of the structure, an array of
    // pntpd->cFonts BYTES are stored at the end of the NTPD structure.

    // add the BYTE array to the end of the NTPD structure.

    pntpd->loFonts = pntpd->cjThis;
    pntpd->cjThis += pntpd->cFonts;

    PPDDBG(("post cFonts: cjThis = %d.\n", pntpd->cjThis));

    pfont = (BYTE *)pntpd + pntpd->loFonts;

    for (i = 0; i < (DWORD)pntpd->cFonts; i++) {

        pfont[i] = ptmp->bFonts[i];

        PPDDBG(("Font[%d] = %d\n", i, (int)pfont[i]));
    }
}


//--------------------------------------------------------------------------
// DWORD SizeNTPD(pntpd, ptmp)
// PNTPD       pntpd;
// PTMP_NTPD   ptmp;
//
// This routine determines the size of the NTPD structure for the
// given printer.
//
// Returns:
//   This routine returns the size of the NTPD structure in BYTES, or
// zero for error.
//
// History:
//   29-Sep-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

DWORD SizeNTPD(pntpd, ptmp)
PNTPD       pntpd;
PTMP_NTPD   ptmp;
{
    DWORD           i, dwSize;

    // start by adding the printer name to the end of the NTPD structure.

    PPDDBG(("Entering SizeNTPD.\n"));

    dwSize = sizeof(NTPD);
    dwSize = WCHARALIGN(dwSize);
    dwSize += ptmp->cbPrinterName;
    dwSize = DWORDALIGN(dwSize);

    PPDDBG(("post cbPrinterName: dwSize = %d.\n", dwSize));

    // now add the set resolution strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cResolutions PSRESOLUTION
    // structures are stored at the end of the NTPD structure.  within
    // each PSRESOLUTION structure is an offset to the string corresponding
    // to the resolution in question.

    if (pntpd->cResolutions != 0)
    {
        // add the PSRESOLUTION array to the end of the NTPD structure.

        dwSize += pntpd->cResolutions * sizeof(PSRESOLUTION);

        // for each resolution, fill in the PSRESOLUTION structure.
        // then add the string itself to the end of the NTPD structure.

        for (i = 0; i < (DWORD)pntpd->cResolutions; i++)
            dwSize += (DWORD)szLength(ptmp->siResolutions[i].szString);
    }

    PPDDBG(("post resolutions: dwSize = %d.\n", dwSize));

    // add the transfernormalized string to the end of the structure.

    dwSize += ptmp->cbTransferNorm;

    PPDDBG(("post cbTransferNorm: dwSize = %d.\n", dwSize));

    // add the inverse transfernormalized string to the end of the structure.

    dwSize += ptmp->cbInvTransferNorm;

    PPDDBG(("post cbInvTransferNorm: dwSize = %d.\n", dwSize));

    // add the form names list to the end of the structure.

    dwSize += szLength(ptmp->szDefaultForm);
    dwSize = DWORDALIGN(dwSize);

    PPDDBG(("post szDefaultForm: dwSize = %d.\n", dwSize));

    dwSize += pntpd->cPSForms * sizeof(PSFORM);

    PPDDBG(("post cPSForms: dwSize = %d.\n", dwSize));

    // for each form, allow room for the FormName, SizeInvo, and
    // RegionInvo strings.

    for (i = 0; i < pntpd->cPSForms; i++)
    {
        // account for the form name and invocation strings.

        dwSize += szLength(ptmp->FormEntry[i].szName) +
                  szLength(ptmp->FormEntry[i].szInvocation);

    }

    PPDDBG(("post all forms: dwSize = %d.\n", dwSize));

    // make room for the page region invocation strings.

    for (i = 0; i < pntpd->cPageRegions; i++)
        dwSize += szLength(ptmp->PageRegion[i].szInvocation);

    PPDDBG(("post page regions: dwSize = %d.\n", dwSize));

    // now add the outputbin strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cOutputBins PSOUTPUTBIN
    // structures are stored at the end of the NTPD structure.  within
    // each PSOUTPUTBIN structure are offsets to the strings corresponding
    // to the output bin and invocation.  if there is only the default
    // outputbin defined, cOutputBins will be zero.  otherwise it is assumed
    // there will be at least two output bins defined.  if there is only one
    // defined, it will be the same as the default.

    dwSize += szLength(ptmp->szDefaultOutputBin);
    dwSize = DWORDALIGN(dwSize);

    PPDDBG(("post szDefaultOutputBin: dwSize = %d.\n", dwSize));

    if (pntpd->cOutputBins > 0)
    {
        dwSize += pntpd->cOutputBins * sizeof(PSOUTPUTBIN);

        // for each outputbin, fill in the PSOUTPUTBIN structure.

        for (i = 0; i < pntpd->cOutputBins; i++)
        {
            dwSize += szLength(ptmp->siOutputBin[i].szName);
            dwSize += szLength(ptmp->siOutputBin[i].szInvocation);
        }
    }

    PPDDBG(("post all output bins: dwSize = %d.\n", dwSize));

    // now add the inputslot strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cInputSlots PSINPUTSLOT
    // structures are stored at the end of the NTPD structure.  within
    // each PSINPUTSLOT structure are offsets to the strings corresponding
    // to the slot name and the invocation string.  if there is only the
    // default inputslot defined, cInputSlots will be zero.  otherwise it
    // is assumed there will be at least two input slots defined.  if there
    // is only one defined, it will be the same as the default.

    dwSize += szLength(ptmp->szDefaultInputSlot);
    dwSize = DWORDALIGN(dwSize);

    PPDDBG(("post szDefaultInputSlot: dwSize = %d.\n", dwSize));

    if (pntpd->cInputSlots > 0)
    {
        dwSize += pntpd->cInputSlots * sizeof(PSINPUTSLOT);

        // for each inputslot, fill in the PSINPUTSLOT structure.

        for (i = 0; i < pntpd->cInputSlots; i++)
        {
            dwSize += szLength(ptmp->siInputSlot[i].szName);
            dwSize += szLength(ptmp->siInputSlot[i].szInvocation);
        }
    }

    PPDDBG(("post all input slots: dwSize = %d.\n", dwSize));

    // add the manualfeed strings to the end of the structure.

    dwSize += ptmp->cbManualTRUE + ptmp->cbManualFALSE;

    PPDDBG(("post manual strings: dwSize = %d.\n", dwSize));

    // add the duplex strings.

    dwSize += ptmp->cbDuplexNone + ptmp->cbDuplexTumble +
              ptmp->cbDuplexNoTumble;

    PPDDBG(("post duplex strings: dwSize = %d.\n", dwSize));

    // add the collate strings.

    dwSize += ptmp->cbCollateOn + ptmp->cbCollateOff;

    PPDDBG(("post collate strings: dwSize = %d.\n", dwSize));

    // now add the fonts to the end of the structure,
    // it is worth noting how these are stored in the NTPD structure.
    // an array of pntpd->cFonts BYTES are stored at the end of the NTPD
    // structure.

    // add the BYTE array to the end of the NTPD structure.

    dwSize += pntpd->cFonts;

    PPDDBG(("SizeNTPD - dwSize = %d.\n", dwSize));

    return(dwSize);
}


//--------------------------------------------------------------------------
//
// BOOL GetBuffer();
//
// This routines reads a new buffer full of text from the input file.
//
// Note: If the end of file is encountered in this function then
//     the program is aborted with an error message.    Normally
//     the program will stop processing the input when it sees
//     the end of information keyword.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns TRUE if end of file, FALSE otherwise.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

BOOL GetBuffer(pbufs)
PPARSEDATA  pbufs;
{
    // initialize the buffer count to zero.

    pbufs->cbBuffer = 0;

    // read in the next buffer full of data if we have not already hit the
    // end of file.

    if (!pbufs->fEOF)
    {
        ReadFile(pbufs->hFile, pbufs->rgbBuffer, sizeof(pbufs->rgbBuffer),
                 (LPDWORD)&pbufs->cbBuffer, (LPOVERLAPPED)NULL);

        if (pbufs->cbBuffer == 0)
            pbufs->fEOF = TRUE;
    }

    pbufs->pbBuffer = pbufs->rgbBuffer;
    return(pbufs->fEOF);
}


//--------------------------------------------------------------------------
//
// BOOL GetLine(pbufs);
// PPARSEDATA   pbufs;
//
// This routine gets the next line of text out of the input buffer.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns TRUE if end of file, FALSE otherwise.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

BOOL GetLine(PPARSEDATA  pbufs)
{
    int cbLine;
    char bCh;

    if (pbufs->fUnGetLine)
    {
        pbufs->szLine = pbufs->rgbLine;
        pbufs->fUnGetLine = FALSE;
        return(FALSE);
    }

    cbLine = 0;
    pbufs->szLine = pbufs->rgbLine;
    *(pbufs->szLine) = 0;

    if (!pbufs->fEOF)
    {
        while(TRUE)
        {
            if (pbufs->cbBuffer <= 0)
            {
                if (GetBuffer(pbufs))    // done if end of file hit.
                break;
            }

            while(--pbufs->cbBuffer >= 0)
            {
                bCh = *(pbufs->pbBuffer++);
                if (bCh == '\n' || bCh == '\r' || ++cbLine > sizeof(pbufs->rgbLine))
                {
                    *(pbufs->szLine) = 0;
                    pbufs->szLine = pbufs->rgbLine;
                    EatWhite(pbufs);
                    if (*(pbufs->szLine) != 0)
                    {
                        pbufs->szLine = pbufs->rgbLine;
                        return(pbufs->fEOF);
                    }

                    pbufs->szLine = pbufs->rgbLine;
                    cbLine = 0;
                    continue;
                }

                *(pbufs->szLine++) = bCh;
            }
        }
    }

    *(pbufs->szLine) = 0;

    pbufs->szLine = pbufs->rgbLine;
    return(pbufs->fEOF);
}


//--------------------------------------------------------------------------
//

// VOID UnGetLine(pbufs)
// PPARSEDATA  pbufs;
//
// This routine pushes the most recent line back into the input buffer.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns no value.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

VOID UnGetLine(pbufs)
PPARSEDATA  pbufs;
{
    pbufs->fUnGetLine = TRUE;
    pbufs->szLine = pbufs->rgbLine;
}


//--------------------------------------------------------------------------
//
// int GetKeyword(pTable, pbufs)
// TABLE_ENTRY    *pTable;
// PPARSEDATA      pbufs;
//
// Get the next token from the input stream.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns integer value of next token.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

int GetKeyword(pTable, pbufs)
TABLE_ENTRY    *pTable;
PPARSEDATA      pbufs;
{
    char szWord[256];

    if (*(pbufs->szLine) == 0)
        if (GetLine(pbufs))
            return(TK_EOF);

    GetWord(szWord, sizeof(szWord), pbufs);
    return(MapToken(szWord, pTable));
}


//--------------------------------------------------------------------------
// VOID GetOptionString(pstrOptionName, cbBuffer, pbufs)
// PSTR        pstrOptionName;
// DWORD       cbBuffer;
// PPARSEDATA  pbufs;
//
// This routine fills in the option name of the next option.
//
// Parameters:
//   pstrOptionName - place to put option name.
//
//   cbBuffer - size of buffer.
//
// Returns:
//   This routine returns no value.
//
// History:
//   08-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID GetOptionString(pstrOptionName, cbBuffer, pbufs)
PSTR        pstrOptionName;
DWORD       cbBuffer;
PPARSEDATA  pbufs;
{
    if (*(pbufs->szLine) == 0)
        if (GetLine(pbufs))
            return;

    EatWhite(pbufs);

    // copy the form name until the ':' deliminator is encountered.

    while (cbBuffer--)
    {
        *pstrOptionName = *(pbufs->szLine++);

        if ((*pstrOptionName == ':') || (*pstrOptionName == '\0'))
        {
            *pstrOptionName = '\0';  // add the zero terminator.
            break;
        }

        pstrOptionName++;
    }

    // strip off any trailing spaces, 'cause some people just can't
    // follow the spec.

    pstrOptionName--;

    if ((*pstrOptionName == ' ') || (*pstrOptionName == '\t'))
    {
        while ((*pstrOptionName == ' ') || (*pstrOptionName == '\t'))
            *pstrOptionName-- = '\0';
    }

    return;
}


//--------------------------------------------------------------------------
// int GetOptionIndex(pTable, pbufs)
// TABLE_ENTRY    *pTable;
// PPARSEDATA      pbufs;
//
// Get the next token from the input stream.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns integer value of next token.
//
// History:
//   08-Apr-1991    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

int GetOptionIndex(pTable, pbufs)
TABLE_ENTRY    *pTable;
PPARSEDATA      pbufs;
{
    char    szWord[PPD_LINEBUF_SIZE];
    int     i;
    char    ch;

    if (*(pbufs->szLine) == 0)
        if (GetLine(pbufs)) return(TK_EOF);

    EatWhite(pbufs);

    for (i = 0 ; i < PPD_LINEBUF_SIZE ; ++i) {

    // search to the end of the option.  this could be either the
    // colon, which ends the option, or the slash, which begins the
    // translation string (which we will ignore).

        ch = *(pbufs->szLine++);
        if (ch == ':' || ch == '/' || ch == 0) {
            szWord[i] = 0;
            break;
        }
        szWord[i] = ch;

    }

    return szWord[0] ? MapToken(szWord, pTable) : TK_UNDEFINED;
}


//--------------------------------------------------------------------------
//
// int MapToken(szWord, pTable)
// char           *szWord;        // Ptr to the ascii keyword string
// TABLE_ENTRY    *pTable;
//
// This routine maps an ascii key word into an integer token.
//
// Parameters:
//   szWord
//     Pointer to the ascii keyword string.
//
// Returns:
//   This routine returns int identifying token.
//
// History:
//   03-Apr-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

int MapToken(szWord, pTable)
char           *szWord;        // Ptr to the ascii keyword string
TABLE_ENTRY    *pTable;
{
    while (pTable->szStr)
    {
        if (szIsEqual(szWord, pTable->szStr))
            return(pTable->iValue);

        ++pTable;
    }

    PPDDBG(("MapToken could not map %s.\n", szWord));
    return(TK_UNDEFINED);
}


//--------------------------------------------------------------------------
// VOID GetWord(szWord, cbWord, pbufs)
// char       *szWord;        // Ptr to the destination area
// int         cbWord;        // The size of the destination area
// PPARSEDATA  pbufs;
//
// This routine gets the next word delimited by white space
// from the input buffer.
//
// Parameters:
//   szWord
//     Pointer to the destination area.
//
//   cbWord
//     Size of destination area.
//
// Returns:
//   This routine returns no value.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

VOID GetWord(szWord, cbWord, pbufs)
char       *szWord;        // Ptr to the destination area
int         cbWord;        // The size of the destination area
PPARSEDATA  pbufs;
{
    char bCh;

    EatWhite(pbufs);
    while (cbWord--)
    {
        switch(bCh = *(pbufs->szLine++))
        {
            case 0:
            case ' ':
            case '\t':
            case '\n':     // take care of newline and carriage returns.
            case '\r':
                --pbufs->szLine;
                *szWord = 0;
                return;
            case ':':       // the colon is a delimeter in PPD files,
                break;      // and should simply be skipped over.
            default:
                *szWord++ = bCh;
                break;
        }
    }

    *szWord = 0;
}


//--------------------------------------------------------------------------
// int GetString(szDst, pbufs)
// char       *szDst;
// PPARSEDATA  pbufs;
//
// This routine gets a " bracketed string from the ppd_file, attaching
// a zero terminator to it.
//
// Returns:
//   This routine returns the length of the string, including the zero
//   terminator.
//
// History:
//   03-Apr-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

// 4/25/95 -davidx-
//
// Temporary relief - make sure we don't overflow the string buffer.
// PPD parser will be rewritten for the next version of the driver.
//
// When an invocation is longer than MAX_PPD_STRING, we will simply
// discard it and replace it with an empty string. This is better
// than truncating the invocation string - it would cause a feature
// not to be invocated instead of generating PS errors.

int
PpdStringTooLong(
	char *	pBuffer
	)
{
	DBGPS(DBGPS_LEVEL_ERROR, ("PPD string too long!\n"));

	// Fill the string buffer with an empty string

	pBuffer[0] = ' ';
	pBuffer[1] = '\0';

	// Return the length of the string including the null terminator

	return 2;
}

int GetString(szDst, pbufs)
char       *szDst;
PPARSEDATA  pbufs;
{
    int     i;
	char *	pBuffer = szDst;

    // advance to the first quotation mark, then one character past it.

    while (*(pbufs->szLine) != '"')
        pbufs->szLine++;
    pbufs->szLine++;

    // initialize string length counter to include zero terminator.

    i = 1;

    // copy the string itself.  be sure to ignore ppd file comments (#).

    while (*(pbufs->szLine) && *(pbufs->szLine) != '"' &&
           *(pbufs->szLine) != '%')
    {
        if (++i > MAX_PPD_STRING)
			return PpdStringTooLong(pBuffer);
        *szDst++ = *(pbufs->szLine++);
    }

    // get the next line if the string is longer than one line.

    if (*(pbufs->szLine) != '"')
    {
        while (!(GetLine(pbufs)))
        {
            while (*(pbufs->szLine) && *(pbufs->szLine) != '"' &&
                   *(pbufs->szLine) != '%')
            {
				if (++i > MAX_PPD_STRING)
					return PpdStringTooLong(pBuffer);
                *szDst++ = *(pbufs->szLine++);
            }

            if (*(pbufs->szLine) == '"')
                break;

            // how 'bout a new line.

			if (++i > MAX_PPD_STRING)
				return PpdStringTooLong(pBuffer);
            *szDst++ = '\n';
        }
    }

    // add the zero terminator.

    *szDst = 0;

    // return the length of the string.

    return (i);
}


int
GetQuotedValue(
    char*       szDst,
    int         len,
    PPARSEDATA  pbufs)

/*++

Routine Description:

    Parse a quoted value from the PPD file.

Arguments:

    szDst   buffer to store the parsed value
    len     size of the buffer (including null terminator)
    pbufs   pointer to parser data

Return Value:

    number of characters in the quoted value
    -1 if an error occurs

History:

    04/01/95 -davidx-
        Wrote it.

--*/

#define GQV_HEXMODE_NONE        0
#define GQV_HEXMODE_FIRSTCHAR   0x100
#define GQV_HEXMODE_SECONDCHAR  0x200

{
    // hexMode can be in one of three states:
    // 0 - not in hexdecimal mode
    // 0x100 - hexdecimal mode, expecting first digit
    // 0x200 + first digit - hexdecimal mode, expecting second digit

    int     hexMode = GQV_HEXMODE_NONE;
    char    *szPtr = szDst;

    // Skip past the opening quote character

    while (*pbufs->szLine != '"')
        if (*pbufs->szLine++ == '\0')
            return -1;
    pbufs->szLine++;

    for ( ; ; ) {

        // Get the next character

        char ch = *pbufs->szLine++;

        // We're done if the closing quote is seen.
        // It's an error if we're in hex mode.

        if (ch == '"') {
            if (hexMode != GQV_HEXMODE_NONE)
                return -1;
            break;
        }

        // Check for end-of-line conditions.
        // It's an error if we encounter EOF.

        if (ch == '\0') {
            if (GetLine(pbufs))
                return -1;
            continue;
        }

        if (hexMode != GQV_HEXMODE_NONE) {

            // Leave hex mode

            if (ch == '>') {

                // Odd number of digits is an error

                if (hexMode >= GQV_HEXMODE_SECONDCHAR)
                    return -1;

                hexMode = GQV_HEXMODE_NONE;

            } else {

                int digit;

                // Ignore white space and newlines

                if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
                    continue;

                // Convert character to hex digit

                if (ch >= '0' && ch <= '9')
                    digit = ch - '0';
                else if (ch >= 'A' && ch <= 'F')
                    digit = ch - 'A' + 10;
                else if (ch >= 'a' && ch <= 'f')
                    digit = ch - 'a' + 10;
				else
					// Return error on invalid hexdecimal digit
					return -1;

                // Are we on the first or second digit?

                if (hexMode >= GQV_HEXMODE_SECONDCHAR) {

                    // Are we exceeding length limit?

                    if (--len <= 0)
                        return -1;

                    *szPtr++ = ((hexMode & 0xf) << 4) + digit;
                    hexMode = GQV_HEXMODE_FIRSTCHAR;

                } else
                    hexMode = GQV_HEXMODE_SECONDCHAR + digit;
            }

        } else {

            if (ch == '<') {

                // Enter hex mode

                hexMode = GQV_HEXMODE_FIRSTCHAR;

            } else {

                // Copy regular character directly into buffer.
                // Make sure we have enough space left.

                if (--len <= 0)
                    return -1;
                *szPtr++ = ch;
            }
        }
    }

    // Null-terminate the buffer and return
    // number of characters in it.

    *szPtr = '\0';
    return szPtr - szDst;
}


//--------------------------------------------------------------------------
// VOID EatWhite(pbufs)
// PPARSEDATA  pbufs;
//
// This routine moves the input buffer pointer forward to the
// next non-white character.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns TRUE if end of file, FALSE otherwise.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

VOID EatWhite(pbufs)
PPARSEDATA  pbufs;
{
    while (TRUE)
    {
        // skip to the next line if necessary.

        if ((*(pbufs->szLine) == '\0') || (*(pbufs->szLine) == '\n') ||
            (*(pbufs->szLine) == '\r'))
            GetLine(pbufs);

        // we are done if we hit a non-white character.

        if ((*(pbufs->szLine) != ' ') && (*(pbufs->szLine) != '\t'))
            break;

        pbufs->szLine++;
    }
}


//--------------------------------------------------------------------------
// int GetNumber(pbufs)
// PPARSEDATA  pbufs;
//
// This routine parses an ASCII decimal number from the
// input file stream and returns its value.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns integer value of ASCII decimal number.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

int GetNumber(pbufs)
PPARSEDATA  pbufs;
{
    int iVal;
    BOOL fNegative;

    fNegative = FALSE;

    iVal = 0;

    EatWhite(pbufs);

    // skip quotation mark if number is in quotes.

    if (*(pbufs->szLine) == '"')
    {
        pbufs->szLine++;
        EatWhite(pbufs);    // necessary if " 0 1".
    }

    if (*(pbufs->szLine) == '-')
    {
        fNegative = TRUE;
        ++(pbufs->szLine);
    }

    // handle the case where the value is '.2'.  make it zero.

    if (*(pbufs->szLine) == '.')
    {
        // skip all the fractional digits.

        while ((*(pbufs->szLine)) && (*(pbufs->szLine) != ' ') &&
               (*(pbufs->szLine) != '\t') && (*(pbufs->szLine) != '"'))
            pbufs->szLine++;

        return(0);
    }

    if (*(pbufs->szLine) < '0' || *(pbufs->szLine) > '9') {

        DBGPS(DBGPS_LEVEL_ERROR, ("PSCRIPT!GetNumber: invalid number.\n"));
        return(0);
    }

    while (*(pbufs->szLine) >= '0' && *(pbufs->szLine) <= '9')
        iVal = iVal * 10 + (*(pbufs->szLine++) - '0');

    // some .PPD files, which will not be mentioned do NOT follow
    // the Adobe spec, and put non-integer values where they
    // do not belong.  therefore, if we hit a non-integer value,
    // simply lop off the fraction.

    if (*(pbufs->szLine) == '.')
    {
    // just skip along until we hit some white space.

        while ((*(pbufs->szLine)) && (*(pbufs->szLine) != ' ') &&
               (*(pbufs->szLine) != '\t') && (*(pbufs->szLine) != '"'))
        pbufs->szLine++;
    }

    if (fNegative)
        iVal = - iVal;

    return(iVal);
}


//--------------------------------------------------------------------------
// int GetFloat(iScale, pbufs)
// int         iScale;        // The amount to scale the value by
// PPARSEDATA  pbufs;
//
// This routine parses an ASCII floating point decimal number from the
// input file stream and returns its value scaled by a specified amount.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns integer value of ASCII decimal number.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

int GetFloat(iScale, pbufs)
int         iScale;        // The amount to scale the value by
PPARSEDATA  pbufs;
{
    long lVal;
    long lDivisor;
    BOOL fNegative;

    EatWhite(pbufs);

    fNegative = FALSE;
    lVal = 0L;

    if (*(pbufs->szLine) == '-')
    {
        fNegative = TRUE;
        pbufs->szLine++;
    }

    if (*(pbufs->szLine) < '0' || *(pbufs->szLine) > '9') {

        DBGPS(DBGPS_LEVEL_ERROR, ("PSCRIPT!GetFloat: invalid number.\n"));
        return(0);
    }

    while (*(pbufs->szLine) >= '0' && *(pbufs->szLine) <= '9')
        lVal = lVal * 10 + (*(pbufs->szLine++) - '0');

    lDivisor = 1L;
    if (*(pbufs->szLine) == '.')
    {
        pbufs->szLine++;
        while (*(pbufs->szLine) >= '0' && *(pbufs->szLine) <= '9')
        {
            lVal = lVal * 10 + (*(pbufs->szLine++) - '0');
            lDivisor = lDivisor * 10;
        }
    }
    lVal = (lVal * iScale) / lDivisor;

    if (fNegative)
        lVal = - lVal;

    return((short)lVal);
}


//--------------------------------------------------------------------------
// void GetDimension(pdim, pbufs)
// PAPERDIM   *pdim;
// PPARSEDATA  pbufs;
//
// This routine extracts the paper dimension from the ppd file.
//
// Returns:
//   This routine returns no value.
//
// History:
//   03-Apr-1991    -by-    Kent Settle     (kentse)
//  Rewrote it.
//   25-Mar-1991    -by-    Kent Settle    (kentse)
//  Stole from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

void GetDimension(pdim, pbufs)
PAPERDIM   *pdim;
PPARSEDATA  pbufs;
{
    pdim->sizl.cx = GetNumber(pbufs);
    EatWhite(pbufs);
    pdim->sizl.cy = GetNumber(pbufs);
}


//--------------------------------------------------------------------------
// void GetImageableArea(prect, pbufs)
// RECTL      *prect;
// PPARSEDATA  pbufs;
//
// This routine extracts the imageable area from the ppd file.
//
// Returns:
//   This routine returns no value.
//
// History:
//   03-Apr-1991    -by-    Kent Settle     (kentse)
//  Rewrote it.
//   25-Mar-1991    -by-    Kent Settle    (kentse)
//  Stole from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

void GetImageableArea(prect, pbufs)
RECTL      *prect;
PPARSEDATA  pbufs;
{
    prect->left = GetNumber(pbufs);
    prect->bottom = GetNumber(pbufs);
    prect->right = GetNumber(pbufs);
    prect->top = GetNumber(pbufs);
}


//--------------------------------------------------------------------
// szLength(pszScan)
//
// This routine calculates the length of a given string, including
// the terminating NULL.  This routine checks to make sure the
// string is not longer than MAX_STRING.
//
// History:
//   19-Mar-1991        -by-    Kent Settle     (kentse)
// Created.
//--------------------------------------------------------------------

int szLength(pszScan)
char    *pszScan;
{
    int i;
    char *pszTmp;

    pszTmp = pszScan;

    i = 1;
    while (*pszScan++ != '\0')
        i++;

    // do a little internal checking.

    if (i > MAX_PPD_STRING) {

        PPDDBG(("String Length too long!\n"));
        PPDDBG(("Offending String: \"%s\"", pszTmp));
        DBGPS(DBGPS_LEVEL_ERROR,
            ("PSCRIPT!szLength:  about to overrun buffer.\n"));
    }

    return(i);
}


//--------------------------------------------------------------------------
//
// BOOL szIsEqual(sz1, sz2)
// char *sz1;
// char *sz2;
//
// This routine compares two NULL terminated strings.
//
// Parameters:
//   sz1
//     Pointer to string 1.
//
//   sz2
//     Pointer to string2.
//
// Returns:
//   This routine returns TRUE if strings are same, FALSE otherwise.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

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


//--------------------------------------------------------------------------
// int NameComp(pname1, pname2)
// CHAR   *pname1;
// CHAR   *pname2;
//
// This routine is a glorified version of strcmp, in that it first gets
// rid of any PostScript translation strings before comparing the strings.
//
// Returns same as strcmp.
//
// History:
//   20-Mar-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

int NameComp(pname1, pname2)
CHAR   *pname1;
CHAR   *pname2;
{
    CHAR    buf1[MAX_PS_NAME];
    CHAR    buf2[MAX_PS_NAME];
    PSTR    pstr1, pstr2;

    // loop through the first name.  copy it into a buffer until we hit
    // either the NULL terminator, or the '/' translation string deliminator.

    pstr1 = pname1;
    pstr2 = buf1;

    while (*pstr1 && (*pstr1 != '/'))
        *pstr2++ = *pstr1++;

    *pstr2 = '\0';

    // now do the same for the second name.

    pstr1 = pname2;
    pstr2 = buf2;

    while (*pstr1 && (*pstr1 != '/'))
        *pstr2++ = *pstr1++;

    *pstr2 = '\0';

    // now both buffers contain the names with any translation strings removed.

    return(strcmp(buf1, buf2));
}


//--------------------------------------------------------------------------
// int NameComp(pname1, pname2)
// CHAR   *pname1;
// CHAR   *pname2;
//
// This routine is a glorified version of strcmp, in that it first gets
// rid of any PostScript translation strings before comparing the strings.
//
// Returns same as strcmp.
//
// History:
//   20-Mar-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ParseProtocols(pbufs, pntpd)
PPARSEDATA      pbufs;
PNTPD           pntpd;
{
    CHAR    buf[256];
    CHAR   *pbuf;
    BOOL    bMore, bWordDone;
    CHAR    jCh;
    DWORD   cjBuf;

    // there may be several protocols defined, so loop until we get
    // them all.

    bMore = TRUE;

    while (bMore)
    {
        // gobble up any white space.

        EatWhite(pbufs);

        pbuf = buf;
        cjBuf = sizeof(buf) - 1;

        bWordDone = FALSE;

        while(cjBuf--)
        {
            switch(jCh = *(pbufs->szLine++))
            {
                case 0:
                case '\n':     // take care of newline and carriage returns.
                case '\r':
                    --pbufs->szLine;
                    *pbuf = 0;
                    bMore = FALSE;
                    bWordDone = TRUE;
                    break;
                case ' ':
                case '\t':
                    --pbufs->szLine;
                    *pbuf = 0;
                    bWordDone = TRUE;
                    break;
                default:
                    *pbuf++ = jCh;
                    break;
            }

            if (bWordDone) {

                // reset pointer to start of buffer.

                pbuf = buf;
                cjBuf = sizeof(buf) - 1;

                if (!(strncmp(pbuf, "PJL", 3))) {

                    pntpd->flFlags |= PJL_PROTOCOL;
                    PPDDBG(("Device supports PJL protocol.\n"));
                } else if (!(strncmp(pbuf, "SIC", 3))) {

                    pntpd->flFlags |= SIC_PROTOCOL;
                    PPDDBG(("Device supports SIC protocol.\n"));
                }

                // now go see if there is another protocol to parse.

                break;
            }
        }

    }
}


/*  Define Global critical object to protect static data */

CRITICAL_SECTION criticalPPDparse;

/*  For Initialize/DeleteCriticalSection, see these functions:
        DrvEnableDriver, DrvDisableDrv (for pscript.dll)
        DLLInitialize (for pscrptui.dll)
*/

//  03/29/95 -davidx-
//      Make function multithread safe.

PNTPD LoadPPD(LPTSTR lpppdname)
{
    // local cache for parsed PPD information

    static TCHAR st_ppdname[MAX_PATH] = L"";
    static TMP_NTPD st_ppd_raw;
    static NTPD st_ppd_stub;

    HANDLE hppdfile = INVALID_HANDLE_VALUE;
    DWORD ppdsize;
    PNTPD pppd_final;
    BOOL ioerror = FALSE;


    if (!*lpppdname) return NULL;

    // In order to be multithread safe, we need to protect
    // the local static variables in a critical region.

    EnterCriticalSection(&criticalPPDparse);
    
    try {

        // parse ppd afresh if different from static storage

        if (wcsicmp(st_ppdname, lpppdname)) {

            ioerror = TRUE;

            hppdfile = CreateFile(
                            lpppdname,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

            if (hppdfile != INVALID_HANDLE_VALUE) {

                InitNTPD(&st_ppd_stub, &st_ppd_raw);
                ParsePPD(hppdfile, &st_ppd_stub, &st_ppd_raw);
                wcscpy(st_ppdname, lpppdname);
                ioerror = FALSE;
            }
        }

    } finally {

        if (hppdfile != INVALID_HANDLE_VALUE)
            CloseHandle(hppdfile);
    }

    if (ioerror) {

        LeaveCriticalSection(&criticalPPDparse);
        return NULL;
    }

    ppdsize = SizeNTPD(&st_ppd_stub, &st_ppd_raw);

    // allocate some memory to build the NTPD structure in.

    pppd_final = (PNTPD) GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, ppdsize);

    if (pppd_final == NULL) {

        DBGPS(DBGPS_LEVEL_ERROR, ("pslib!LoadPPD: GlobalAlloc failed.\n"));
        LeaveCriticalSection(&criticalPPDparse);
        return NULL;
    }

    // ParsePPD will have filled in the NTPD stub structure, so copy
    // it to the real NTPD structure, then call BuildNTPD to add to it.

    memcpy(pppd_final, &st_ppd_stub, sizeof(NTPD));

    // now move data from the TMP_NTPD structure, into the more compact
    // NTPD structure.

    BuildNTPD(pppd_final, &st_ppd_raw);

    // It's ok to allow other threads to use
    // the local static variables now.

    LeaveCriticalSection(&criticalPPDparse);

    return pppd_final;
}

