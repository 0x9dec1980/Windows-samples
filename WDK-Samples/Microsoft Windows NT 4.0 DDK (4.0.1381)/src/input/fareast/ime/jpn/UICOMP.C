/**********************************************************************/
/*                                                                    */
/*      UICOMP.C - Windows 95 FAKEIME                                 */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/
#include "precomp.h"

/**********************************************************************/
/*                                                                    */
/* CompStrWndProc()                                                   */
/*                                                                    */
/* IME UI window procedure                                            */
/*                                                                    */
/**********************************************************************/
LRESULT CALLBACK CompStrWndProc( hWnd, message, wParam, lParam )
HWND   hWnd;
UINT   message;
WPARAM wParam;
LPARAM lParam;
{
    HWND hUIWnd;

    switch (message)
    {
        case WM_PAINT:
            PaintCompWindow( hWnd);
            break;

        case WM_SETCURSOR:
        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            DragUI(hWnd,message,wParam,lParam);
            if ((message == WM_SETCURSOR) &&
                (HIWORD(lParam) != WM_LBUTTONDOWN) &&
                (HIWORD(lParam) != WM_RBUTTONDOWN)) 
                return DefWindowProc(hWnd,message,wParam,lParam);
            if ((message == WM_LBUTTONUP) || (message == WM_RBUTTONUP))
                SetWindowLong(hWnd,FIGWL_MOUSE,0L);
            break;

        case WM_MOVE:
            hUIWnd = (HWND)GetWindowLong(hWnd,FIGWL_SVRWND);
            if (IsWindow(hUIWnd))
                SendMessage(hUIWnd,WM_UI_DEFCOMPMOVE,wParam,lParam);
            break;

        default:
            if (!MyIsIMEMessage(message))
                return DefWindowProc(hWnd,message,wParam,lParam);
            break;
    }
    return 0;
}

/**********************************************************************/
/*                                                                    */
/* CreateCompWindow()                                                 */
/*                                                                    */
/**********************************************************************/
void PASCAL CreateCompWindow( HWND hUIWnd, LPUIEXTRA lpUIExtra,LPINPUTCONTEXT lpIMC )
{
    int i;
    RECT rc;

    lpUIExtra->dwCompStyle = lpIMC->cfCompForm.dwStyle;
    for (i=0; i < MAXCOMPWND ; i++)
    {
        if (!IsWindow(lpUIExtra->uiComp[i].hWnd))
        {
            lpUIExtra->uiComp[i].hWnd = 
                CreateWindowEx(0,
                             (LPTSTR)szCompStrClassName,NULL,
                             WS_COMPNODEFAULT,
                             0,0,1,1,
                             hUIWnd,NULL,hInst,NULL);
        }
        lpUIExtra->uiComp[i].rc.left   = 0;
        lpUIExtra->uiComp[i].rc.top    = 0;
        lpUIExtra->uiComp[i].rc.right  = 1;
        lpUIExtra->uiComp[i].rc.bottom = 1;
        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_FONT,(DWORD)lpUIExtra->hFont);
        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_SVRWND,(DWORD)hUIWnd);
        ShowWindow(lpUIExtra->uiComp[i].hWnd,SW_HIDE);
        lpUIExtra->uiComp[i].bShow = FALSE;
    }

    if (lpUIExtra->uiDefComp.pt.x == -1)
    {
        GetWindowRect(lpIMC->hWnd,&rc);
        lpUIExtra->uiDefComp.pt.x = rc.left;
        lpUIExtra->uiDefComp.pt.y = rc.bottom + 1;
    }

    if (!IsWindow(lpUIExtra->uiDefComp.hWnd))
    {
        lpUIExtra->uiDefComp.hWnd = 
            CreateWindowEx( WS_EX_WINDOWEDGE,
                         (LPTSTR)szCompStrClassName,NULL,
                         WS_COMPDEFAULT | WS_DLGFRAME,
                         lpUIExtra->uiDefComp.pt.x,
                         lpUIExtra->uiDefComp.pt.y,
                         1,1,
                         hUIWnd,NULL,hInst,NULL);
    }

    //SetWindowLong(lpUIExtra->uiDefComp.hWnd,FIGWL_FONT,(DWORD)lpUIExtra->hFont);
    SetWindowLong(lpUIExtra->uiDefComp.hWnd,FIGWL_SVRWND,(DWORD)hUIWnd);
    ShowWindow(lpUIExtra->uiDefComp.hWnd,SW_HIDE);
    lpUIExtra->uiDefComp.bShow = FALSE;

    return;
}


