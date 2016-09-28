//--------------------------------------------------------------------------
//
// Module Name:  PFM.H
//
// Brief Description:  This module contains the PSCRIPT driver's
// font metrics defines.
//
// Author:  Kent Settle (kentse)
// Created: 22-Jan-1991
//
// Copyright (C) 1991 - 1992 Microsoft Corporation.
//--------------------------------------------------------------------------

#define MAX_KERNPAIRS   1024

#define ANSI_CHARSET    0
#define SYMBOL_CHARSET  2
#define OEM_CHARSET     255

#define NTM_VERSION     0x00010000
#define FL_NTFM_SOFTFONT              1
#define FL_NTFM_NO_TRANSLATE_CHARSET  2


#define INIT_IFI    2048
#define INIT_PFM  262144 + INIT_IFI   // storage to allocate to build NTFM.

#define MIN_UNICODE_VALUE       0
#define MAX_UNICODE_VALUE       0xFFFE
#define INVALID_UNICODE_VALUE   0xFFFF
// The AFM tokens.

#define TK_UNDEFINED            0
#define TK_STARTKERNDATA        2
#define TK_STARTKERNPAIRS       3
#define TK_KPX                  4
#define TK_ENDKERNPAIRS         5
#define TK_ENDKERNDATA          6
#define TK_FONTNAME             7
#define TK_WEIGHT               8
#define TK_ITALICANGLE          9
#define TK_ISFIXEDPITCH         10
#define TK_UNDERLINEPOSITION    11
#define TK_UNDERLINETHICKNESS   12
#define TK_FONTBBOX             13
#define TK_CAPHEIGHT            14
#define TK_XHEIGHT              15
#define TK_DESCENDER            16
#define TK_ASCENDER             17
#define TK_STARTCHARMETRICS     18
#define TK_ENDCHARMETRICS       19
#define TK_ENDFONTMETRICS       20
#define TK_STARTFONTMETRICS     21
#define TK_ENCODINGSCHEME       22
#define TK_FULLNAME             23
#define TK_FAMILYNAME           24
#define TK_MSFAMILY             25

// font defines.

#define ARIAL                               1
#define ARIAL_BOLD                          2
#define ARIAL_BOLDOBLIQUE                   3
#define ARIAL_OBLIQUE                       4
#define ARIAL_NARROW                        5
#define ARIAL_NARROW_BOLD                   6
#define ARIAL_NARROW_BOLDOBLIQUE            7
#define ARIAL_NARROW_OBLIQUE                8
#define AVANTGARDE_BOOK                     9
#define AVANTGARDE_BOOKOBLIQUE              10
#define AVANTGARDE_DEMI                     11
#define AVANTGARDE_DEMIOBLIQUE              12
#define BOOKMAN_DEMI                        13
#define BOOKMAN_DEMIITALIC                  14
#define BOOKMAN_LIGHT                       15
#define BOOKMAN_LIGHTITALIC                 16
#define COURIER                             17
#define COURIER_BOLD                        18
#define COURIER_BOLDOBLIQUE                 19
#define COURIER_OBLIQUE                     20
#define GARAMOND_BOLD                       21
#define GARAMOND_BOLDITALIC                 22
#define GARAMOND_LIGHT                      23
#define GARAMOND_LIGHTITALIC                24
#define HELVETICA                           25
#define HELVETICA_BLACK                     26
#define HELVETICA_BLACKOBLIQUE              27
#define HELVETICA_BOLD                      28
#define HELVETICA_BOLDOBLIQUE               29
#define HELVETICA_CONDENSED                 30
#define HELVETICA_CONDENSED_BOLD            31
#define HELVETICA_CONDENSED_BOLDOBL         32
#define HELVETICA_CONDENSED_OBLIQUE         33
#define HELVETICA_LIGHT                     34
#define HELVETICA_LIGHTOBLIQUE              35
#define HELVETICA_NARROW                    36
#define HELVETICA_NARROW_BOLD               37
#define HELVETICA_NARROW_BOLDOBLIQUE        38
#define HELVETICA_NARROW_OBLIQUE            39
#define HELVETICA_OBLIQUE                   40
#define KORINNA_BOLD                        41
#define KORINNA_KURSIVBOLD                  42
#define KORINNA_KURSIVREGULAR               43
#define KORINNA_REGULAR                     44
#define LUBALINGRAPH_BOOK                   45
#define LUBALINGRAPH_BOOKOBLIQUE            46
#define LUBALINGRAPH_DEMI                   47
#define LUBALINGRAPH_DEMIOBLIQUE            48
#define NEWCENTURYSCHLBK_BOLD               49
#define NEWCENTURYSCHLBK_BOLDITALIC         50
#define NEWCENTURYSCHLBK_ITALIC             51
#define NEWCENTURYSCHLBK_ROMAN              52
#define PALATINO_BOLD                       53
#define PALATINO_BOLDITALIC                 54
#define PALATINO_ITALIC                     55
#define PALATINO_ROMAN                      56
#define SOUVENIR_DEMI                       57
#define SOUVENIR_DEMIITALIC                 58
#define SOUVENIR_LIGHT                      59
#define SOUVENIR_LIGHTITALIC                60
#define SYMBOL                              61
#define TIMES_BOLD                          62
#define TIMES_BOLDITALIC                    63
#define TIMES_ITALIC                        64
#define TIMES_ROMAN                         65
#define TIMES_NEW_ROMAN                     66
#define TIMES_NEW_ROMAN_BOLD                67
#define TIMES_NEW_ROMAN_BOLDITALIC          68
#define TIMES_NEW_ROMAN_ITALIC              69
#define VARITIMES_BOLD                      70
#define VARITIMES_BOLDITALIC                71
#define VARITIMES_ITALIC                    72
#define VARITIMES_ROMAN                     73
#define ZAPFCALLIGRAPHIC_BOLD               74
#define ZAPFCALLIGRAPHIC_BOLDITALIC         75
#define ZAPFCALLIGRAPHIC_ITALIC             76
#define ZAPFCALLIGRAPHIC_ROMAN              77
#define ZAPFCHANCERY_MEDIUMITALIC           78
#define ZAPFDINGBATS                        79

