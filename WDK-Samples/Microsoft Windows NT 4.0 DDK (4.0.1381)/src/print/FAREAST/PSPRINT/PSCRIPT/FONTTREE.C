/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    fonttree.c

Abstract:

    Implementation of DDI entry point DrvQueryFontTree.

[Environment:]

	Win32 subsystem, PostScript driver

Revision History:

	04/18/91 -kentse-
		Created it.

	08/08/95 -davidx-
		Clean up.

	mm/dd/yy -author-
		description

--*/

#include "pscript.h"

// Pointer to a set of glyphs supported by pscript driver.

FD_GLYPHSET *gpGlyphSet;

#if defined(DBCS) // FE font

#define FONTENC_ANY MAXFONTENC
#define CHARSET_ANY MAXCHARSET

struct {
    WORD wEncoding;
    WORD wCharSet;
    FD_GLYPHSET *p;
} gpGlyphSetArray[] = {
    { FONTENC_RKSJ, CHARSET_ANY, NULL },
    { FONTENC_B5, CHARSET_ANY, NULL },
    { FONTENC_UHC, CHARSET_KSCMS, NULL },
    { FONTENC_EUC, CHARSET_GB, NULL }
};
#define MAXGLYPHSET \
    (sizeof (gpGlyphSetArray)/sizeof (*gpGlyphSetArray))

FD_GLYPHSET *
GetPsGlyphSet(
    PDEVDATA pdev,
    DWORD iFace
    );

extern HSEMAPHORE  hGlyphSetSemaphore;

#endif // DBCS


PVOID
DrvQueryFontTree(
    DHPDEV    dhpdev,
    ULONG     iFile,
    ULONG     iFace,
    ULONG     iMode,
    ULONG    *pid
    )
/*++

Routine Description:

    This function implements DDI entry point DrvQueryFontTree.
    Please refer to DDK documentation for more details.

--*/

