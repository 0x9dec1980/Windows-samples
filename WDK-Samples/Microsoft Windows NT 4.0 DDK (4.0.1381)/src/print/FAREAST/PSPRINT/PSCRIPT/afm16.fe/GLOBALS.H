/*
 * the order of the gSourceList[], gPaperList[] and gPaperSizes[]
 * coorispond with the constants defined in DRIVINIT.H for the
 * common device initilization.  these orders should not be changed
 * without consulting those values.
 */

#ifdef APDGLOBALS

TOKEN gTokenList[] = {
        "Product",              APD_PRODUCT,
        "DefaultResolution",    APD_DEFAULTRESOLUTION,
        "DefaultTransfer",      APD_DEFAULTTRANSFER,
        "Transfer",             APD_TRANSFER,
        "DefaultPageSize",      APD_DEFAULTPAGESIZE,
        "PageSize",             APD_PAGESIZE,
        "DefaultPageRegion",    APD_DEFAULTPAGEREGION,
        "PageRegion",           APD_PAGEREGION,
        "DefaultPaperTray",     APD_DEFAULTPAPERTRAY,
        "PaperTray",            APD_PAPERTRAY,
        "DefaultImageableArea", APD_DEFAULTIMAGEABLEAREA,
        "ImageableArea",        APD_IMAGEABLEAREA,
        "DefaultInputSlot",     APD_DEFAULTINPUTSLOT,
        "InputSlot",            APD_INPUTSLOT,
        "DefaultManualFeed",    APD_DEFAULTMANUALFEED,
        "ManualFeed",           APD_MANUALFEED,
        "PrinterName",          APD_PRINTERNAME,
        "WindowsAuto",          APD_WINDOWSAUTO,
        "DefaultPaperStock",    APD_DEFAULTPAPERSTOCK,
        "End",                  APD_END,
        "EndOfFile",            APD_ENDOFFILE,
        "Transverse",           APD_TRANSVERSE,
        "ColorDevice",          APD_COLOR,
        NULL, 0
};

/* basically used by the error handler */
char *gFileName;

char gCapsFile[]="printcap.h";

int gLineNum;
BOOL gReUseLineFlag;

char *gAPDFileList[MAX_APDS];
char *gPrinterList[MAX_APDS];
char *gPrCapsFileList[MAX_APDS];
char *gPSSFileList[MAX_APDS];

/* these coorispond with the DMBIN_ constants in DRIVINIT.H */

char *gSourceList[]={
        "Upper",                /* DMBIN_UPPER Default */
        "ManualFeed",           /* DMBIN_UPPER */
        "Upper",                /* DMBIN_MANUAL */
        "Lower",                /* DMBIN_LOWER */
        "Middle",               /* DMBIN_MIDDLE */
        "Envelope",             /* DMBIN_ENVELOPE */
        "EnvelopeManual",       /* DMBIN_ENVMANUAL place holder */
        "AutoSelect",           /* DMBIN_AUTO */
        "Tractor",              /* DMBIN_TRACTOR place holder */
        "SmallFormat",          /* DMBIN_SMALLFMT       */
        "LargeFormat",          /* DMBIN_LARGEFMT       */
        "LargeCapacity",        /* DMBIN_LARGECAPACITY  */
        "AnySmallFormat",       /* DMBIN_ANYSMALLFMT new */
        "AnyLargeFormat",       /* DMBIN_ANYLARGEFMT new */
        "PaperCassette",        /* DMBIN_CASSETTE    new */
        ""
};

/* these coorispond with the DMPAPER_ constants in DRIVINIT.H */

FPOINT gPaperSizes[]={
        {612.0,792.0},          /* Default */
        {612.0,792.0},          /* Letter */
        {612.0,792.0},          /* LetterSmall */
        {792.0,1224.0},         /* Tabloid */
        {1224.0,792.0},         /* Ledger */
        {612.0,1008.0},         /* Legal */
        {396.0,612.0},          /* Statement */
        {522.0,756.0},          /* Executive */
        {841.88973,1190.5511},  /* A3 */
        {595.27558,841.88973},  /* A4 */
        {595.27558,841.88973},  /* A4Small */
        {419.52755,595.27558},  /* A5 */
        {708.66142,1002.0472},  /* B4 */
        {501.02362,708.66142},  /* B5 */
        {612.0,936.0},          /* Folio */
        {609.44881,779.52751},  /* Quarto */
        {720.0,1008.0},         /* 10x14 */
        {792.0,1224.0},         /* 11x17 */
        {540.0,279.0},          /* Envelope 7 3/4 */
        {639.0,279.0},          /* Envelope 9 */
        {684.0,297.0},          /* Envelope 10 */
        {747.0,324.0},          /* Envelope 11 */
        {623.622,311.811},      /* Envelope DL */
        {649.134,323.150},      /* Envelope C5/C6 */
        {0.0,0.0}
};