/**********************************************************************/
/*                                                                    */
/* NumCharInDX()                                                      */
/*                                                                    */
/* Count how may the char can be arranged in DX.                      */
/*                                                                    */
/**********************************************************************/
int PASCAL NumCharInDX(HDC hDC,LPTSTR lp,int dx)
{
    SIZE sz;
    int width = 0;
    int num   = 0;
    int numT  = 0;

    if (!*lp)
        return 0;

    while ((width < dx) && *(lp + numT))
    {
        num = numT++;
        GetTextExtentPoint(hDC,lp,numT,&sz);
        width = sz.cx;
    }

    if (width < dx)
        num = numT;

    return num;
}
/**********************************************************************/
/*                                                                    */
/* NumCharInDY()                                                      */
/*                                                                    */
/* Count how may the char can be arranged in DY.                      */
/*                                                                    */
/**********************************************************************/
int PASCAL NumCharInDY(HDC hDC,LPTSTR lp,int dy)
{
    SIZE sz;
    int width = 0;
    int num;
    int numT = 0;

    if (!*lp)
        return 0;

    while ((width < dy) && *(lp + numT))
    {
        num = numT++;
        GetTextExtentPoint(hDC,lp,numT,&sz);
        width = sz.cy;
    }

    return num;
}
/**********************************************************************/
/*                                                                    */
/* MoveCompWindow()                                                   */
/*                                                                    */
/* Calc the position of composition windows and move them.            */
/*                                                                    */
/**********************************************************************/
void PASCAL MoveCompWindow( LPUIEXTRA lpUIExtra,LPINPUTCONTEXT lpIMC )
{
    HDC hDC;
    HFONT hFont  = (HFONT)NULL;
    HFONT hOldFont = (HFONT)NULL;
    LPCOMPOSITIONSTRING lpCompStr;
    LPTSTR lpstr;
    RECT rc;
    RECT oldrc;
    SIZE sz;
    int width = 0;
    int height = 0;
    int i;

    //
    // Save the composition form style into lpUIExtra.
    //
    lpUIExtra->dwCompStyle = lpIMC->cfCompForm.dwStyle;

    if (lpIMC->cfCompForm.dwStyle)  // Style is not DEFAULT.
    {
        POINT ptSrc = lpIMC->cfCompForm.ptCurrentPos;
        RECT  rcSrc;
        LPTSTR lpt;
        int   dx;
        int   dy;
        int   num;
        int   curx;
        int   cury;

        //
        // Lock the COMPOSITIONSTRING structure.
        //
        if (!(lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr)))
            return;

        //
        // If there is no composition string, don't display anything.
        //
        if ((lpCompStr->dwSize <= sizeof(COMPOSITIONSTRING))
            || (lpCompStr->dwCompStrLen == 0))
        {
            ImmUnlockIMCC(lpIMC->hCompStr);
            return;
        }

        //
        // Set the rectangle for the composition string.
        //
        if (lpIMC->cfCompForm.dwStyle & CFS_RECT)
            rcSrc = lpIMC->cfCompForm.rcArea;
        else
            GetClientRect(lpIMC->hWnd,(LPRECT)&rcSrc);

        ClientToScreen(lpIMC->hWnd, &ptSrc);
        ClientToScreen(lpIMC->hWnd, (LPPOINT)&rcSrc.left);
        ClientToScreen(lpIMC->hWnd, (LPPOINT)&rcSrc.right);
        //
        // Check the start position.
        //
        if (!PtInRect((LPRECT)&rcSrc,ptSrc))
        {
            ImmUnlockIMCC(lpIMC->hCompStr);
            return;
        }

        //
        // Hide the default composition window.
        //
        if (IsWindow(lpUIExtra->uiDefComp.hWnd))
        {
            ShowWindow(lpUIExtra->uiDefComp.hWnd,SW_HIDE);
            lpUIExtra->uiDefComp.bShow = FALSE;
        }

        lpt = lpstr = GETLPCOMPSTR(lpCompStr);
        num = 1;

        if (!lpUIExtra->bVertical)
        {
            dx = rcSrc.right - ptSrc.x;
            curx = ptSrc.x;
            cury = ptSrc.y;

            //
            // Set the composition string to each composition window.
            // The composition windows that are given the compostion string
            // will be moved and shown.
            //
            for (i = 0; i < MAXCOMPWND; i++)
            {
                if (IsWindow(lpUIExtra->uiComp[i].hWnd))
                {
                    hDC = GetDC(lpUIExtra->uiComp[i].hWnd);

                    if (hFont = (HFONT)GetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_FONT))
                        hOldFont = SelectObject(hDC,hFont);

                    sz.cy = 0;
                    oldrc = lpUIExtra->uiComp[i].rc;

                    if (num = NumCharInDX(hDC,lpt,dx))
                    {
                        GetTextExtentPoint(hDC,lpt,num,&sz);

                        lpUIExtra->uiComp[i].rc.left    = curx;
                        lpUIExtra->uiComp[i].rc.top     = cury;
                        lpUIExtra->uiComp[i].rc.right   = sz.cx;
                        lpUIExtra->uiComp[i].rc.bottom  = sz.cy;
                        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_COMPSTARTSTR,
                                      (lpt - lpstr));
                        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_COMPSTARTNUM,
                                      num);
                        MoveWindow(lpUIExtra->uiComp[i].hWnd, 
                                 curx,cury,sz.cx,sz.cy,TRUE);
                        ShowWindow(lpUIExtra->uiComp[i].hWnd, SW_SHOWNOACTIVATE);
                        lpUIExtra->uiComp[i].bShow = TRUE;

                        lpt+=num;

                    }
                    else
                    {
                        lpUIExtra->uiComp[i].rc.left    = 0;
                        lpUIExtra->uiComp[i].rc.top     = 0;
                        lpUIExtra->uiComp[i].rc.right   = 0;
                        lpUIExtra->uiComp[i].rc.bottom  = 0;
                        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_COMPSTARTSTR,
                                      0L);
                        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_COMPSTARTNUM,
                                      0L);
                        ShowWindow(lpUIExtra->uiComp[i].hWnd, SW_HIDE);
                        lpUIExtra->uiComp[i].bShow = FALSE;
                    }

                    InvalidateRect(lpUIExtra->uiComp[i].hWnd,NULL,FALSE);

                    dx = rcSrc.right - rcSrc.left;
                    curx = rcSrc.left;
                    cury += sz.cy;

                    if (hOldFont)
                        SelectObject(hDC,hOldFont);
                    ReleaseDC(lpUIExtra->uiComp[i].hWnd,hDC);
                }
            }
        }
        else  // when it is vertical fonts.
        {
            dy = rcSrc.bottom - ptSrc.y;
            curx = ptSrc.x;
            cury = ptSrc.y;

            for (i = 0; i < MAXCOMPWND; i++)
            {
                if (IsWindow(lpUIExtra->uiComp[i].hWnd))
                {
                    hDC = GetDC(lpUIExtra->uiComp[i].hWnd);

                    if (hFont = (HFONT)GetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_FONT))
                        hOldFont = SelectObject(hDC,hFont);

                    sz.cy = 0;

                    if (num = NumCharInDY(hDC,lpt,dy))
                    {
                        GetTextExtentPoint(hDC,lpt,num,&sz);

                        lpUIExtra->uiComp[i].rc.left    = curx - sz.cy;
                        lpUIExtra->uiComp[i].rc.top     = cury;
                        lpUIExtra->uiComp[i].rc.right   = sz.cy;
                        lpUIExtra->uiComp[i].rc.bottom  = sz.cx;
                        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_COMPSTARTSTR,
                                      (lpt - lpstr));
                        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_COMPSTARTNUM,
                                      num);
                        MoveWindow(lpUIExtra->uiComp[i].hWnd, 
                                 curx,cury,sz.cy,sz.cx,TRUE);
                        ShowWindow(lpUIExtra->uiComp[i].hWnd, SW_SHOWNOACTIVATE);
                        lpUIExtra->uiComp[i].bShow = TRUE;

                        lpt+=num;
                    }
                    else
                    {
                        lpUIExtra->uiComp[i].rc.left    = 0;
                        lpUIExtra->uiComp[i].rc.top     = 0;
                        lpUIExtra->uiComp[i].rc.right   = 0;
                        lpUIExtra->uiComp[i].rc.bottom  = 0;
                        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_COMPSTARTSTR,
                                      0L);
                        SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_COMPSTARTNUM,
                                      0L);
                        ShowWindow(lpUIExtra->uiComp[i].hWnd, SW_HIDE);
                        lpUIExtra->uiComp[i].bShow = FALSE;
                    }

                    InvalidateRect(lpUIExtra->uiComp[i].hWnd,NULL,FALSE);

                    dy = rcSrc.bottom - rcSrc.top;
                    cury = rcSrc.top;
                    curx -= sz.cy;

                    if (hOldFont)
                        SelectObject(hDC,hOldFont);
                    ReleaseDC(lpUIExtra->uiComp[i].hWnd,hDC);
                }
            }
        }


        ImmUnlockIMCC(lpIMC->hCompStr);
    }
    else
    {
        //
        // When the style is DEFAULT, show the default composition window.
        //
        if (IsWindow(lpUIExtra->uiDefComp.hWnd))
        {
            for (i = 0; i < MAXCOMPWND; i++)
            {
                if (IsWindow(lpUIExtra->uiComp[i].hWnd))
                {
                    ShowWindow(lpUIExtra->uiComp[i].hWnd,SW_HIDE);
                    lpUIExtra->uiComp[i].bShow = FALSE;
                }
            }

            hDC = GetDC(lpUIExtra->uiDefComp.hWnd);

            if (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
            {
                if ((lpCompStr->dwSize > sizeof(COMPOSITIONSTRING))
                   && (lpCompStr->dwCompStrLen > 0))
                {
                    lpstr = GETLPCOMPSTR(lpCompStr);
                    GetTextExtentPoint(hDC,lpstr,lstrlen(lpstr),&sz);
                    width = sz.cx;
                    height = sz.cy;
                }
                ImmUnlockIMCC(lpIMC->hCompStr);
            }

            ReleaseDC(lpUIExtra->uiDefComp.hWnd,hDC);
        
            GetWindowRect(lpUIExtra->uiDefComp.hWnd,&rc);
            lpUIExtra->uiDefComp.pt.x = rc.left;
            lpUIExtra->uiDefComp.pt.y = rc.top;
            MoveWindow(lpUIExtra->uiDefComp.hWnd,
                       rc.left,
                       rc.top,
                       width+ 2 * GetSystemMetrics(SM_CXEDGE),
                       height+ 2 * GetSystemMetrics(SM_CYEDGE),
                       TRUE);

            ShowWindow(lpUIExtra->uiDefComp.hWnd, SW_SHOWNOACTIVATE);
            lpUIExtra->uiDefComp.bShow = TRUE;
            InvalidateRect(lpUIExtra->uiDefComp.hWnd,NULL,FALSE);
        }
    }

}

