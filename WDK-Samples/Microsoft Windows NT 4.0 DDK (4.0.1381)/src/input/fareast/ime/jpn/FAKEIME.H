/**********************************************************************/
/*                                                                    */
/*      FAKEIME.H - Windows 95 FAKEIME                                */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/
/**********************************************************************/
/*                                                                    */
/*      Define                                                        */
/*                                                                    */
/**********************************************************************/

/* for limit of FAKEIME */
#define MAXCOMPWND              10
#define MAXCOMPSIZE             256
#define MAXCLAUSESIZE           16
#define MAXCANDPAGESIZE         9
#define MAXCANDSTRSIZE          16
#define MAXGLCHAR               32
#define MAXCANDSTRNUM           32


/* for GlobalAlloc */
#define GHIME (GHND | GMEM_SHARE)

/* special messages */
#define WM_UI_UPDATE      (WM_USER+500)
#define WM_UI_HIDE        (WM_USER+501)

#define WM_UI_STATEMOVE   (WM_USER+601)
#define WM_UI_DEFCOMPMOVE (WM_USER+602)
#define WM_UI_CANDMOVE    (WM_USER+603)
#define WM_UI_GUIDEMOVE   (WM_USER+604)

/* special style */
#define WS_COMPDEFAULT (WS_DISABLED | WS_POPUP)
#define WS_COMPNODEFAULT (WS_DISABLED | WS_POPUP)

/* macro for me! */
#define IsCTLPushed(x)          ((x)[VK_CONTROL] & 0x80)
#define IsSHFTPushed(x)         ((x)[VK_SHIFT] & 0x80)
#define IsALTPushed(x)          ((x)[VK_ALT] & 0x80)

/* update context */
#define UPDATE_MODE             0x01
#define UPDATE_SYSTEM           0x02
#define UPDATE_TEXT             0x04
#define UPDATE_FORCE            0x08
#define UPDATE_ALL              ( UPDATE_MODE | UPDATE_SYSTEM | UPDATE_TEXT )

/* advise context */
#define ADVISE_MODE             0x0001  /* advise about Mode requested   */
#define ADVISE_ISOPEN           0x0002  /* advise about IsOpen requested */

/* key state context */
#define KS_SHIFT                0x01
#define KS_CONTROL              0x02

/* Property Sheet DLG ID */
#define DLG_GENERAL             100
#define DLG_REGISTERWORD        101
#define DLG_SELECTDICTIONARY    102
#define DLG_ABOUT               103

/* for dialog box */
#define ID_WR_READING           200
#define ID_WR_STRING            201
#define ID_WR_STYLE             202

#define ID_SD_SELDIC            301
#define ID_SD_DICPATH           302

/* string tables */
#define IDS_DICFILENAME         1000
#define IDS_CONFIGNAME          1001
#define IDS_GL_NODICTIONARY     1010
#define IDS_GL_TYPINGERROR      1011
#define IDS_NOREADING           1020
#define IDS_NOSTRING            1021
#define IDS_NOMEMORY            1022
#define IDS_REGWORDRET          1023


/* ID of guideline table */
#define MYGL_NODICTIONARY     0
#define MYGL_TYPINGERROR      1

/* Change Mode index */
#define TO_CMODE_ALPHANUMERIC  0x0001
#define TO_CMODE_KATAKANA      0x0002
#define TO_CMODE_HIRAGANA      0x0003
#define TO_CMODE_FULLSHAPE     0x0008
#define TO_CMODE_ROMAN         0x0010
#define TO_CMODE_CHARCODE     0x0020
#define TO_CMODE_TOOLBAR       0x0100

/* WndExtra of child UI windows */
#define FIGWL_MOUSE         4
#define FIGWL_SVRWND        8
#define FIGWL_FONT         12
#define FIGWL_COMPSTARTSTR 16
#define FIGWL_COMPSTARTNUM 20
#define FIGWL_STATUSBMP    24
#define FIGWL_CLOSEBMP     28
#define FIGWL_PUSHSTATUS   32
#define FIGWL_CHILDWND     36
#define UIEXTRASIZE        40

/* The flags of FIGWL_MOUSE */
#define FIM_CAPUTURED 0x01
#define FIM_MOVED     0x02

/* The flags of the button of Status Window */
#define PUSHED_STATUS_HDR   0x01
#define PUSHED_STATUS_MODE  0x02
#define PUSHED_STATUS_ROMAN 0x04
#define PUSHED_STATUS_CLOSE 0x08

