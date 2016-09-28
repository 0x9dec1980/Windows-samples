/*++
Copyright (c) 1992  Microsoft Corporation

Module Name:

    Detdlg.c

Abstract:

    Window NT Netdetect tester

Author:

    Brian Lieuallen (BrianL)

Environment:

    WIN32 User Mode

Revision History:

--*/


#define UNICODE 1

#include <windows.h>
#include <windowsx.h>

#include "detapp.h"

#include "dialogs.h"





extern TCHAR   szWindowTitle[];






BOOL CallUpDlgBox(HWND hWnd,FARPROC DlgBoxProc,LPTSTR Template,LPARAM lParam)
{
    DLGPROC     lpProcAbout;
    BOOL        result;
    HINSTANCE   hInst;

    hInst=(HINSTANCE)GetWindowLong(hWnd,GWL_HINSTANCE);


    lpProcAbout =(DLGPROC) MakeProcInstance (DlgBoxProc, hInst);

    result=DialogBoxParam(hInst,
                          Template,
                          hWnd,
                          lpProcAbout,
                          lParam);

    FreeProcInstance((FARPROC)lpProcAbout);
    return  result;
}




BOOL FAR PASCAL
AboutBoxProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LONG lParam
    )

{

    switch (message) {

        case WM_INITDIALOG:

            return 1;


        case WM_COMMAND:

            switch (LOWORD(wParam)) {

                case IDOK:

                    EndDialog (hDlg,FALSE);
                    return 0;


            }

            return 0;

        case WM_SYSCOMMAND:

            if (wParam==SC_CLOSE) {

                EndDialog (hDlg,FALSE);
                return 0;

            }

      }

    return 0;
}







BOOL FAR PASCAL
DetectBoxProc(
    HWND hdlg,
    UINT message,
    WPARAM wParam,
    LONG lParam
    )

{
    LPDETECT         Detect;

    TCHAR            TokenStr[128];

    LONG             Status;

    LONG             Index;

    HWND             hWndList;
    HWND             hWndList2;

    UINT             Tabs[2];

    UINT             i;

    Detect=(LPDETECT)GetWindowLong(hdlg,DWL_USER);

    hWndList=GetDlgItem(hdlg,IDD_LISTBOX);

    switch (message) {

        case WM_INITDIALOG:

            SetWindowLong(hdlg,DWL_USER,(ULONG)lParam);

            Detect =(LPDETECT)lParam;

            Tabs[0]=100;

            SendMessage(
                hWndList,
                LB_SETTABSTOPS,
                1,
                (LONG)Tabs
                );


            Index=1000;

            Status=(*Detect->DllList[Detect->CurrentDll].IdentifyProc)(Index,TokenStr,64);

            while (Status == 0) {

                for (i=0;i<128;i++) {

                    if (TokenStr[i]==TEXT('\0')) {

                        Status=(*Detect->DllList[Detect->CurrentDll].IdentifyProc)(
                                   Index+3,
                                   &TokenStr[i+1],
                                   128-(i+2)
                                   );

                        if (Status==0) {

                           TokenStr[i]=TEXT('\t');

                        }

                        break;

                    }

                }

                SendMessage(
                    hWndList,
                    LB_ADDSTRING,
                    0,
                    (LONG)TokenStr
                    );

                Index+=100;

                Status=(*Detect->DllList[Detect->CurrentDll].IdentifyProc)(Index,TokenStr,64);
            }

            SendMessage(
                hWndList,
                LB_SETCURSEL,
                0,
                0
                );



            hWndList2=GetDlgItem(hdlg,IDD_LISTBOX2);

            Tabs[0]=72;
            Tabs[1]=84;

            SendMessage(
                hWndList2,
                LB_SETTABSTOPS,
                2,
                (LONG)Tabs
                );

            hWndList2=GetDlgItem(hdlg,IDD_LISTBOX4);

            SendMessage(
                hWndList2,
                LB_SETTABSTOPS,
                2,
                (LONG)Tabs
                );

            AdapterListBoxHandler(
                Detect,
                hdlg,
                LBN_SELCHANGE
                );


            Detect->BusType=Isa;

            CheckRadioButton(
                hdlg,
                IDD_RADIO1,
                IDD_RADIO4,
                IDD_RADIO2
                );

            return 1;


        case WM_COMMAND:

            switch (LOWORD(wParam)) {

                case IDOK:

                    EndDialog (hdlg,FALSE);
                    return 0;

                case IDD_FIND:

                    FindAdapterHandler(
                        Detect,
                        hdlg
                        );

                    return 0;


                case IDD_CREATE:

                    CreateAdapterHandler(
                        Detect,
                        hdlg
                        );

                    return 0;


                case IDD_QUERY:

                    FoundListBoxHandler(
                        Detect,
                        hdlg,
                        LBN_DBLCLK
                        );

                    return 0;


                case IDD_LISTBOX:

                    AdapterListBoxHandler(
                        Detect,
                        hdlg,
                        HIWORD(wParam)
                        );

                    return 0;

                case IDD_LISTBOX3:

                    FoundListBoxHandler(
                        Detect,
                        hdlg,
                        HIWORD(wParam)
                        );

                    return 0;

                case IDD_LISTBOX2:

                    RangeListBoxHandler(
                        Detect,
                        hdlg,
                        HIWORD(wParam)
                        );

                    return 0;

                case IDD_RADIO1:
                case IDD_RADIO2:
                case IDD_RADIO3:
                case IDD_RADIO4:

                    Detect->BusType=(INTERFACE_TYPE)(wParam-IDD_RADIO1);

                    CheckRadioButton(
                        hdlg,
                        IDD_RADIO1,
                        IDD_RADIO4,
                        wParam
                        );

                    return 0;

            }

        case WM_SYSCOMMAND:

            if (wParam==SC_CLOSE) {

                EndDialog (hdlg,FALSE);
                return 0;

            }


      }

    return 0;
}



