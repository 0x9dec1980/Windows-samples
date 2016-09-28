/**[f******************************************************************
 * pfm.h - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

#define NULL 0
#define TRUE 1
#define FALSE 0


typedef int BOOL;
typedef char BYTE;
typedef short int WORD;
typedef long int DWORD;

#ifdef DBCS
#define IS_DBCS_CHARSET(i) \
    (((i) == CHINESEBIG5_CHARSET) || \
    ((i) == GB2312_CHARSET) || \
    ((i) == SHIFTJIS_CHARSET) || \
    ((i) == HANGEUL_CHARSET))
#define DBCS_MAX_CHARWIDTH 1000
#define DBCS_AVE_CHARWIDTH 495
#endif // DBCS

#define ANSI_CHARSET	      0
#define SYMBOL_CHARSET	      2
#ifdef DBCS
#define SHIFTJIS_CHARSET        128
#define HANGEUL_CHARSET         129
#define GB2312_CHARSET          134
#define CHINESEBIG5_CHARSET     136
#endif // DBCS
#define OEM_CHARSET	      255

#ifdef DBCS
#define FW_DONTCARE         0
#define FW_THIN             100
#define FW_EXTRALIGHT       200
#define FW_LIGHT            300
#define FW_NORMAL           400
#define FW_MEDIUM           500
#define FW_SEMIBOLD         600
#define FW_BOLD             700
#define FW_EXTRABOLD        800
#define FW_HEAVY            900
#define FW_ULTRALIGHT       FW_EXTRALIGHT
#define FW_REGULAR          FW_NORMAL
#define FW_DEMIBOLD         FW_SEMIBOLD
#define FW_ULTRABOLD        FW_EXTRABOLD
#define FW_BLACK            FW_HEAVY
#else // DBCS
#define FW_LIGHT    250
#define FW_NORMAL   400
#define FW_BOLD     700
#endif // DBCS

/*	GDI font families.						*/
#define FF_DONTCARE	(0<<4)	/* Don't care or don't know.		*/
#define FF_ROMAN	(1<<4)	/* Variable stroke width, serifed.	*/
				/* Times Roman, Century Schoolbook, etc.*/
#define FF_SWISS	(2<<4)	/* Variable stroke width, sans-serifed. */
				/* Helvetica, Swiss, etc.		*/
#define FF_MODERN	(3<<4)	/* Constant stroke width, serifed or sans-serifed. */
				/* Pica, Elite, Courier, etc.		*/
#define FF_SCRIPT	(4<<4)	/* Cursive, etc.			*/
#define FF_DECORATIVE	(5<<4)	/* Old English, etc.			*/


/* flags for fFlags field of AFM structure */
#define FFLAGS_TRUETYPE 1

typedef struct
    {
    short left;
    short bottom;
    short right;
    short top;
    }RECT;


typedef struct EMM
    {
    short capHeight;
    short xHeight;
    short loAscent;	    /* Lower-case ascent */
    short loDescent;	    /* Lower-case descent */
    short ulOffset;	    /* The underline offset */
    short ulThick;	    /* The underline thickness */
    short iSlant;	    /* The italic angle */
    RECT  rcBounds;	    /* The font bounding box */
    }EMM;


typedef struct
    {
    char szFont[32];		/* The PostScript font name */
    char szFace[32];		/* The face name of the font */
    BOOL fEnumerate;		/* TRUE if the font should be enumerated */
    BOOL fItalic;		/* TRUE if this is an italic font */
    BOOL fSymbol;		/* TRUE if the font is decorative */
    short iFamily;		/* The fonts family */
    short iWeight;		/* TRUE if this is a bold font */
    short iFirstChar;		/* The first character in the font */
    short iLastChar;		/* The last character in the font */
    short rgWidths[256];	/* Character widths from 0x020 to 0x0ff */
    }FONT;



extern PutByte(short int);
extern PutWord(short int);
extern PutLong(long);

typedef struct
    {
    unsigned short int iKey;
    short int iKernAmount;
    }KX;

typedef struct
    {
    int cPairs; 	    /* The number of kerning pairs */
    KX	rgPairs[1024];
    }KP;

/* The info for a single kern track */
typedef struct
    {
    short iDegree;    /* The degree of kerning */
    short iPtMin;     /* The minimum point size */
    short iKernMin;   /* The minimum kern amount */
    short iPtMax;     /* The maximum point size */
    short iKernMax;   /* The maximum kern amount */
    }TRACK;

/* The track kerning table for a font */
typedef struct
    {
    short cTracks;	  /* The number of kern tracks */
    TRACK rgTracks[10];  /* The kern track information */
    }KT;




/* Character metrics */
typedef struct
    {
    RECT    rc;
    int     iWidth;
    }CM;

typedef struct
    {
    int iFirstChar;
    int iLastChar;
    int iAvgWidth;
    int iMaxWidth;
    int iItalicAngle;
    int iFamily;
    int ulOffset;
    int ulThick;
    int iAscent;
    int iDescent;
    int iEncodingScheme;
    int iCapHeight;
    BOOL fVariablePitch;
    char szFile[80];
    char szFont[80];
    char szFace[80];
    int  iWeight;
    KP	 kp;
    KT	 kt;
    RECT rcBBox;
    CM	rgcm[256];	    /* The character metrics */
    int fFlags;         /* placeholder for misc. info (FFLAGS_*) */
    }AFM;

AFM afm;