/**********************************************************************/
/*                                                                    */
/* DrawTextOneLine(hDC, lpstr, lpattr, num)                           */
/*                                                                    */
/**********************************************************************/
void PASCAL DrawTextOneLine( HWND hCompWnd, HDC hDC, LPTSTR lpstr, LPBYTE lpattr, int num, BOOL fVert)
{
    LPTSTR lpStart = lpstr;
    LPTSTR lpEnd = lpstr + num - 1;
    int x,y;
    RECT rc;

    if (num = 0)
        return;

    GetClientRect (hCompWnd,&rc);

    if (!fVert)
    {
        x = 0;
        y = 0;
    }
    else
    {
        x = rc.right;
        y = 0;
    }

    while (lpstr <= lpEnd)
    {
        int cnt = 0;
        BYTE bAttr = *lpattr;
        SIZE sz;

        while ((bAttr == *lpattr) && (cnt <= num))
        {
            lpattr++;
            cnt++;
        }
        switch (bAttr)
        {
            case ATTR_INPUT:
                break;

            case ATTR_TARGET_CONVERTED:
                SetTextColor(hDC,RGB(127,127,127));
                if (!fVert)
                    TextOut(hDC,x + 1,y + 1,lpstr,cnt);
                else
                    TextOut(hDC,x - 1,y + 1,lpstr,cnt);
                SetTextColor(hDC,RGB(0,0,0));
                SetBkMode(hDC,TRANSPARENT);
                break;

            case ATTR_CONVERTED:
                SetTextColor(hDC,RGB(127,127,127));
                if (!fVert)
                    TextOut(hDC,x + 1,y + 1,lpstr,cnt);
                else
                    TextOut(hDC,x - 1,y + 1,lpstr,cnt);
                SetTextColor(hDC,RGB(0,0,255));
                SetBkMode(hDC,TRANSPARENT);
                break;

            case ATTR_TARGET_NOTCONVERTED:
                break;
        }

        TextOut(hDC,x,y,lpstr,cnt);

        GetTextExtentPoint(hDC,lpstr,cnt,&sz);
        lpstr += cnt;

        if (!fVert)
            x += sz.cx;
        else
            y += sz.cx;
    }

}

