/**[f******************************************************************
 * etm.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
* This module of the afm compiler generates the extended text metrics
* portion of the PFM file.
*
**********************************************************************
*/
#include "pfm.h"


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
short etmUpperCaseDecent;
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

/* Convert from PostScript to extended text metrics */
AfmToEtm()
    {
#ifdef DBCS
    // set default values if not specified in AFM.
    if (IS_DBCS_CHARSET(afm.iEncodingScheme))
    {
        if(afm.ulOffset == 0)
            afm.ulOffset = 97;
        if(afm.ulThick == 0)
            afm.ulThick = 73;
    }
#endif // DBCS
    etmPointSize = 12 * 20;	/* Nominal point size = 12 */
    etmOrientation = 0;
    etmMasterHeight = 1000;
    etmMinScale = 3;
    etmMaxScale = 1000;
    etmMasterUnits = 1000;
#ifdef DBCS
    // for Japanese PS fonts, we have fixed default value
    // for these metrics.
    if (IS_DBCS_CHARSET(afm.iEncodingScheme))
    {
        etmCapHeight = 1000;
        etmXHeight = 1000;
        etmLowerCaseAscent = 880;
        etmUpperCaseDecent = 120;
    }
    else
    {
        etmCapHeight = afm.rgcm['H'].rc.top;
        etmXHeight = afm.rgcm['x'].rc.top;
        etmLowerCaseAscent =  afm.rgcm['d'].rc.top;
        etmUpperCaseDecent = - afm.rgcm['p'].rc.bottom;
    }
#else // DBCS
    etmCapHeight = afm.rgcm['H'].rc.top;
    etmXHeight = afm.rgcm['x'].rc.top;
    etmLowerCaseAscent =  afm.rgcm['d'].rc.top;
    etmUpperCaseDecent = - afm.rgcm['p'].rc.bottom;
#endif // DBCS
    etmSlant = afm.iItalicAngle;
    etmSuperScript = -500;
    etmSubScript = 250;
    etmSuperScriptSize = 500;
    etmSubScriptSize = 500;
    etmUnderlineOffset = afm.ulOffset;
    etmUnderlineWidth = afm.ulThick;
    etmDoubleUpperUnderlineOffset = afm.ulOffset/2;
    etmDoubleLowerUnderlineOffset = afm.ulOffset;
    etmDoubleUpperUnderlineWidth = afm.ulThick / 2;
    etmDoubleLowerUnderlineWidth = afm.ulThick / 2;
    etmStrikeOutOffset = 500;
    etmStrikeOutWidth = 100;
    etmNKernPairs = afm.kp.cPairs;
    etmNKernTracks = 0;
    }


PutEtm(iFont)
int iFont;
    {
    AfmToEtm();
    PutWord(etmSize);

    PutWord(etmPointSize);
    PutWord(etmOrientation);
    PutWord(etmMasterHeight);
    PutWord(etmMinScale);
    PutWord(etmMaxScale);
    PutWord(etmMasterUnits);
    PutWord(etmCapHeight);
    PutWord(etmXHeight);
    PutWord(etmLowerCaseAscent);
    PutWord(etmUpperCaseDecent);
    PutWord(etmSlant);
    PutWord(etmSuperScript);
    PutWord(etmSubScript);
    PutWord(etmSuperScriptSize);
    PutWord(etmSubScriptSize);
    PutWord(etmUnderlineOffset);
    PutWord(etmUnderlineWidth);
    PutWord(etmDoubleUpperUnderlineOffset);
    PutWord(etmDoubleLowerUnderlineOffset);
    PutWord(etmDoubleUpperUnderlineWidth);
    PutWord(etmDoubleLowerUnderlineWidth);
    PutWord(etmStrikeOutOffset);
    PutWord(etmStrikeOutWidth);
    PutWord(etmNKernPairs);
    PutWord(etmNKernTracks);
    }