#if defined(DBCS) // FE font

// Every FE typeface has font index IDs for both horizontal and vertical
// vertical version.  Some of these typefaces do not have vertical (-V)
// version of the font file, but we will asssign IDs as if they exist so
// that we can simulate them easily.

// The IDs for vertical writing fonts can be obtained by adding 2,000 to
// the IDs of the horizontal variants's IDs.

#define FONTINDEX_FEVERT_BASE               2000

// Japanese fonts.

#define FUTOGOB101_BOLD                     100
#define FUTOMINA101_BOLD                    101
#define GOTHICBBB_MEDIUM                    102
#define HEISEIKAKUGO_W3                     103
#define HEISEIKAKUGO_W5                     104
#define HEISEIKAKUGO_W7                     105
#define HEISEIKAKUGO_W9                     106
#define HEISEIMARUGO_W4                     107
#define HEISEIMIN_W3                        108
#define HEISEIMIN_W9                        109
#define JUN101_LIGHT                        110
#define JUN34_MEDIUM                        111
#define JUN501_BOLD                         112
#define MIDASHIGO_MB31                      113
#define MIDASHIMIN_MA31                     114
#define RYUMIN_BOLD                         115
#define RYUMIN_HEAVY                        116
#define RYUMIN_LIGHT                        117
#define RYUMIN_MEDIUM                       118
#define RYUMIN_REGULAR                      119
#define RYUMIN_ULTRA                        120
#define SHINGO_BOLD                         121
#define SHINGO_LIGHT                        122
#define SHINGO_MEDIUM                       123
#define SHINGO_REGULAR                      124
#define SHINGO_ULTRA                        125
#define SHINSEIKAI_CBSK1                    126
#define TYPEBANKG_B                         127
#define TYPEBANKG_HV                        128
#define TYPEBANKM_HV                        129
#define TYPEBANKM_M                         130

#define QMS_TYPEBANKM_M                     131
#define QMS_TYPEBANKM_H                     132
#define QMS_TYPEBANKG_B                     133
#define QMS_TYPEBANKG_H                     134

#define HEISEIKAKUGOTHICW5_MEDIUM           135
#define HEISEIMINCHOW3_LIGHT                136
#define NIS_M5                              137
#define NIS_R1                              138
#define NIS_S7                              139

