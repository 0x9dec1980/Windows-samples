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

//#include "packet32.h"

#include "hellowin.h"
//#include "ntddndis.h"



HFONT GetFont(void);

DWORD GetTextSize(HWND hWnd, HFONT hFont);



extern HWND          hwndchild;
extern CONTROL_BLOCK Adapter;


extern UINT    showdump;
HFONT   hFont;

LRESULT FAR PASCAL ChildWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)

  {
    HDC           hdc;
    PAINTSTRUCT   ps;
    RECT          rect;
    UINT          i,j;
    TCHAR         szbuff[100],szbuff2[8],szbuff3[17];
    UINT          limit;
    LPSTR         lpMem;
static HFONT  hOldFont;
static UINT       cyclient,cxclient,lines,dumppos,oldpos,paragphs,cychar;
static WORD   start;

    switch (message)
      {
        case WM_CREATE:
          cychar = HIWORD(GetTextSize(hwnd,hFont));


        case WM_DUMPCHANGE:
          oldpos=0;
          dumppos=0;
          wsprintf(szbuff,TEXT("Hex Dump--"));
          SetWindowText(hwnd,szbuff);

          limit=Adapter.BufferSize;

          paragphs=(short)((limit & 0xffff) >> 4);
          SetScrollRange(hwnd,SB_VERT,0,(WORD)paragphs,0);
          SetScrollPos(hwnd,SB_VERT,(WORD)dumppos,1);

          InvalidateRect(hwnd,NULL,1);
          return 0;

        case WM_SIZE:
          cyclient=HIWORD(lParam);
          cxclient=LOWORD(lParam);
          lines=cyclient/cychar;
          return 0;


	case WM_PAINT:

          hdc=BeginPaint(hwnd,&ps);
          hOldFont=SelectObject(hdc,hFont);

          limit=Adapter.PacketLength;


          limit=limit & 0xffff;

          lpMem=(LPSTR)Adapter.lpMem;

          start=dumppos*cychar;

          for (i=start ;((i<limit) && ((i-start)/cychar<lines));i+=16) {

              wsprintf(
                  szbuff,
                  TEXT("%08lx  "),
                  i
                  );

              for (j=0;(j<16 && (j+i<limit));j++) {

                  wsprintf(
                      szbuff2,
                      TEXT("%02hX "),
                      ((WORD)lpMem[i+j] & 0xff)
                      );

                  lstrcat(szbuff,szbuff2);

                  if (j==3 || j==7 || j==11) {

                      lstrcat(szbuff,TEXT(" "));
                  }

                  wsprintf(
                      &szbuff3[j],
                      TEXT("%c"),
                      lpMem[i+j]
                      );

                  if (szbuff3[j]=='\0') {

                      szbuff3[j]='.';
                  }

              }

              szbuff3[j]='\0';
              lstrcat(szbuff,szbuff3);

              TextOut(
                  hdc,
                  0,
                  i-start,
                  szbuff,
                  lstrlen(szbuff)
                  );

          }



          SelectObject(hdc,hOldFont);
	  EndPaint(hwnd,&ps);
          return 0;

        case WM_VSCROLL:
          oldpos=dumppos;
          switch(wParam)
            {
              case SB_PAGEDOWN:
                dumppos+=lines;
                break;

              case SB_LINEDOWN:
                dumppos+=1;
                break;

              case SB_THUMBPOSITION:
                dumppos=LOWORD(lParam);
                break;

              case SB_LINEUP:
                dumppos-=1;
                break;

              case SB_PAGEUP:
                dumppos-=lines;
                break;

              default:
                return 0;
            }

          limit=Adapter.BufferSize;

          paragphs=(short)((limit & 0xffff) >> 4);

          if (dumppos<0)
             dumppos=0;
           else
              if (dumppos>paragphs)
                 dumppos=paragphs;

          if (((oldpos-dumppos==1) || (dumppos-oldpos==1)))
             {
               rect.left=0;
               rect.top=0;
               rect.right=cxclient;
               rect.bottom=lines*cychar;
               ScrollWindow(hwnd,0,cychar*(oldpos-dumppos),&rect,NULL);
               UpdateWindow(hwnd);
             }
           else
             InvalidateRect(hwnd,NULL,1);

          SetScrollPos(hwnd,SB_VERT,(WORD)dumppos,1);
          return 0;


      }
    return DefWindowProc(hwnd,message, wParam, lParam);
  }


HFONT GetFont(void)
  {
    static LOGFONT logfont;

    logfont.lfHeight=16;
    logfont.lfCharSet=ANSI_CHARSET;
    logfont.lfQuality=PROOF_QUALITY;
    logfont.lfPitchAndFamily=FIXED_PITCH | FF_MODERN;
    lstrcpy((LPTSTR)&logfont.lfFaceName,(LPTSTR)TEXT("Courier"));

    return CreateFontIndirect(&logfont);
  }

DWORD GetTextSize(HWND hWnd, HFONT hFont)
  {
    TEXTMETRIC     Metrics;
    HDC            hDC;
    HFONT          hOldFont;

    hDC = GetDC(hWnd);
    hOldFont=SelectObject(hDC,hFont);
    GetTextMetrics(hDC,&Metrics);
    SelectObject(hDC,hOldFont);
    ReleaseDC(hWnd,hDC);

    return MAKELONG(Metrics.tmAveCharWidth,
                    Metrics.tmHeight+Metrics.tmExternalLeading);

  }