/* Status Button Pos */
#define BTX       20
#define BTY       20
#define BTFHIRA   20
#define BTFKATA   40
#define BTFALPH   60
#define BTHKATA   80
#define BTHALPH  100
#define BTROMA   120
#define BTEMPT   140

/* Statue Close Button */
#define STCLBT_X   (BTX*2+3)
#define STCLBT_Y   1
#define STCLBT_DX  12
#define STCLBT_DY  12

/* define Shift Arrow right-left */
#define ARR_RIGHT 1
#define ARR_LEFT  2

/* Init or Clear Structure Flag */
#define CLR_RESULT 1
#define CLR_UNDET  2
#define CLR_RESULT_AND_UNDET  3


/* define GET LP for COMPOSITIONSTRING members. */
#define GETLPCOMPREADATTR(lpcs) ((PBYTE)((PBYTE)(lpcs) + (lpcs)->dwCompReadAttrOffset))
#define GETLPCOMPREADCLAUSE(lpcs) ((PDWORD)((PBYTE)(lpcs) + (lpcs)->dwCompReadClauseOffset))
#define GETLPCOMPREADSTR(lpcs) ((PTSTR)((PBYTE)(lpcs) + (lpcs)->dwCompReadStrOffset))
#define GETLPCOMPATTR(lpcs) ((PBYTE)((PBYTE)(lpcs) + (lpcs)->dwCompAttrOffset))
#define GETLPCOMPCLAUSE(lpcs) ((PDWORD)((PBYTE)(lpcs) + (lpcs)->dwCompClauseOffset))
#define GETLPCOMPSTR(lpcs) ((PTSTR)((PBYTE)(lpcs) + (lpcs)->dwCompStrOffset))
#define GETLPRESULTREADCLAUSE(lpcs) ((PDWORD)((PBYTE)(lpcs) + (lpcs)->dwResultReadClauseOffset))
#define GETLPRESULTREADSTR(lpcs) ((PTSTR)((PBYTE)(lpcs) + (lpcs)->dwResultReadStrOffset))
#define GETLPRESULTCLAUSE(lpcs) ((PDWORD)((PBYTE)(lpcs) + (lpcs)->dwResultClauseOffset))
#define GETLPRESULTSTR(lpcs) ((PTSTR)((PBYTE)(lpcs) + (lpcs)->dwResultStrOffset))


#define SetClause(lpdw,num)   {*((LPDWORD)(lpdw)) = 0;*((LPDWORD)(lpdw)+1) = num;}

#define GCS_COMPALL ( GCS_COMPSTR | GCS_COMPATTR | GCS_COMPREADSTR | GCS_COMPREADATTR | GCS_COMPCLAUSE | GCS_COMPREADCLAUSE)
#define GCS_RESULTALL ( GCS_RESULTSTR | GCS_RESULTREADSTR | GCS_RESULTCLAUSE | GCS_RESULTREADCLAUSE)

#define JAPANESE_CODEPAGE 932
#define JAPANESE_LOCALE   0x411

/**********************************************************************/
/*                                                                    */
/*      Structures                                                    */
/*                                                                    */
/**********************************************************************/
typedef struct _tagGENEMSG{
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
} GENEMSG, NEAR *PGENEMSG, FAR *LPGENEMSG;

typedef struct _tagTRANSKEY{
    DWORD dwSize;
    GENEMSG gmMSG[1];
} TRABSKEY, NEAR *PTRABSKEY, FAR *LPTRABSKEY;

typedef struct _tagMYCOMPSTR{
    COMPOSITIONSTRING cs;
    TCHAR          szCompReadStr[MAXCOMPSIZE];
    BYTE           bCompReadAttr[MAXCOMPSIZE];
    DWORD          dwCompReadClause[MAXCLAUSESIZE];
    TCHAR          szCompStr[MAXCOMPSIZE];
    BYTE           bCompAttr[MAXCOMPSIZE];
    DWORD          dwCompClause[MAXCLAUSESIZE];
    TCHAR          szTypeInfo[MAXCOMPSIZE];
    TCHAR          szResultReadStr[MAXCOMPSIZE];
    DWORD          dwResultReadClause[MAXCOMPSIZE];
    TCHAR          szResultStr[MAXCOMPSIZE];
    DWORD          dwResultClause[MAXCOMPSIZE];
} MYCOMPSTR, NEAR *PMYCOMPSTR, FAR *LPMYCOMPSTR;