#define RICOHGOTHICB                        140
#define RICOHGOTHICEB                       141
#define RICOHGOTHICM                        142
#define RICOHGYOSHO                         143
#define RICOHKAISHO                         144
#define RICOHKYOKASHO                       145
#define RICOHMGOTHICB                       146
#define RICOHMGOTHICL                       147
#define RICOHMGOTHICM                       148
#define RICOHMINCHOB                        149
#define RICOHMINCHOEB                       150
#define RICOHMINCHOL                        151

// Japanese fonts (vertical).

#define FUTOGOB101_BOLD_VERT               2100
#define FUTOMINA101_BOLD_VERT              2101
#define GOTHICBBB_MEDIUM_VERT              2102
#define HEISEIKAKUGO_W3_VERT               2103
#define HEISEIKAKUGO_W5_VERT               2104
#define HEISEIKAKUGO_W7_VERT               2105
#define HEISEIKAKUGO_W9_VERT               2106
#define HEISEIMARUGO_W4_VERT               2107
#define HEISEIMIN_W3_VERT                  2108
#define HEISEIMIN_W9_VERT                  2109
#define JUN101_LIGHT_VERT                  2110
#define JUN34_MEDIUM_VERT                  2111
#define JUN501_BOLD_VERT                   2112
#define MIDASHIGO_MB31_VERT                2113
#define MIDASHIMIN_MA31_VERT               2114
#define RYUMIN_BOLD_VERT                   2115
#define RYUMIN_HEAVY_VERT                  2116
#define RYUMIN_LIGHT_VERT                  2117
#define RYUMIN_MEDIUM_VERT                 2118
#define RYUMIN_REGULAR_VERT                2119
#define RYUMIN_ULTRA_VERT                  2120
#define SHINGO_BOLD_VERT                   2121
#define SHINGO_LIGHT_VERT                  2122
#define SHINGO_MEDIUM_VERT                 2123
#define SHINGO_REGULAR_VERT                2124
#define SHINGO_ULTRA_VERT                  2125
#define SHINSEIKAI_CBSK1_VERT              2126
#define TYPEBANKG_B_VERT                   2127
#define TYPEBANKG_HV_VERT                  2128
#define TYPEBANKM_HV_VERT                  2129
#define TYPEBANKM_M_VERT                   2130

#define QMS_TYPEBANKM_M_VERT               2131
#define QMS_TYPEBANKM_H_VERT               2132
#define QMS_TYPEBANKG_B_VERT               2133
#define QMS_TYPEBANKG_H_VERT               2134

#define HEISEIKAKUGOTHICW5_MEDIUM_VERT     2135
#define HEISEIMINCHOW3_LIGHT_VERT          2136
#define NIS_M5_VERT                        2137
#define NIS_R1_VERT                        2138
#define NIS_S7_VERT                        2139

#define RICOHGOTHICB_VERT                  2140
#define RICOHGOTHICEB_VERT                 2141
#define RICOHGOTHICM_VERT                  2142
#define RICOHGYOSHO_VERT                   2143
#define RICOHKAISHO_VERT                   2144
#define RICOHKYOKASHO_VERT                 2145
#define RICOHMGOTHICB_VERT                 2146
#define RICOHMGOTHICL_VERT                 2147
#define RICOHMGOTHICM_VERT                 2148
#define RICOHMINCHOB_VERT                  2149
#define RICOHMINCHOEB_VERT                 2150
#define RICOHMINCHOL_VERT                  2151

// Traditional Chinese fnots.

#define HEI_LIGHT                           200
#define KAI_LIGHT                           201
#define SUNG_LIGHT                          202

#define DFFANGSONG_W4                       203
#define DFGIRL_W6                           204
#define DFGUYIN_MD                          205
#define DFHEI_BD                            206
#define DFHEI_MD                            207
#define DFHEI_UB                            208
#define DFHEI_XB                            209
#define DFKAISHU_W5                         210
#define DFKANTINGLIU_XB                     211
#define DFLIHEI_BD                          212
#define DFLIHEI_LT                          213
#define DFLIHEI_MD                          214
#define DFLIKAISHU_MD                       215
#define DFLIKINGHEI_XB                      216
#define DFLISHU_W5                          217
#define DFLISONG_BD                         218
#define DFLISONG_LT                         219
#define DFLISONG_MD                         220
#define DFLIYEASONG_BD                      221
#define DFLIYUAN_BD                         222
#define DFLIYUAN_XB                         223
#define DFMING_BD                           224
#define DFMING_LT                           225
#define DFMING_MD                           226
#define DFMING_UB                           227
#define DFMING_XB                           228
#define DFPOP1_W7                           229
#define DFSONG_W4                           230
#define DFXINGSHU_MD                        231
#define DFYUAN_BD                           232
#define DFYUAN_LT                           233
#define DFYUAN_XB                           234
#define DFZONGYI_XB                         235