BOOLEAN
AdapterListBoxHandler(
    LPDETECT  Detect,
    HWND      hDlg,
    UINT      Notification
    )

{

    HWND      hWndAdapterListBox;
    HWND      hWndParamListBox;

    LONG             Status;

    LONG             Index;

    TCHAR            TokenStr[128];

    UINT             i,j;

    PTSTR            Mask;

    hWndAdapterListBox=GetDlgItem(hDlg,IDD_LISTBOX);

    hWndParamListBox=GetDlgItem(hDlg,IDD_LISTBOX2);

    if (Notification==LBN_SELCHANGE) {

        InitWideBuffer(TokenStr,128);

        Index=SendMessage(
                  hWndAdapterListBox,
                  LB_GETCURSEL,
                  0,
                  0
                  );

        Index= (Index*100) + 1000;

        Detect->CurrentIndex=Index;

        Status=(*Detect->DllList[Detect->CurrentDll].QueryMaskProc)(Index,TokenStr,128);

        SendMessage(
            hWndParamListBox,
            LB_RESETCONTENT,
            0,
            0
            );

        SendMessage(
            GetDlgItem(hDlg,IDD_LISTBOX3),
            LB_RESETCONTENT,
            0,
            0
            );

        SendMessage(
            GetDlgItem(hDlg,IDD_LISTBOX4),
            LB_RESETCONTENT,
            0,
            0
            );

        SendMessage(
            GetDlgItem(hDlg,IDD_LISTBOX5),
            LB_RESETCONTENT,
            0,
            0
            );

        EnableWindow(
            GetDlgItem(hDlg,IDD_QUERY),
            FALSE
            );


        if (Status==0) {

            j=0;

            Mask=&TokenStr[0];

            for (i=0;i<128;i++) {

                if (TokenStr[i] == (TCHAR)0) {

                    if (j<2) {

                        TokenStr[i] = TEXT('\t');
                        j++;


                    } else {

                        SendMessage(
                            hWndParamListBox,
                            LB_ADDSTRING,
                            0,
                            (LONG)Mask
                            );

                        j=0;


                        Mask=&TokenStr[i+1];
                    }

                    if (TokenStr[i+1] == (TCHAR)0) {

                        break;
                    }



                }
            }
        }
    }

    return TRUE;
}