typedef struct _tagMYCAND{
    CANDIDATEINFO  ci;
    CANDIDATELIST  cl;
    DWORD          offset[MAXCANDSTRNUM];
    TCHAR          szCand[MAXCANDSTRNUM][MAXCANDSTRSIZE];
} MYCAND, NEAR *PMYCAND, FAR *LPMYCAND;


typedef struct _tagUICHILD{
    HWND    hWnd;
    BOOL    bShow;
    POINT   pt;
} UICHILD, NEAR *PUICHILD, FAR *LPUICHILD;

typedef struct _tagUICHILD2{
    HWND    hWnd;
    BOOL    bShow;
    RECT    rc;
} UICHILD2, NEAR *PUICHILD2, FAR *LPUICHILD2;

typedef struct _tagUIEXTRA{
    HIMC     hIMC;
    UICHILD  uiStatus;
    UICHILD  uiCand;
    DWORD    dwCompStyle;
    HFONT    hFont;
    BOOL     bVertical;
    UICHILD  uiDefComp;
    UICHILD2 uiComp[MAXCOMPWND];
    UICHILD  uiGuide;
} UIEXTRA, NEAR *PUIEXTRA, FAR *LPUIEXTRA;

typedef struct _tagMYGUIDELINE{
    DWORD dwLevel;
    DWORD dwIndex;
    DWORD dwStrID;
} MYGUIDELINE, NEAR *PMYGUIDELINE, FAR *LPMYGUIDELINE;

/**********************************************************************/
/*                                                                    */
/*      Externs                                                       */
/*                                                                    */
/**********************************************************************/
#ifndef _NO_EXTERN_
extern HINSTANCE  hInst;
extern HANDLE hMutex;
extern LPDWORD lpdwCurTransKey;
extern UINT uNumTransKey;
extern BOOL fOverTransKey;
extern TCHAR szUIClassName[];
extern TCHAR szCompStrClassName[];
extern TCHAR szCandClassName[];
extern TCHAR szStatusClassName[];
extern TCHAR szGuideClassName[];
extern MYGUIDELINE glTable[];
extern TCHAR szDicFileName[];
extern BYTE bComp[];
extern BYTE bCompCtl[];
extern BYTE bCompSht[];
extern BYTE bCompAlt[];
extern BYTE bNoComp[];
extern BYTE bNoCompCtl[];
extern BYTE bNoCompSht[];
extern BYTE bNoCompAlt[];
#endif //_NO_EXTERN_

/**********************************************************************/
/*                                                                    */
/*      Functions                                                     */
/*                                                                    */
/**********************************************************************/
/*   fakeime.c     */
int PASCAL Init(void);

/*   subs.c     */
void PASCAL InitCompStr(LPCOMPOSITIONSTRING pComp,DWORD dwClrFlag);
void PASCAL ClearCompStr(LPCOMPOSITIONSTRING pComp,DWORD dwClrFlag);
void PASCAL ClearCandidate(LPCANDIDATEINFO lpCandInfo);
void PASCAL ChangeMode(HIMC hIMC,DWORD dwToMode);
void PASCAL ChangeCompStr(HIMC hIMC,DWORD dwToMode);
BOOL PASCAL IsCompStr(HIMC hIMC);
BOOL PASCAL IsConvertedCompStr(HIMC hIMC);
BOOL PASCAL IsCandidate(LPINPUTCONTEXT lpIMC);
void PASCAL lmemset(PBYTE,BYTE,UINT);

/*   toascii.c   */
BOOL PASCAL GenerateMessageToTransKey(LPDWORD lpdwTrabsKey,LPGENEMSG lpGeneMsg);
BOOL PASCAL GenerateOverFlowMessage(LPDWORD lpdwTransKey);
BOOL PASCAL IMEKeydownHandler(HIMC,WPARAM, LPARAM, LPBYTE);
BOOL PASCAL IMEKeyupHandler(HIMC,WPARAM,LPARAM,LPBYTE);

/*   notify.c    */
BOOL PASCAL NotifyUCSetOpen(HIMC hIMC);
BOOL PASCAL NotifyUCConversionMode(HIMC hIMC);
BOOL PASCAL NotifyUCSetCompositionWindow(HIMC hIMC);