#endif // DBCS

#define FIRST_FONT                          1
#define DEFAULT_FONT                        COURIER
#define NUM_INTERNAL_FONTS                  79

extern PutByte(SHORT);
extern PutWord(SHORT);
extern PutLong(long);

typedef USHORT  SOFFSET;        // short offset.

#define DWORDALIGN(a) ((a + (sizeof(DWORD) - 1)) & ~(sizeof(DWORD) - 1))
#define WCHARALIGN(a) ((a + (sizeof(WCHAR) - 1)) & ~(sizeof(WCHAR) - 1))

// entry for each soft font.

// NT Font Metrics structure.

typedef ULONG   LOFFSET;        // long offset.

typedef struct
{
    ULONG   cjNTFM;             // size of NTFM struct, with attached data.
    LOFFSET loszFontName;       // offset to FontName.
    LOFFSET loIFIMETRICS;       // offset to IFIMETRICS structure.
    ULONG   cKernPairs;
    LOFFSET loKernPairs;        // offset to start of FD_KERNINGPAIR structs.
} NTFMSZ;


typedef struct _EXTTEXTMETRIC {
    SHORT  etmSize;
    SHORT  etmPointSize;
    SHORT  etmOrientation;
    SHORT  etmMasterHeight;
    SHORT  etmMinScale;
    SHORT  etmMaxScale;
    SHORT  etmMasterUnits;
    SHORT  etmCapHeight;
    SHORT  etmXHeight;
    SHORT  etmLowerCaseAscent;
    SHORT  etmLowerCaseDescent;
    SHORT  etmSlant;
    SHORT  etmSuperScript;
    SHORT  etmSubScript;
    SHORT  etmSuperScriptSize;
    SHORT  etmSubScriptSize;
    SHORT  etmUnderlineOffset;
    SHORT  etmUnderlineWidth;
    SHORT  etmDoubleUpperUnderlineOffset;
    SHORT  etmDoubleLowerUnderlineOffset;
    SHORT  etmDoubleUpperUnderlineWidth;
    SHORT  etmDoubleLowerUnderlineWidth;
    SHORT  etmStrikeOutOffset;
    SHORT  etmStrikeOutWidth;
    WORD   etmNKernPairs;
    WORD   etmNKernTracks;
} EXTTEXTMETRIC;

typedef struct
{
    ULONG           ulVersion;          // version
    NTFMSZ          ntfmsz;             // size inormation
    FLONG           flNTFM;             // flags [bodind]
    EXTTEXTMETRIC   etm;
    USHORT          ausCharWidths[256];
} NTFM, *PNTFM;

// This is value needed to determine if a particular soft font needs
// encoding vector remapping (stolen win31 source code) [bodind]

#define NO_TRANSLATE_CHARSET    200

// Maximum length of font names

#define MAX_FONTNAME            128

// An estimate of average PS font size =~ 33K
#ifdef  DBCS
#define AVERAGE_FONT_SIZE       (330*1024)
#else
#define AVERAGE_FONT_SIZE       (33*1024)
#endif

#if defined(DBCS) // FE font

//#define A_LATIN1_DEFAULT_CHAR 0x0095 // BULETTE (LATIN1)
//#define U_LATIN1_DEFAULT_CHAR 0x2022 // BULETTE (UNICODE)

#define A_CHINESEBIG5_BREAK_CHAR 0x0020 // SPACE (BIG5)
#define U_CHINESEBIG5_BREAK_CHAR 0x0020 // SPACE (UNICODE)
#define A_CHINESEBIG5_DEFAULT_CHAR 0x003f // QUESTION MARK (BIG5)
#define U_CHINESEBIG5_DEFAULT_CHAR 0x30fb // QUESTION MARK (UNICODE)
#define U_CHINESEBIG5_FIRST_CHAR 0x0000 // NULL (UNICODE)
#define U_CHINESEBIG5_LAST_CHAR 0xffe5 // FULLWIDTH YEN SIGN (UNICODE)

