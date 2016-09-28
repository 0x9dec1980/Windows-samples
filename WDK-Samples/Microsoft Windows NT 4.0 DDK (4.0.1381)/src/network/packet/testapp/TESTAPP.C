/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    packet32.c

Abstract:


Author:


Environment:

    User mode only.

Notes:


Future:



Revision History:

--*/
#define UNICODE 1

#include <windows.h>
#include <windowsx.h>

#include "packet32.h"

#include "hellowin.h"
#include "ntddndis.h"


ULONG   Filters[6]={0,
                    NDIS_PACKET_TYPE_DIRECTED,
                    NDIS_PACKET_TYPE_MULTICAST,
                    NDIS_PACKET_TYPE_BROADCAST,
                    NDIS_PACKET_TYPE_ALL_MULTICAST,
                    NDIS_PACKET_TYPE_PROMISCUOUS};

TCHAR   szbuff[40];





HWND    hwndchild;

HINSTANCE  hInst;

CONTROL_BLOCK Adapter;

TCHAR   szChildAppName[]=TEXT("hexdump");

UINT    showdump=0;

LRESULT FAR PASCAL WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);





LRESULT
HandleCommands(
    HWND     hWnd,
    UINT     uMsg,
    UINT     wParam,
    LONG     lParam
    );



int PASCAL WinMain (HINSTANCE hInstance,
		    HINSTANCE hPrevInstance,
                    LPSTR     lpszCmdParam,
                    int       nCmdShow)




  {
    static    TCHAR szAppName[]=TEXT("HelloWin");
    HWND      hwnd;
    MSG       msg;
    WNDCLASS  wndclass;

    hInst=hInstance;

    if (!hPrevInstance)
       {
         wndclass.style        =  CS_HREDRAW | CS_VREDRAW;
         wndclass.lpfnWndProc  =  WndProc;
         wndclass.cbClsExtra   =  0;
         wndclass.cbWndExtra   =  0;
         wndclass.hInstance    =  hInstance;
         wndclass.hIcon        =  LoadIcon (NULL, IDI_APPLICATION);
         wndclass.hCursor      =  LoadCursor(NULL, IDC_ARROW);
         wndclass.hbrBackground=  GetStockObject(WHITE_BRUSH);
	 wndclass.lpszMenuName =  TEXT("GenericMenu");
	 wndclass.lpszClassName=  szAppName;

         RegisterClass(&wndclass);
       }


    if (!hPrevInstance)
       {
         wndclass.style        =  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS |
                                  CS_BYTEALIGNCLIENT;
         wndclass.lpfnWndProc  =  ChildWndProc;
         wndclass.cbClsExtra   =  0;
         wndclass.cbWndExtra   =  0;
         wndclass.hInstance    =  hInstance;
         wndclass.hIcon        =  LoadIcon (hInstance,MAKEINTRESOURCE(1000));
         wndclass.hCursor      =  LoadCursor(NULL, IDC_ARROW);
         wndclass.hbrBackground=  GetStockObject(WHITE_BRUSH);
	 wndclass.lpszMenuName =  NULL;
	 wndclass.lpszClassName=  szChildAppName;

         RegisterClass(&wndclass);

       }



    hwnd = CreateWindow (szAppName,
                         TEXT("the hello world program"),
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);

    ShowWindow (hwnd,nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage (&msg, NULL, 0,0))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

    return (msg.wParam);
  }