/**********************************************************************/
/*                                                                    */
/* PaintCompWindow(hCompWnd)                                          */
/*                                                                    */
/**********************************************************************/
void PASCAL PaintCompWindow( HWND hCompWnd)
{
    PAINTSTRUCT ps;
    HIMC hIMC;
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    HDC hDC;
    RECT rc;
    HFONT hFont = (HFONT)NULL;
    HFONT hOldFont = (HFONT)NULL;
    HWND hSvrWnd;

    hDC = BeginPaint(hCompWnd,&ps);

    if (hFont = (HFONT)GetWindowLong(hCompWnd,FIGWL_FONT))
        hOldFont = SelectObject(hDC,hFont);

    hSvrWnd = (HWND)GetWindowLong(hCompWnd,FIGWL_SVRWND);

    if (hIMC = (HIMC)GetWindowLong(hSvrWnd,IMMGWL_IMC))
    {
        lpIMC = ImmLockIMC(hIMC);
        if (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
        {
            if ((lpCompStr->dwSize > sizeof(COMPOSITIONSTRING))
               && (lpCompStr->dwCompStrLen > 0))
            {
                LPTSTR lpstr;
                LPBYTE lpattr;
                LONG lstart;
                LONG num;
                BOOL fVert = FALSE;

                if (hFont)
                    fVert = (lpIMC->lfFont.A.lfEscapement == 2700);

                lpstr = GETLPCOMPSTR(lpCompStr);
                lpattr = GETLPCOMPATTR(lpCompStr);
                SetBkMode(hDC,TRANSPARENT);
                if (lpIMC->cfCompForm.dwStyle)
                {
                    HDC hPDC;

                    hPDC = GetDC(GetParent(hCompWnd));
                    GetClientRect (hCompWnd,&rc);
                    SetBkColor(hDC,GetBkColor(hPDC));
                    SetBkMode(hDC,OPAQUE);

                    lstart = GetWindowLong(hCompWnd,FIGWL_COMPSTARTSTR);
                    num = GetWindowLong(hCompWnd,FIGWL_COMPSTARTNUM);
                
                    if (!num || ((lstart + num) > lstrlen(lpstr)))
                        goto end_pcw;

                    lpstr+=lstart;
                    lpattr+=lstart;
                    DrawTextOneLine(hCompWnd, hDC, lpstr, lpattr, num, fVert);
                    ReleaseDC(GetParent(hCompWnd),hPDC);
                }
                else
                {
                    num = lstrlen(lpstr);
                    DrawTextOneLine(hCompWnd, hDC, lpstr, lpattr, num, fVert);
                }
            }
end_pcw:
            ImmUnlockIMCC(lpIMC->hCompStr);
        }
        ImmUnlockIMC(hIMC);
    }
    if (hFont && hOldFont)
        SelectObject(hDC,hOldFont);
    EndPaint(hCompWnd,&ps);
}
/**********************************************************************/
/*                                                                    */
/* HideCompWindow(lpUIExtra)                                          */
/*                                                                    */
/**********************************************************************/
void PASCAL HideCompWindow(LPUIEXTRA lpUIExtra)
{
    int i;
    RECT rc;

    if (IsWindow(lpUIExtra->uiDefComp.hWnd))
    {
        if (!lpUIExtra->dwCompStyle)
            GetWindowRect(lpUIExtra->uiDefComp.hWnd,(LPRECT)&rc);

        ShowWindow(lpUIExtra->uiDefComp.hWnd, SW_HIDE);
        lpUIExtra->uiDefComp.bShow = FALSE;
    }

    for (i=0;i<MAXCOMPWND;i++)
    {
        if (IsWindow(lpUIExtra->uiComp[i].hWnd))
        {
            ShowWindow(lpUIExtra->uiComp[i].hWnd, SW_HIDE);
            lpUIExtra->uiComp[i].bShow = FALSE;
        }
    }
}

/**********************************************************************/
/*                                                                    */
/* SetFontCompWindow(lpUIExtra)                                       */
/*                                                                    */
/**********************************************************************/
void PASCAL SetFontCompWindow(LPUIEXTRA lpUIExtra)
{
    int i;

    for (i=0;i<MAXCOMPWND;i++)
        if (IsWindow(lpUIExtra->uiComp[i].hWnd))
            SetWindowLong(lpUIExtra->uiComp[i].hWnd,FIGWL_FONT,(DWORD)lpUIExtra->hFont);

}
