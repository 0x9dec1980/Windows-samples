#define IDM_ABOUT  100
#define IDM_ACTION 101

int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
void SizeWindow (short *, short *);
long FAR PASCAL MainWndProc(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL About(HWND, unsigned, WORD, LONG);


#define  vdev_addr   0200h
#define  cvdev_addr  0x0200