LRESULT FAR PASCAL WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)

  {
    HDC       hdc;
    PAINTSTRUCT  ps;
    RECT         rect;


    switch (message)
      {
        case WM_COMMAND:
            HandleCommands(hwnd, message, wParam, lParam);
            return 0;

        case WM_LBUTTONDOWN:
          if (!showdump) {
               hwndchild = CreateWindow (szChildAppName,
                               TEXT("Hex Dump"),
                               WS_CHILD | WS_CAPTION | WS_SYSMENU | WS_VSCROLL |
                               WS_VISIBLE |  WS_THICKFRAME | WS_CLIPSIBLINGS,
                               0,0,
                               300,100,
                               hwnd,
                               (HMENU) 1,
                               hInst,
                               NULL);
          } else {

              SendMessage(hwndchild,WM_DUMPCHANGE,0,0l);
          }

          showdump=1;
          return 0;



        case WM_CREATE:


          {
              ULONG NameLength=64;

              PacketGetAdapterNames(
                  Adapter.AdapterName,
                  &NameLength
                  );

          }

          Adapter.BufferSize=1514;

          Adapter.hMem=GlobalAlloc(GMEM_MOVEABLE,1514);

          Adapter.lpMem=GlobalLock(Adapter.hMem);

          Adapter.hMem2=GlobalAlloc(GMEM_MOVEABLE,1514);

          Adapter.lpMem2=GlobalLock(Adapter.hMem2);


          return 0;



        case WM_KEYDOWN:



          return 0;

	case WM_PAINT:
          hdc=BeginPaint(hwnd,&ps);
          GetClientRect (hwnd,&rect);

	  EndPaint(hwnd,&ps);
          return 0;


	case WM_DESTROY:
          PostQuitMessage(0);
          return 0;
      }
    return DefWindowProc(hwnd,message, wParam, lParam);
  }





LRESULT
HandleCommands(
    HWND     hWnd,
    UINT     uMsg,
    UINT     wParam,
    LONG     lParam
    )

{
static ULONG    Filter=0;

    PVOID    Packet;


    UINT     i;



    switch (wParam) {

        case IDM_OPEN:

            Adapter.hFile=PacketOpenAdapter(Adapter.AdapterName);

            if (Adapter.hFile != NULL) {

                SetWindowText(hWnd,Adapter.AdapterName);

            }


            return TRUE;


        case IDM_CLOSE:

            PacketCloseAdapter(Adapter.hFile);

            return TRUE;


        case  IDM_NO_FILTER:

        case  IDM_DIRECTED:

        case  IDM_MULTICAST:

        case  IDM_BROADCAST:

        case  IDM_ALL_MULTICAST:

        case  IDM_PROMISCUOUS:


            Filter=Filters[wParam-IDM_NO_FILTER];

            PacketSetFilter(
                Adapter.hFile,
                Filter
                );


            return TRUE;

        case IDM_RESET:

            PacketResetAdapter(
                Adapter.hFile
                );



            return TRUE;



        case IDM_READ:

          if ((Filter == NDIS_PACKET_TYPE_PROMISCUOUS)
             ||
              (Filter == NDIS_PACKET_TYPE_BROADCAST)) {

              Packet=PacketAllocatePacket(Adapter.hFile);

              if (Packet != NULL) {

                  PacketInitPacket(
                      Packet,
                      Adapter.lpMem,
                      1514
                      );


                  PacketReceivePacket(
                      Adapter.hFile,
                      Packet,
                      TRUE,
                      &Adapter.PacketLength
                      );

                  PacketFreePacket(Packet);
              }
          }

          return TRUE;


        case IDM_SEND:

          Packet=PacketAllocatePacket(Adapter.hFile);

          if (Packet != NULL) {

              PacketInitPacket(
                  Packet,
                  Adapter.lpMem2,
                  64
                  );



              Adapter.lpMem2[0]=(UCHAR)0xff;
              Adapter.lpMem2[1]=(UCHAR)0xff;
              Adapter.lpMem2[2]=(UCHAR)0xff;
              Adapter.lpMem2[3]=(UCHAR)0xff;
              Adapter.lpMem2[4]=(UCHAR)0xff;
              Adapter.lpMem2[5]=(UCHAR)0xff;

              for (i=0;i<5;i++) {

                  PacketSendPacket(
                      Adapter.hFile,
                      Packet,
                      TRUE
                      );

              }

              PacketFreePacket(Packet);
          }

          return TRUE;


        default:

            return 0;

    }


}