BOOLEAN
FoundListBoxHandler(
    LPDETECT  Detect,
    HWND      hDlg,
    UINT      Notification
    )

{

    HWND      hWndFoundListBox;
    HWND      hWndConfigListBox;

    LONG             Index;

    TCHAR            TokenStr[128];

    UINT             i,j;

    PTSTR            Mask;

    hWndFoundListBox=GetDlgItem(hDlg,IDD_LISTBOX3);

    hWndConfigListBox=GetDlgItem(hDlg,IDD_LISTBOX4);

    SendMessage(
        hWndConfigListBox,
        LB_RESETCONTENT,
        0,
        0
        );


    if (Notification==LBN_SELCHANGE) {

        Index=SendMessage(
                  hWndFoundListBox,
                  LB_GETCURSEL,
                  0,
                  0
                  );

        Detect->CurrentFoundAdapter=Index;

    } else {

        if (Notification==LBN_DBLCLK) {

            Index=SendMessage(
                      hWndFoundListBox,
                      LB_GETCURSEL,
                      0,
                      0
                      );

            Detect->CurrentFoundAdapter=Index;


            j=0;

            Mask=&TokenStr[0];

            for (i=0;i<128;i++) {

                TokenStr[i]=Detect->FoundConfig[Detect->CurrentFoundAdapter][i];

                if (Detect->FoundConfig[Detect->CurrentFoundAdapter][i] == (TCHAR)0) {

                    if (j<1) {

                        TokenStr[i] = TEXT('\t');
                        j++;

                    } else {

                        SendMessage(
                            hWndConfigListBox,
                            LB_ADDSTRING,
                            0,
                            (LONG)Mask
                            );

                        j=0;

                        if (Detect->FoundConfig[Detect->CurrentFoundAdapter][i+1] == (TCHAR)0) {

                            break;
                        }

                        Mask=&TokenStr[i+1];

                    }
                }

            }
        }
    }

    return TRUE;
}





BOOLEAN
FindAdapterHandler(
    LPDETECT  Detect,
    HWND      hDlg
    )

{

    HWND      hWndFoundListBox;

    LONG      Confidence;

    LONG      Status;
    UINT      i;

    TCHAR     TokenStr[128];

    BOOLEAN   First;

    PVOID     Handle;

    hWndFoundListBox=GetDlgItem(hDlg,IDD_LISTBOX3);

    SendMessage(
            hWndFoundListBox,
            LB_RESETCONTENT,
            0,
            0
            );


    Detect->AdaptersFound=0;

    First=TRUE;

    for (i=0; i <MAX_FOUND; i++) {

        InitWideBuffer(
            Detect->FoundConfig[i],
            128
            );

        Status=(*Detect->DllList[Detect->CurrentDll].FirstNextProc)(
                   Detect->CurrentIndex,
                   Detect->BusType,
                   0,
                   First,
                   &Detect->FoundTokens[i],
                   &Confidence
                   );

        First=FALSE;

        if ((Confidence == 0) || (Status != 0)) {

            break;
        }

        Status=(*Detect->DllList[Detect->CurrentDll].OpenProc)(
                   Detect->FoundTokens[Detect->CurrentFoundAdapter],
                   &Handle
                   );

        if (Status == 0) {

            Status=(*Detect->DllList[Detect->CurrentDll].QueryProc)(
                       Handle,
                       Detect->FoundConfig[i],
                       128
                       );

            if (Status == 0 ) {

                Status=(*Detect->DllList[Detect->CurrentDll].VerifyProc)(
                           Handle,
                           Detect->FoundConfig[i]
                           );

                if (Status != 0) {

                    MessageBox(
                        hDlg,
                        TEXT("Verification of queried info failed"),
                        szWindowTitle,
                        MB_OK | MB_ICONEXCLAMATION
                        );
                }

            } else {

                MessageBox(
                    hDlg,
                    TEXT("Query Failed on found adapter"),
                    szWindowTitle,
                    MB_OK | MB_ICONEXCLAMATION
                    );
            }


            Status=(*Detect->DllList[Detect->CurrentDll].CloseProc)(
                       Handle
                       );


        }


        Detect->AdaptersFound++;

        Status=(*Detect->DllList[Detect->CurrentDll].IdentifyProc)(
                   Detect->CurrentIndex,
                   TokenStr,
                   64
                   );

        if (Status == 0) {

            SendMessage(
                hWndFoundListBox,
                LB_ADDSTRING,
                0,
                (LONG)TokenStr
                );
        }

    }

    if (Detect->AdaptersFound > 0) {


        SendMessage(
            hWndFoundListBox,
            LB_SETCURSEL,
            0,
            0
            );

        EnableWindow(
            GetDlgItem(hDlg,IDD_QUERY),
            TRUE
            );
    }

    return TRUE;

}




