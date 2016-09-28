
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


typedef struct _CONTROL_BLOCK {
    PVOID       hFile;
    HANDLE      hEvent;
    TCHAR       AdapterName[64];

    HANDLE      hMem;
    LPBYTE      lpMem;

    HGLOBAL     hMem2;
    LPBYTE      lpMem2;
    ULONG       PacketLength;

    ULONG       LastReadSize;
    UINT        BufferSize;
    } CONTROL_BLOCK, *PCONTROL_BLOCK;


#define WM_DUMPCHANGE         0x8001




LRESULT FAR PASCAL ChildWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