{
    PDEVDATA    pdev;
    PNTFM       pntfm;
#if defined(DBCS) // FE font
    FD_GLYPHSET *pGlyphSet;
#endif // DBCS

    UNREFERENCED_PARAMETER(iFile);

	TRACEDDIENTRY("DrvQueryFontTree");

    // This can be used by the driver to flag or id the data returned.
    // May be useful for deletion of the data later by DrvFree().
    // Not used by pscript driver.

    *pid = 0;

    // Get a pointer to our PDEV and validate it

    pdev = (PDEVDATA) dhpdev;

    if (bValidatePDEV(pdev) == FALSE) {
		DBGERRMSG("bValidatePDEV");
		SETLASTERROR(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    // Make sure the font index is valid.

    if (! ValidPsFontIndex(pdev, iFace)) {
        DBGERRMSG("ValidPsFontIndex");
		SETLASTERROR(ERROR_INVALID_PARAMETER);
		return NULL;
    }

    switch (iMode) {

	case QFT_GLYPHSET:

        // GDI requests a pointer to a FD_GLYPHSET structure that defines
        // the mappings from single Unicode characters to glyph handles.

#if defined(DBCS) // FE font

        ACQUIRESEMAPHORE(hGlyphSetSemaphore);
        pGlyphSet = GetPsGlyphSet(pdev, iFace);
        RELEASESEMAPHORE(hGlyphSetSemaphore);
        return pGlyphSet;

#else // DBCS

        return gpGlyphSet;

#endif // DBCS

	case QFT_KERNPAIRS:

        // GDI requests a pointer to a sorted, NULL-terminated
        // array of FD_KERNINGPAIR structures.
    
        // Get the font information for the given font
    
        pntfm = GetPsFontNtfm(pdev, iFace);
    
        // Return a pointer to the FD_KERNINGPAIR structure
    
        return (pntfm->ntfmsz.cKernPairs == 0) ? NULL :
                    ((PBYTE) pntfm + pntfm->ntfmsz.loKernPairs);

	default:
	    DBGMSG1(DBG_LEVEL_ERROR, "Invalid iMode: %d\n", iMode);
	    SETLASTERROR(ERROR_INVALID_PARAMETER);
	    return NULL;
    }
}



BOOL
ComputePsGlyphSet(
    VOID
    )

/*++

Routine Description:

    Compute the glyph set supported by PostScript driver.
    This information is returned to GDI when DrvQueryFontTree
    is called with iMode = QFT_GLYPHSET.

Arguments:

    NONE

Return Value:

    TRUE if successful. FALSE otherwise.

--*/

{
    gpGlyphSet = EngComputeGlyphSet(0, 0, 256);
    if (gpGlyphSet == NULL) {
        DBGERRMSG("EngComputeGlyphSet");
    }

    return (gpGlyphSet != NULL);
}


VOID
FreePsGlyphSet(
    VOID
    )

{
#if defined(DBCS) // FE font
    INT i;

    for (i = 0; i < MAXGLYPHSET; i++) {
        if (gpGlyphSetArray[i].p != NULL)
        {
            MEMFREE(gpGlyphSetArray[i].p);
            gpGlyphSetArray[i].p = NULL;
        }
    }
#endif // DBCS
	if (gpGlyphSet != NULL) {
		MEMFREE(gpGlyphSet);
#if defined(KKBUGFIX) // NULL
        gpGlyphSet = NULL;
#endif // KKBUGFIX
	}
}

#if defined(DBCS) // FE font

FD_GLYPHSET *
GetPsGlyphSet(
    PDEVDATA pdev,
    DWORD iFace
    )

/*++

Routine Description:


Arguments:

    WORD

Return Value:

    TRUE if successful. FALSE otherwise.

--*/
{
    FD_GLYPHSET *pGlyphSet;
    ULONG       ulSize;
    WCRUN       *pWcRun;
    HGLYPH      *phg;
    UINT        i;
    BYTE        *pRes;

    PDEVFONT    pfont;
    PNTFM       pntfm;
    PIFIMETRICS pifi;
    WORD        wEncoding;
    WORD        wCharSet;
    INT         iGlyphSet;

    pfont = (PDEVFONT)
        LISTOBJ_FindIndexed((PLISTOBJ) pdev->hppd->pFontList, iFace-1);
    wEncoding = pfont ? pfont->wEncoding : -1;
    wCharSet = pfont ? pfont->wCharSet : -1;

    switch (wEncoding)
    {
    case FONTENC_RKSJ:
    case FONTENC_B5:
    case FONTENC_UHC:

        // Language specific encodings.  No more investigation is needed
        // to identify glyphset for this font.

        break;

    case FONTENC_EUC:

        // Specical handling for KSC, map KSC-EUC to KSCms-UHC since
        // the latter is the superset of former.

        if (wCharSet == CHARSET_KSC) {
            wEncoding = FONTENC_UHC;
            wCharSet = CHARSET_KSCMS;
        }

        break;

    default:

        // We must respect charset information specificed in PFM since
        // some of the PPD files may be wrong in terms of *Font attributes.
    
        pntfm = GetPsFontNtfm(pdev, iFace);

        pifi = (PIFIMETRICS) ((PBYTE) pntfm + pntfm->ntfmsz.loIFIMETRICS);

        switch (pifi->jWinCharSet)
        {
        case SHIFTJIS_CHARSET:
            wEncoding = FONTENC_RKSJ;
            wCharSet = CHARSET_90MS;
            break;
        case CHINESEBIG5_CHARSET:
            wEncoding = FONTENC_B5;
            break;
        case GB2312_CHARSET:
            wEncoding = FONTENC_EUC;
            wCharSet = CHARSET_GB;
            break;
        case HANGEUL_CHARSET:
            wEncoding = FONTENC_UHC;
            wCharSet = CHARSET_KSCMS;
            break;
        default:
            // others, default to ANSI.
            return gpGlyphSet;
        }
    }

    // Search for matching glyphset index.  If it is already
    // loaded, just returns its address to the caller.

    for ( iGlyphSet = 0; iGlyphSet < MAXGLYPHSET; iGlyphSet++)
    {
        if (gpGlyphSetArray[iGlyphSet].wEncoding == wEncoding
                && (gpGlyphSetArray[iGlyphSet].wCharSet == wCharSet
                || gpGlyphSetArray[iGlyphSet].wCharSet == CHARSET_ANY))
            break;
    }

    // If not matched, default to ANSI.

    if (iGlyphSet == MAXGLYPHSET)
    {
        return gpGlyphSet;
    }

    // Glyphset already loaded, just return to the caller.

    if (gpGlyphSetArray[iGlyphSet].p != NULL)
        return gpGlyphSetArray[iGlyphSet].p;

    // Load specified glyphset.

    if ((pRes = EngFindResource(
            pdev->hModule, wEncoding, GLYPHSET, &ulSize)) == NULL)
    {
        DBGMSG(DBG_LEVEL_ERROR,
            "Could not find resource\n");
        return NULL;
    }

    // Duplicate resource into R/W memory

    if ((pGlyphSet = MEMALLOC(ulSize)) == NULL)
    {
        DBGMSG(DBG_LEVEL_ERROR,
            "Could not allocate enough memory\n");
        return NULL;
    }

    memcpy( pGlyphSet, pRes, ulSize );

    // Fixup address.

    pWcRun = &pGlyphSet->awcrun[0];
    phg = (HGLYPH *)((PBYTE)pGlyphSet
	    + offsetof(FD_GLYPHSET, awcrun[pGlyphSet->cRuns]));

    for (i = 0; i < pGlyphSet->cRuns; i++, pWcRun++) {
        pWcRun->phg = phg;
        phg += pWcRun->cGlyphs;
    }

    // Save for future referenece.

    gpGlyphSetArray[iGlyphSet].p = pGlyphSet;

    return pGlyphSet;
}

#endif // DBCS


