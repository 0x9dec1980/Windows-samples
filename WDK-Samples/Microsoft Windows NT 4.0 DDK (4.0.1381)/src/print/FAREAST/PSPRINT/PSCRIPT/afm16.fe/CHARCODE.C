/**[f******************************************************************
 * charcode.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/********************************************************************
* This module of the afm compiler translates the PostScript character
* codes into their numeric values
*
**********************************************************************
*/
#define NULL	0

typedef struct
    {
    char *szKey;
    int iValue;
    }KEY;




int GetCharCode(szWord)
char *szWord;
    {

    static KEY rgKeys[] =
	{
	"space",	    	' ',
	"exclam",	    	'!',
	"quotedbl",	    	'"',
	"numbersign",		'#',
	"dollar",	    	'$',
	"percent",	    	'%',
	"ampersand",	    '&',
	"quoteright",	    0x092,
	"parenleft",	    '(',
	"parenright",	    ')',
	"asterisk",	    	'*',
	"plus", 			'+',
	"comma",	    	',',
	"hyphen",	    	'-',
   "sfthyphen",      0x0ad,
	"period",	    	'.',
	"slash",			'/',
	"zero", 	'0',
	"one",		'1',
	"two",		'2',
	"three",	'3',
	"four", 	'4',
	"five", 	'5',
	"six",		'6',
	"seven",	'7',
	"eight",	'8',
	"nine", 	'9',
	"colon",	    ':',
	"semicolon",	    ';',
	"less", 	'<',
	"equal",	'=',
	"greater",	'>',
	"question",	    '?',
	"at",		    '@',
	"A",	'A',
	"B",	'B',
	"C",	'C',
	"D",	'D',
	"E",	'E',
	"F",	'F',
	"G",	'G',
	"H",	'H',
	"I",	'I',
	"J",	'J',
	"K",	'K',
	"L",	'L',
	"M",	'M',
	"N",	'N',
	"O",	'O',
	"P",	'P',
	"Q",	'Q',
	"R",	'R',
	"S",	'S',
	"T",	'T',
	"U",	'U',
	"V",	'V',
	"W",	'W',
	"X",	'X',
	"Y",	'Y',
	"Z",	'Z',
	"bracketleft",	    '[',
	"backslash",	    '\\',
	"bracketright",     ']',
	"asciicircum",	    '^',
	"underscore",	    '_',
	"quoteleft",	    0x091,
	"a",	'a',
	"b",	'b',
	"c",	'c',
	"d",	'd',
	"e",	'e',
	"f",	'f',
	"g",	'g',
	"h",	'h',
	"i",	'i',
	"j",	'j',
	"k",	'k',
	"l",	'l',
	"m",	'm',
	"n",	'n',
	"o",	'o',
	"p",	'p',
	"q",	'q',
	"r",	'r',
	"s",	's',
	"t",	't',
	"u",	'u',
	"v",	'v',
	"w",	'w',
	"x",	'x',
	"y",	'y',
	"z",	'z',
	"braceleft",	    '{',
	"bar",		    0x07c,		/* sjp20Aug87: changed from '|' */
	"braceright",	    '}',
	"asciitilde",	    '~',
	"exclamdown",	    0x0a1,
	"cent", 	    0x0a2,
	"sterling",	    0x0a3,
	"fraction",	    0x0bc,
	"yen",		    0x0a5,
	"florin",	    0x83,
	"section",	    0x0a7,
	"currency",	    0x0a4,
	"quotesingle",	    0x027,
	"quotedblleft",     0x093,		/* 87-1-15 sec */
	"guillemotleft",    0x0ab,
	"guilsinglleft",    0x8B,
	"guilsinglright",   0x9B,
	"fi",		    0,
	"fl",		    0,
	"endash",	    0x096,			/* 87-1-15 sec */
	"dagger",	    0x86,
	"daggerdbl",	    0x87,
	"periodcentered",   0x0b7,
	"paragraph",	    0x0b6,
	"bullet",	    	0x095,		/* 87-1-15 sec (was 1) */
	"quotesinglbase",   0x82,
	"quotedblbase",     0x84,
	"quotedblright",    0x94,		/* 87-1-15 sec */
	"guillemotright",   0xbb,
	"ellipsis",	    0x85,
	"perthousand",	    0x89,
	"questiondown",     0x0bf,
	"grave",	    0x060,
	"acute",	    0x0b4,
	"circumflex",	    0,
	"tilde",	    0,
	"macron",	    0x0af,
	"breve",	    0,
	"dotaccent",	    0,
	"dieresis",	    0x0a8,
/* sjp21jul87	"ring", 	    0x0b0, */
	"cedilla",	    0x0b8,
	"hungarumlaut",     0,
	"ogonek",	    0,
	"caron",	    0,
	"emdash",	    0x097,			/* 87-1-15 sec */
	"AE",		    0x0c6,
	"ordfeminine",	    0x0aa,
	"Lslash",	    0,
	"Oslash",	    0x0d8,
	"OE",		    0x8c,
	"ordmasculine",     0x0ba,
	"ae",		    0x0e6,
	"dotlessi",	    0,
	"lslash",	    0,
	"oslash",	    0x0f8,
	"oe",		    0x9C,
	"germandbls",	    0x0df,
	"Aacute",	    0x0c1,
	"Acircumflex",	    0x0c2,
	"Adieresis",	    0x0c4,
	"Agrave",	    0x0c0,
	"Aring",	    0x0c5,
	"Atilde",	    0x0c3,
	"Ccedilla",	    0x0c7,
	"Eacute",	    0x0c9,
	"Ecircumflex",	    0x0ca,
	"Edieresis",	    0x0cb,
	"Egrave",	    0x0c8,
	"Iacute",	    0x0cd,
	"Icircumflex",	    0x0ce,
	"Idieresis",	    0x0cf,
	"Igrave",	    0x0cc,
	"Ntilde",	    0x0d1,
	"Oacute",	    0x0d3,
	"Ocircumflex",	    0x0d4,
	"Odieresis",	    0x0d6,
	"Ograve",	    0x0d2,
	"Otilde",	    0x0d5,
	"Scaron",	    0x8A,
	"Uacute",	    0x0da,
	"Ucircumflex",	    0x0db,
	"Udieresis",	    0x0dc,
	"Ugrave",	    0x0d9,
	"Ydieresis",	    0x9F,
	"Zcaron",	    0,
	"aacute",	    0x0e1,
	"acircumflex",	    0x0e2,
	"adieresis",	    0x0e4,
	"agrave",	    0x0e0,
	"aring",	    0x0e5,
	"atilde",	    0x0e3,
	"ccedilla",	    0x0e7,
	"copyright",	    0x0a9,
	"eacute",	    0x0e9,
	"ecircumflex",	    0x0ea,
	"edieresis",	    0x0eb,
	"egrave",	    0x0e8,
	"iacute",	    0x0ed,
	"icircumflex",	    0x0ee,
	"idieresis",	    0x0ef,
	"igrave",	    0x0ec,
	"logicalnot",	    0x0ac,
	"minus",	    0,
	"ntilde",	    0x0f1,
	"oacute",	    0x0f3,
	"ocircumflex",	    0x0f4,
	"odieresis",	    0x0f6,
	"ograve",	    0x0f2,
	"otilde",	    0x0f5,
	"registered",	    0x0ae,
	"scaron",	    0x9A,
	"trademark",	    0x99,
	"uacute",	    0x0fa,
	"ucircumflex",	    0x0fb,
	"udieresis",	    0x0fc,
	"ugrave",	    0x0f9,
	"ydieresis",	    0x0ff,
	"zcaron",	    0,
   "overstore",    0x0af,

/* 21jul87--sjp on request of Adobe */
	"brokenbar",		0xa6,		/* sjp20Aug87 */
	"logicalnot",		0xac,		/* sjp05Nov87 */
	"degree",			0xb0,
	"plusminus",		0xb1,
	"twosuperior",		0xb2,
	"threesuperior",	0xb3,
	"mu",				0xb5,
	"onesuperior",		0xb9,
	"onequarter",		0xbc,
	"onehalf",			0xbd,
	"threequarters",	0xbe,
	"Eth",				0xd0,
	"multiply",			0xd7,
	"Yacute",			0xdd,
	"Thorn",			0xde,
	"eth",				0xf0,
	"divide",			0xf7,
	"yacute",			0xfd,
	"thorn",			0xfe,
	NULL,	0
	};

    KEY *pkey;

    pkey = rgKeys;
    while (pkey->szKey)
	{
	if (szIsEqual(szWord, pkey->szKey))
	    return(pkey->iValue);
	++pkey;
	}
    printf("GetCharCode: Undefined character = %s\n", szWord);
#ifdef DBCS
    // tell the caller the name is not known and N is not
    // overriding what character code tells us.
    return -1;
#else // DBCS
    return(0);
#endif // DBCS
    }