#define A_GB2312_BREAK_CHAR 0x0020 // SPACE (SHIFT-JIS)
#define U_GB2312_BREAK_CHAR 0x0020 // SPACE (UNICODE)
#define A_GB2312_DEFAULT_CHAR 0x003f // QUESTION MARK (BIG5)
#define U_GB2312_DEFAULT_CHAR 0x30fb // QUESTION MARK (UNICODE)
#define U_GB2312_FIRST_CHAR 0x0000 // NULL (UNICODE)
#define U_GB2312_LAST_CHAR 0xfffd // ???? (UNICODE)

#define A_HANGEUL_BREAK_CHAR 0x0020 // SPACE (SHIFT-JIS)
#define U_HANGEUL_BREAK_CHAR 0x0020 // SPACE (UNICODE)
#define A_HANGEUL_DEFAULT_CHAR 0x003f // QUESTION MARK (BIG5)
#define U_HANGEUL_DEFAULT_CHAR 0x30fb // QUESTION MARK (UNICODE)
#define U_HANGEUL_FIRST_CHAR 0x0000 // NULL (UNICODE)
#define U_HANGEUL_LAST_CHAR 0xfffd // ???? (UNICODE)

#define A_SHIFTJIS_BREAK_CHAR 0x0020 // SPACE (SHIFT-JIS)
#define U_SHIFTJIS_BREAK_CHAR 0x0020 // SPACE (UNICODE)
#define A_SHIFTJIS_DEFAULT_CHAR 0x00a5 // HALFWIDTH KATAKANA MIDDLE DOT (SHIFT-JIS)
#define U_SHIFTJIS_DEFAULT_CHAR 0xff65 // HALFWIDTH KATAKANA MIDDLE DOT (UNICODE)
#define U_SHIFTJIS_FIRST_CHAR 0x0000 // NULL (UNICODE)
#define U_SHIFTJIS_LAST_CHAR 0xffe5 // FULLWIDTH YEN SIGN (UNICODE)

#endif // DBCS

/*--------------------------------------------------------------------*\
*  The PFB file format is a sequence of segments, each of which has a  *
*  header part and a data part. The header format, defined in the      *
*  struct PFBHEADER below, consists of a one byte sanity check number  *
*  (128) then a one byte segment type and finally a four byte length   *
*  field for the data following data. The length field is stored in    *
*  the file with the least significant byte first.                     *
*                                                                      *
*  The segment types are:                                              *
*  1.) The data is a sequence of ASCII characters.                     *
*  2.) The data is a sequence of binary characters to be converted     *
*      to a sequence of pairs of hexadecimal digits.                   *
*  3.) The last segment in the file. This segment has no length or     *
*      data fields.                                                    *
*                                                                      *
*  The segment types are defined explicitly rather than as an          *
*  enumerated type because the values for each type are defined by the *
*  file format rather than the compiler manipulating them.             *
\*--------------------------------------------------------------------*/

#define CHECK_BYTE      128         // first byte of file segment
#define ASCII_TYPE      1           // segment type identifier
#define BINARY_TYPE     2
#define EOF_TYPE        3

// Macro to verify whether a PFBHEADER is valid

#define ValidPfbHeader(p)   (*((PBYTE)(p)) == CHECK_BYTE)

// Macro to retrieve the segment type field of PFBHEADER

#define PfbSegmentType(p)   (((PBYTE)(p))[1])

// Macro to retrieve the segment length field of PFBHEADER

#define PfbSegmentLength(p) (((DWORD) ((PBYTE)(p))[2]      ) |  \
                             ((DWORD) ((PBYTE)(p))[3] <<  8) |  \
                             ((DWORD) ((PBYTE)(p))[4] << 16) |  \
                             ((DWORD) ((PBYTE)(p))[5] << 24))

// Size of PFBHEADER = 6 bytes

#define PFBHEADER_SIZE  6

PNTFM
pntfmConvertPfmToNtm(
    PBYTE    pjPFM,
    BOOL     bSoft
    );