/* these coorispond with the DMPAPER_ constants in DRIVINIT.H */

char *gPaperList[]={
        "Letter",               /* Default */
        "Letter",
        "Letter small",
        "Tabloid",
        "Ledger",
        "Legal",
        "Statement",
        "Executive",
        "A3",
        "A4",
        "A4 small",
        "A5",
        "B4",
        "B5",
        "Folio",
        "Quarto",
        "10x14",
        "11x17",
        "Env. (7.5 x 3.875 in)",
        "Env. (8.875 x 3.875 in)",
        "Env. (9.5 x 4.125 in)",
        "Env. (10.375 x 4.5 in)",
        "Env. (220 x 110 mm)",
        "Env. (229 x 114 mm)",
        ""
};

char *gLogicalList[]={
        "False",
        "True"
};

char *gTransferList[]={
        "Null",
        "Normalized"
};

/* these are set for each printer being processed */
BOOL *gPaperCaps;
BOOL *gSourceCaps;
RECT *gMarginCaps;
int gDefSourceCaps;
int gDefResolutionCaps;
int gDefJobTimeoutCaps;
BOOL gEndOfFileCaps;
BOOL gColor;

/* these are use for ALL printers if something is missing in APD */
int gDefSource=DEFAULTSOURCE;
int gDefResolution=DEFAULTRESOLUTION;
int gDefJobTimeout=DEFAULTJOBTIMEOUT;
BOOL gDefEndOfFile=TRUE;
BOOL gDefColor = FALSE;

int gDefPrinter;                /* Probably used for header file */
int gDefPaper;                  /* Perhaps used for header file? */

int gNumPapers;
int gNumSources;
int gNumPrinters;

char **gNormalImagePSSList;
char **gManualImagePSSList;
char **gManualSwitchPSSList;
char **gInputSlotPSSList;
char *gTransferPSS;
char *gWindowsAutoPSS;

BOOL gNormalImageFlag;
BOOL gManualImageFlag;
BOOL gManualSwitchFlag;
BOOL gInputSlotFlag;
BOOL gTransferFlag;
BOOL gWindowsAutoFlag;


#else

extern TOKEN gTokenList[];

char *gFileName;

extern char *gCapsFile;

extern int gLineNum;
extern BOOL gReUseLineFlag;

extern char *gAPDFileList[];
extern char *gPrinterList[];
extern char *gPrCapsFileList[];
extern char *gPSSFileList[];
extern char *gPaperList[];
extern char *gSourceList[];
extern FPOINT gPaperSizes[];
extern char *gLogicalList[];
extern char *gTransferList[];

extern BOOL *gPaperCaps;
extern BOOL *gSourceCaps;
extern RECT *gMarginCaps;
extern int gDefSourceCaps;
extern int gDefResolutionCaps;
extern int gDefJobTimeoutCaps;
extern BOOL gEndOfFileCaps;
extern BOOL gColor;

extern int gDefSource;
extern int gDefResolution;
extern int gDefJobTimeout;
extern BOOL gDefEndOfFile;
extern BOOL gDefColor;

extern int gDefPrinter;
extern int gDefPaper;

extern int gNumPapers;
extern int gNumSources;
extern int gNumPrinters;

extern char **gNormalImagePSSList;
extern char **gManualImagePSSList;
extern char **gManualSwitchPSSList;
extern char **gInputSlotPSSList;
extern char *gTransferPSS;
extern char *gWindowsAutoPSS;

extern BOOL gNormalImageFlag;
extern BOOL gManualImageFlag;
extern BOOL gManualSwitchFlag;
extern BOOL gInputSlotFlag;
extern BOOL gTransferFlag;
extern BOOL gWindowsAutoFlag;

#endif

