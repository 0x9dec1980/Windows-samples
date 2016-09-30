
#define  IDM_OPEN   100
#define  IDM_CLOSE  101
#define  IDM_ABOUT  102
#define  IDM_SEND   103
#define  IDM_READ   104
#define  IDM_RESET  105

#define  IDM_NO_FILTER      110
#define  IDM_DIRECTED       111
#define  IDM_MULTICAST      112
#define  IDM_BROADCAST      113
#define  IDM_ALL_MULTICAST  114
#define  IDM_PROMISCUOUS    115

#define  IDM_FIRST_DLL      200



#define WM_DUMPCHANGE         0x8001

#define MAX_DLLS              16
#define MAX_FOUND             16

#define TEXT_BUFFER_LENGTH    128


typedef enum _INTERFACE_TYPE {
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    MaximumInterfaceType
}INTERFACE_TYPE, *PINTERFACE_TYPE;





//
// This action routine is called when a handling a NcDetectIdentify.
//
typedef
LONG
(*NC_DETECT_IDENTIFY)(
    IN LONG Index,
    IN WCHAR * Buffer,
    IN LONG BuffSize
    );

//
// This action routine is called when a handling a NcDetectFirstNext.
//
typedef
LONG
(*NC_DETECT_FIRST_NEXT)(
    IN  LONG NetcardId,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN  BOOL First,
    OUT PVOID *pvToken,
    OUT LONG *Confidence
    );

//
// This action routine is called when a handling a NcDetectOpenHandle.
//
typedef
LONG
(*NC_DETECT_OPEN_HANDLE)(
    IN  PVOID Token,
    OUT PVOID *Handle
    );

//
// This action routine is called when a handling a NcDetectCreateHandle.
//
typedef
LONG
(*NC_DETECT_CREATE_HANDLE)(
    IN  LONG NetcardId,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    OUT PVOID *Handle
    );

//
// This action routine is called when a handling a NcDetectCloseHandle.
//
typedef
LONG
(*NC_DETECT_CLOSE_HANDLE)(
    IN PVOID Handle
    );

//
// This action routine is called when a handling a NcDetectQueryCfg.
//
typedef
LONG
(*NC_DETECT_QUERY_CFG)(
    IN  PVOID Handle,
    OUT WCHAR *Buffer,
    IN  LONG BuffSize
    );

//
// This action routine is called when a handling a NcDetectVerifyCfg.
//
typedef
LONG
(*NC_DETECT_VERIFY_CFG)(
    IN PVOID Handle,
    IN WCHAR *Buffer
    );

//
// This action routine is called when a handling a NcDetectQueryMask.
//
typedef
LONG
(*NC_DETECT_QUERY_MASK)(
    IN  LONG NetcardId,
    OUT WCHAR *Buffer,
    IN  LONG BuffSize
    );

//
// This action routine is called when a handling a NcDetectParamRange.
//
typedef
LONG
(*NC_DETECT_PARAM_RANGE)(
    IN  LONG NetcardId,
    IN  WCHAR *Param,
    OUT LONG *Values,
    OUT LONG *BuffSize
    );

//
// This action routine is called when a handling a NcDetectQueryParameterName.
//
typedef
LONG
(*NC_DETECT_QUERY_PARAMETER_NAME)(
    IN  WCHAR *Param,
    OUT WCHAR *Buffer,
    IN  LONG  BufferSize
    );




typedef struct _DETDLL {

    LPTSTR           pLibName;
    HINSTANCE        hLib;

    NC_DETECT_IDENTIFY      IdentifyProc;
    NC_DETECT_QUERY_MASK    QueryMaskProc;
    NC_DETECT_FIRST_NEXT    FirstNextProc;
    NC_DETECT_QUERY_CFG     QueryProc;
    NC_DETECT_OPEN_HANDLE   OpenProc;
    NC_DETECT_CLOSE_HANDLE  CloseProc;
    NC_DETECT_CREATE_HANDLE CreateProc;
    NC_DETECT_PARAM_RANGE   RangeProc;
    NC_DETECT_VERIFY_CFG    VerifyProc;

    } DETDLL, * LPDETDLL;

typedef struct _DETECT {

    SC_HANDLE   schSCManager;
    SC_HANDLE   ServiceHandle;


    DETDLL         DllList[MAX_DLLS];
    ULONG          NumberOfDlls;

    HMENU          hMenuBar;
    HMENU          hMenuPopup1;
    HMENU          hMenuPopup2;

    INTERFACE_TYPE BusType;

    UINT           CurrentDll;

    UINT           CurrentIndex;

    UINT           AdaptersFound;
    UINT           CurrentFoundAdapter;
    PVOID          FoundTokens[MAX_FOUND];

    TCHAR          FoundConfig[MAX_FOUND][128];

    } DETECT, *LPDETECT;





BOOL
CallUpDlgBox(
    HWND hWnd,
    FARPROC DlgBoxProc,
    LPTSTR Template,
    LPARAM lParam
    );



BOOLEAN
GetNetDetectDlls(
    LPDETECT   Detect
    );

ULONG
ReadRegistry(
    PTSTR   pStr,
    PULONG  BufferSize
    );

BOOLEAN
StartNetDetectDriver(
    LPDETECT  Detect
    );


LRESULT
CreationHandler(
    HWND    hWnd
    );

BOOLEAN
AdapterListBoxHandler(
    LPDETECT  Detect,
    HWND      hDlg,
    UINT      Notification
    );

BOOLEAN
FoundListBoxHandler(
    LPDETECT  Detect,
    HWND      hDlg,
    UINT      Notification
    );

BOOLEAN
FindAdapterHandler(
    LPDETECT  Detect,
    HWND      hDlg
    );

BOOLEAN
CreateAdapterHandler(
    LPDETECT  Detect,
    HWND      hDlg
    );

BOOLEAN
RangeListBoxHandler(
    LPDETECT  Detect,
    HWND      hDlg,
    UINT      Notification
    );


BOOL FAR PASCAL
DetectBoxProc(
    HWND hdlg,
    UINT message,
    WPARAM wParam,
    LONG lParam
    );

BOOL FAR PASCAL
AboutBoxProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LONG lParam
    );

VOID
InitWideBuffer(
    PTCHAR    Buffer,
    UINT      Length
    );