BOOLEAN
RangeListBoxHandler(
    LPDETECT  Detect,
    HWND      hDlg,
    UINT      Notification
    )

{

    HWND      hWndParamListBox;
    HWND      hWndRangeListBox;

    LONG             Status;

    LONG             Index;

    TCHAR            TokenStr[128];

    UINT             i;

    UINT             Length;

    LONG             Values[64];

    UINT             NumberOfValues=64;

    for (i=0;i<128;i++) { TokenStr[i]=TEXT('a'); }



    hWndParamListBox=GetDlgItem(hDlg,IDD_LISTBOX2);

    hWndRangeListBox=GetDlgItem(hDlg,IDD_LISTBOX5);

    SendMessage(
        hWndRangeListBox,
        LB_RESETCONTENT,
        0,
        0
        );


    if (Notification==LBN_SELCHANGE) {

        Index=SendMessage(
                  hWndParamListBox,
                  LB_GETCURSEL,
                  0,
                  0
                  );

        Length=SendMessage(
                  hWndParamListBox,
                  LB_GETTEXT,
                  Index,
                  (LONG)TokenStr
                  );


        for (i=0;i<128;i++) {

            if (TokenStr[i] == TEXT('\t')) {

                TokenStr[i] =  TEXT('\0');

                break;
            }
        }

        Status=(*Detect->DllList[Detect->CurrentDll].RangeProc)(
                   Detect->CurrentIndex,
                   TokenStr,
                   Values,
                   &NumberOfValues
                   );


        if (Status == 0) {

            for (i=0;i<NumberOfValues; i++) {

                wsprintf(TokenStr,TEXT("0x0%x"),Values[i]);

                SendMessage(
                    hWndRangeListBox,
                    LB_ADDSTRING,
                    0,
                    (LONG)TokenStr
                    );

            }
        }

    }




    return TRUE;
}









BOOLEAN
CreateAdapterHandler(
    LPDETECT  Detect,
    HWND      hDlg
    )

{

    PVOID     Handle;
    LONG      Status;

    Status=(*Detect->DllList[Detect->CurrentDll].CreateProc)(
               Detect->CurrentIndex,
               Detect->BusType,
               0,
               &Handle
               );


    if (Status == 0) {

        Status=(*Detect->DllList[Detect->CurrentDll].CloseProc)(
                   Handle
                   );

        MessageBox(
            hDlg,
            TEXT("Handle Created Successfully"),
            szWindowTitle,
            MB_OK | MB_ICONINFORMATION
            );

    } else {

        MessageBox(
            hDlg,
            TEXT("Handle Creation failed"),
            szWindowTitle,
            MB_OK | MB_ICONEXCLAMATION
            );

        return FALSE;

   }

   return TRUE;

}


VOID
InitWideBuffer(
    PTCHAR    Buffer,
    UINT      Length
    )

{
    UINT      i;

    for (i=0; i<Length; i++) {

         Buffer[i]=TEXT('a');

    }

    if (Length>2) {

        Buffer[Length-2]=TEXT('\0');
        Buffer[Length-1]=TEXT('\0');
    }

    return;

}