/*   ui.c        */
BOOL IMERegisterClass(HANDLE hInstance);
LRESULT CALLBACK FAKEIMEWndProc(HWND,UINT,WPARAM,LPARAM);
LONG PASCAL NotifyCommand(HIMC hUICurIMC, HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
LONG PASCAL ControlCommand(HIMC hUICurIMC, HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
void PASCAL DrawUIBorder( LPRECT lprc );
void PASCAL DragUI(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/*   uistate.c   */
LRESULT CALLBACK StatusWndProc(HWND,UINT,WPARAM,LPARAM);
void PASCAL PaintStatus(HWND hStatusWnd , HDC hDC, LPPOINT lppt,DWORD dwPushedStatus);
void PASCAL ButtonStatus(HWND hWnd, UINT message,WPARAM wParam,LPARAM lParam);
BOOL PASCAL MyIsIMEMessage(UINT message);
void PASCAL UpdateStatusWindow(LPUIEXTRA lpUIExtra);

/* uicand.c      */
LRESULT CALLBACK CandWndProc(HWND,UINT,WPARAM,LPARAM);
void PASCAL PaintCandWindow(HWND hCandWnd);
BOOL PASCAL GetCandPosFromComp(LPINPUTCONTEXT lpIMC, LPUIEXTRA lpUIExtra,LPPOINT lppt);
void PASCAL CreateCandWindow(HWND hUIWnd,LPUIEXTRA lpUIExtra, LPINPUTCONTEXT lpIMC);
void PASCAL ResizeCandWindow(LPUIEXTRA lpUIExtra,LPINPUTCONTEXT lpIMC);
void PASCAL HideCandWindow( LPUIEXTRA lpUIExtra);
void PASCAL MoveCandWindow(HWND hUIWnd, LPINPUTCONTEXT lpIMC, LPUIEXTRA lpUIExtra, BOOL fForceComp);

/* uicomp.c      */
LRESULT CALLBACK CompStrWndProc(HWND,UINT,WPARAM,LPARAM);
void PASCAL PaintCompWindow(HWND hCompWnd);
void PASCAL CreateCompWindow(HWND hUIWnd, LPUIEXTRA lpUIExtra,LPINPUTCONTEXT lpIMC);
void PASCAL MoveCompWindow(LPUIEXTRA lpUIExtra,LPINPUTCONTEXT lpIMC);
void PASCAL HideCompWindow(LPUIEXTRA lpUIExtra);
void PASCAL SetFontCompWindow(LPUIEXTRA lpUIExtra);

/*   uiguide.c   */
LRESULT CALLBACK GuideWndProc(HWND,UINT,WPARAM,LPARAM);
void PASCAL PaintGuide(HWND hGuideWnd , HDC hDC, LPPOINT lppt,DWORD dwPushedGuide);
void PASCAL ButtonGuide(HWND hWnd, UINT message,WPARAM wParam,LPARAM lParam);
void PASCAL UpdateGuideWindow(LPUIEXTRA lpUIExtra);
LRESULT CALLBACK LineWndProc(HWND,UINT,WPARAM,LPARAM);

/* config.c      */ 
BOOL CALLBACK RegWordDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SelectDictionaryDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GeneralDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam);

/* DIC.C         */ 
BOOL PASCAL IsEat(WORD);
BOOL PASCAL DicKeydownHandler(HIMC,UINT,TCHAR,LPBYTE);
void PASCAL DeleteChar( HIMC hIMC ,UINT uVKey);
void PASCAL FlushText();
void PASCAL RevertText(HIMC hIMC);
void PASCAL AddChar(HIMC, TCHAR, UINT );
BOOL PASCAL ConvKanji(HIMC);
BOOL WINAPI MakeResultString(HIMC,BOOL);
BOOL PASCAL MakeGuideLine(HIMC, DWORD);
BOOL PASCAL GenerateMessage(HIMC,LPINPUTCONTEXT,LPDWORD,LPGENEMSG);
BOOL PASCAL CheckAttr( LPCOMPOSITIONSTRING pComp);
void PASCAL MakeAttrClause( LPCOMPOSITIONSTRING pComp);
INT MapCompositionString(PTSTR pCompStr, INT iCursorPos, DWORD dwConv);
BOOL ZenToHan(LPTSTR, INT, LPTSTR);
BOOL HanToZen(LPTSTR, INT, LPTSTR,DWORD);
INT MapString(PTSTR pDst, INT cDst, PTSTR pSrc, DWORD dwFlag, INT iCursorPos );
VOID UpdateCompositionString( HIMC );

/* romkan.c */
BOOL RK_OpenDefFile();
INT RK_Convert( PTSTR pDst, INT cDst, PTSTR pSrc, DWORD dwCursorPos );
