/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       multadpt.c
 *  Content:    multiple adapter support
 *
 *  exported functions:
 *      Enable
 *      Disable
 *      SetPalette
 *      DDIGammaRamp
 *      ValidateMode
 *      ValidateDesk
 *      Control
 *  internal functions:
 *      VDDCall
 *
 ***************************************************************************/
#ifndef _WIN32
#define NOUSER
#define NOGDI
#define NOGDIOBJ
#define NOGDICAPMASKS
#pragma warning(disable:4103)
#include <windows.h>
#pragma warning(default:4103)
#include <winerror.h>

#define NOEXTDEVMODEPROPSHEET       // no property sheet structs in print.h
#define LPDOCINFO void * // avoids a compile error in print.h
#include <print.h>                  // to get DEVMODE struct.

#define NOPTRC
#define PTTYPE POINT
#include <gdidefs.inc>
#include <dibeng.inc>
#include <minivdd.h>

#include "debug.h"

/* Icon/Cursor header */
typedef struct
{
   int     xHotSpot, yHotSpot;
   int     cx, cy;
   int     cbWidth;
   BYTE    Planes, BitsPixel;
} CURSORSHAPE;
#endif // _WIN32
#include "valmode.inc"
#include "glglobal.h"

extern LPGLINTINFO PASCAL pGLInfo;
extern GLINT_GAMMA GLGamma;

#define NO_VALIDATEDESK

/***************************************************************************
 *
 * internal functions.
 *
 ***************************************************************************/
DWORD VDDCall(DWORD DeviceHandle, DWORD function, DWORD flags, LPVOID buffer, DWORD buffer_size);
UINT FAR PASCAL _loadds CreatePhysSel(DWORD PhysAddress, DWORD PhysSize);
void FreeSel(UINT Sel);
DWORD PhysToLinear(DWORD PhysAddress, DWORD Size);
#define VDDOpenDevice(sz)   VDDCall(0, VDD_OPEN, 0, sz, 0)
#define VDDCloseDevice(h)   VDDCall(h, VDD_CLOSE, 0, NULL, 0)
#define HWEnable(f)  ((pGLInfo->dwDeviceHandle==1) ? 0 : VDDCall(pGLInfo->dwDeviceHandle, VDD_ENABLE, f, NULL, 0))
/***************************************************************************
 *
 * external functions.
 *
 ***************************************************************************/
extern UINT  FAR PASCAL Enable1        (LPVOID, UINT, LPSTR, LPSTR, LPVOID);
extern UINT  FAR PASCAL ReEnable1      (LPVOID, LPVOID);
extern UINT  FAR PASCAL Disable1       (DIBENGINE FAR *);
extern LONG  FAR PASCAL Control1       (DIBENGINE FAR *, UINT, LPVOID, LPVOID);
extern UINT  FAR PASCAL SetPalette1    (UINT start, UINT count, DWORD FAR *lpPalette);
extern UINT  FAR PASCAL SetGammaRamp1  (LPVOID lpGammaRampData);
extern UINT  FAR PASCAL GetGammaRamp1  (LPVOID lpGammaRampData);
extern UINT  FAR PASCAL ValidateMode1  (DISPVALMODE FAR *);
extern UINT  FAR PASCAL ValidateDesk1  (DISPVALMODE FAR *);
/***************************************************************************
 *
 * DEBUG stuff
 *
 ***************************************************************************/
/*
#ifdef DEBUG
extern void FAR __cdecl DPF(LPSTR szFormat, ...);
#define BREAK() DebugBreak();
#else
#define DPF         1 ? (void)0 : (void)
#define BREAK()
#define BREAK()
#endif */

/***************************************************************************
 *
 * CanHWRunAsSecondary should always return true, but we may want to
 * disable multimonitor from the registry.
 *
 ***************************************************************************/
BOOL  FAR PASCAL CanHWRunAsSecondary (void)
{
    HKEY    hKey;
    LONG    Size = 4;
    WORD    Result = 1;
	char	RegistryKey[SIZE_CONFIGURATIONBASE+18];

	lstrcpy(RegistryKey, "Software\\");
	lstrcat(RegistryKey, pGLInfo->RegistryConfigBase);
	lstrcat(RegistryKey, "\\Display");
    if (RegOpenKey(HKEY_LOCAL_MACHINE, RegistryKey, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, "DisableMultimonitor", NULL, NULL, NULL, &Size) == ERROR_SUCCESS)
            Result = 0;
        RegCloseKey(hKey);
    }
    return(Result);
}

/***************************************************************************
 *
 * Enable   called by GDI to enable the device and set the video mode
 *
 ***************************************************************************/
#pragma optimize("gle", off)

UINT FAR PASCAL _loadds Enable(
                              LPVOID  lpDevice,
                              UINT        style,
                              LPSTR       lpDeviceName,
                              LPSTR       lpOutput,
                              LPVOID      lpStuff)
{
   BOOL    f;
   UINT size;
   DIBENGINE FAR *  pdevice;
#ifdef DEBUG
   DISPLAYINFO DisplayInfo;
#endif //DEBUG
   pdevice=(DIBENGINE FAR *)lpDevice;

        //
        // open the device if we have not done it yet.
        //
   if (pGLInfo->dwDeviceHandle==0)
   {
      if (lpDeviceName)
      {
         pGLInfo->dwDeviceHandle = VDDOpenDevice(lpDeviceName);
         DPF("we got a device name '%s' h=%08lX",lpDeviceName,pGLInfo->dwDeviceHandle);
         if (pGLInfo->dwDeviceHandle == 0 || pGLInfo->dwDeviceHandle == 0xFFFFFFFF)
         {
            DPF("fail on VDDOpenDevice call");
            return FALSE;
         }
         else
         {
            DPF("Enable as external driver.");
         }
      }
      else
      {
         pGLInfo->dwDeviceHandle = 1;
         DPF("Enable as the DISPLAY driver.");
      }
   }
#ifdef DEBUG
   _fmemset(&DisplayInfo, 0, sizeof(DisplayInfo));
   VDDCall(pGLInfo->dwDeviceHandle, VDD_GET_DISPLAY_CONFIG, 0,
           &DisplayInfo, sizeof(DisplayInfo));

   DPF("DISPLAY:Settings %dx%dx%d %d-%dHz [%dx%d]",
       (int)DisplayInfo.diXRes,
       (int)DisplayInfo.diYRes,
       (int)DisplayInfo.diBpp,
       (int)DisplayInfo.diRefreshRateMin,
       (int)DisplayInfo.diRefreshRateMax,
       (int)DisplayInfo.diXDesktopSize,
       (int)DisplayInfo.diYDesktopSize);
        //
        //  or use the DEVMODE passed to CreateDC
        //
   if (lpStuff && pGLInfo->dwDeviceHandle > 1)
   {
      DEVMODE FAR *pdm = (DEVMODE FAR *)lpStuff;

      DPF("we got passed a DEVNODE %dx%dx%d @ %dHz.",
          (int)pdm->dmPelsWidth,
          (int)pdm->dmPelsHeight,
          (int)pdm->dmBitsPerPel,
          (int)pdm->dmDisplayFrequency);

      if (pdm->dmFields & DM_BITSPERPEL)
      {
         DisplayInfo.diBpp = (BYTE)pdm->dmBitsPerPel;
      }

      if (pdm->dmFields & (DM_PELSWIDTH|DM_PELSHEIGHT))
      {
         DisplayInfo.diXRes         = (UINT)pdm->dmPelsWidth;
         DisplayInfo.diYRes         = (UINT)pdm->dmPelsHeight;
         DisplayInfo.diXDesktopSize = pdm->dmPelsWidth;
         DisplayInfo.diYDesktopSize = pdm->dmPelsHeight;
      }

      if (pdm->dmFields & DM_DISPLAYFREQUENCY)
      {
         DisplayInfo.diRefreshRateMin = (UINT)pdm->dmDisplayFrequency;
         DisplayInfo.diRefreshRateMax = (UINT)pdm->dmDisplayFrequency;
      }
      DPF("External:Settings %dx%dx%d %d-%dHz [%dx%d]",
          (int)DisplayInfo.diXRes,
          (int)DisplayInfo.diYRes,
          (int)DisplayInfo.diBpp,
          (int)DisplayInfo.diRefreshRateMin,
          (int)DisplayInfo.diRefreshRateMax,
          (int)DisplayInfo.diXDesktopSize,
          (int)DisplayInfo.diYDesktopSize);
   }

        //
        // now validate the desktop
        //
   if (DisplayInfo.diXDesktopSize < (DWORD)DisplayInfo.diXRes ||
       DisplayInfo.diYDesktopSize < (DWORD)DisplayInfo.diYRes)
   {
      DisplayInfo.diXDesktopSize = DisplayInfo.diXRes;
      DisplayInfo.diYDesktopSize = DisplayInfo.diYRes;
   }
#endif //DEBUG
        //
        // If we are on the primary device, just vector off to the regular Enable()
        //
   if (pGLInfo->dwDeviceHandle == 1)
      return Enable1(lpDevice,style,lpDeviceName,lpOutput,lpStuff);
        //
        // to enable this HW, call to see if the chipset is supported.
        //
   if (HWEnable(ENABLE_ALL) == ENABLE_ERROR)
   {
      DPF("HWEnable failed");
      return FALSE;
   }
   f = CanHWRunAsSecondary();

   if (f)
   {
      DPF("ok on CanHWRunAsSecondary() call");
      size=Enable1(lpDevice,style,lpDeviceName,lpOutput,lpStuff);
      if (size == 0)
      {
         DPF("Enable: failed on %d call,InquireInfo=%d,EnableDevice=%d ",
                                                             style,InquireInfo,EnableDevice);
         HWEnable(ENABLE_NONE);
         return FALSE;
      }
      if (style == InquireInfo)
      {
         HWEnable(ENABLE_NONE);
      }
      else
      {
         HWEnable(ENABLE_MEM);   // leave memory access on if style == EnableDevice
      }
      return size;
   }
   else
   {
      DPF("failed on CanHWRunAsSecondary() call");
      HWEnable(ENABLE_NONE);
      return FALSE;
   }

}

#pragma optimize("", on)
/***************************************************************************
 *
 * ReEnable   called by GDI to ReEnable the device and set the video mode
 *
 ***************************************************************************/

UINT FAR PASCAL _loadds ReEnable(
                                LPVOID  lpDevice,
                                LPVOID  lpGDIINFO)
{
   UINT  f=FALSE;
   DPF("ReEnable");
   if (pGLInfo->dwDeviceHandle == 1) return ReEnable1(lpDevice,lpGDIINFO);

   if (HWEnable(ENABLE_ALL) != ENABLE_ERROR)
   {
      f=ReEnable1(lpDevice,lpGDIINFO);
   }
   HWEnable(ENABLE_MEM);
   return f;
}

/***************************************************************************
 *
 * Disable
 *
 ***************************************************************************/

UINT FAR PASCAL _loadds Disable(DIBENGINE FAR *pde)
{
   UINT  f=FALSE;
   DPF("Disable");
   if (pGLInfo->dwDeviceHandle == 1) return Disable1(pde);
   if (HWEnable(ENABLE_ALL) != ENABLE_ERROR)
   {
      f=Disable1(pde);
   }
   HWEnable(ENABLE_NONE);
   VDDCloseDevice(pGLInfo->dwDeviceHandle);
   pGLInfo->dwDeviceHandle = 0;
   return f;
}

/***************************************************************************
 *
 * ValidateMode
 *
 *  called by GDI to verify whether the screen size is supported
 *      We have to make sure screen size <= desktop size
 *      Actually system verifies it before calling here, I am not sure if we
 *      have to verify again.
 *
 * This code assumes that the driver will NOT be loaded and ValidateMode called on
 * the secondary driver.
 ***************************************************************************/

UINT FAR PASCAL _loadds ValidateMode(DISPVALMODE FAR *pdvm)
{
   UINT  result=0;
   DWORD dw;

   DPF("ValidateMode : mode = %dx%d,%dBpp", pdvm->dvmXRes, pdvm->dvmYRes, pdvm->dvmBpp);
   if (pGLInfo->dwDeviceHandle == 0)
      return VALMODE_YES;

   if ((dw = HWEnable(ENABLE_ALL)) != ENABLE_ERROR)
   {
      result=ValidateMode1(pdvm);
      if (result == VALMODE_YES) DPF("  Mode OK");
      else 
	  {
		  DPF("  Mode BAD");
	  }
      HWEnable(dw);
   }
   else
   {
      DPF("Hardware enable failure!");
   }

   return result;
}

#ifndef NO_VALIDATEDESK
/***************************************************************************
 *
 * ValidateDesk
 * Purpose:
 *
 *  called by GDI to determine whether the driver/board can enable the given
 *      desktop size
 *  Exit:
 *      VALMODE_YES (0) or reason code
 *
 ***************************************************************************/

UINT FAR PASCAL _loadds ValidateDesk(DISPVALMODE FAR *pdvm)
{
   UINT  result=0;
   DWORD dw;

   DPF("ValidateDesk");
   if (pGLInfo->dwDeviceHandle == 0)
      return ValidateDesk1(pdvm);
   if ((dw = HWEnable(ENABLE_ALL)) != ENABLE_ERROR)
   {
      result=ValidateDesk1(pdvm);
      HWEnable(dw);
   }
   return result;
}
#endif // NO_VALIDATEDESK

/***************************************************************************
 *
 * SetPalette
 *
 ***************************************************************************/

UINT FAR PASCAL _loadds SetPalette(
                                  UINT        start,
                                  UINT        count,
                                  DWORD FAR  *lpPalette)
{
   UINT rc=0;
   DWORD dw;

   DPF("SetPalette");
   if (pGLInfo->dwDeviceHandle == 1) return SetPalette1(start, count, lpPalette);

   if ((dw = HWEnable(ENABLE_VGA)) != ENABLE_ERROR)
   {
      rc=SetPalette1(start, count, lpPalette);
      HWEnable(dw);
   }
   return rc;
}

/***************************************************************************
 *
 * DDIGammaRamp
 *
 ***************************************************************************/

BOOL FAR PASCAL _loadds DDIGammaRamp(
                               LPVOID lpDevice,
                               UINT   bSet,
                               LPVOID lpGammaData)
{
    BOOL rc=FALSE;
    DWORD dw;
    
    DPF("DDIGammaRamp");
    if (pGLInfo->dwDeviceHandle == 1)
    {
        if (bSet)
            return SetGammaRamp1(lpGammaData);
        else
            return GetGammaRamp1(lpGammaData);
    }

    if ((dw = HWEnable(ENABLE_VGA)) != ENABLE_ERROR)
    {
        if (bSet)
            rc = SetGammaRamp1(lpGammaData);
        else
            rc = GetGammaRamp1(lpGammaData);
        HWEnable(dw);
    }
    return rc;
}

/***************************************************************************
 *
 * Control
 *
 * this is what GDI calls when a app calles Escape or ExtEscape
 * if you dont handle a escape make sure to pass it to the DIBENG
 *
 ***************************************************************************/

LONG FAR PASCAL _loadds Control(
                               DIBENGINE FAR * lpDevice,
                               UINT     function,
                               LPVOID      lpInput,
                               LPVOID      lpOutput)
{
   LONG rc=0;
   DWORD   dw;

   if (pGLInfo->dwDeviceHandle == 1) return Control1(lpDevice,function,lpInput,lpOutput);

   if ((dw = HWEnable(ENABLE_VGA)) != ENABLE_ERROR)
   {
      rc=Control1(lpDevice,function,lpInput,lpOutput);
      HWEnable(dw);
   }
   return rc;
}

#pragma optimize("", on)

/***************************************************************************
 * VDDCall
 ***************************************************************************/
#pragma optimize("gle", off)
DWORD VDDCall(DWORD dev, DWORD function, DWORD flags, LPVOID buffer, DWORD buffer_size)
{
   static DWORD   VDDEntryPoint=0;
   DWORD   result=0xFFFFFFFF;

   if (VDDEntryPoint == 0)
   {
      _asm
      {
         xor     di,di           ;set these to zero before calling
         mov     es,di           ;
         mov     ax,1684h        ;INT 2FH: Get VxD API Entry Point
         mov     bx,0ah          ;this is device code forVDD
         int     2fh             ;call the multiplex interrupt
         mov     word ptr VDDEntryPoint[0],di    ;
         mov     word ptr VDDEntryPoint[2],es    ;save the returned data
      }

      if (VDDEntryPoint == 0)
         return result;
   }
   _asm
   {
      _emit 66h _asm push si                       ; push esi
      _emit 66h _asm push di                       ; push edi
      _emit 66h _asm mov ax,word ptr function      ;eax = function
      _emit 66h _asm mov bx,word ptr dev           ;ebx = device
      _emit 66h _asm mov cx,word ptr buffer_size   ;ecx = buffer_size
      _emit 66h _asm mov dx,word ptr flags         ;edx = flags
      _emit 66h _asm xor di,di                     ; HIWORD(edi)=0
        les     di,buffer
        mov     si,es                               ;si=es
        call    dword ptr VDDEntryPoint             ;call the VDD's PM API
        cmp     ax,word ptr function
        je      fail
      _emit 66h _asm mov word ptr result,ax
fail: _emit 66h _asm pop di                        ; pop edi
      _emit 66h _asm pop si                        ; pop esi
   }

   return result;
}

//------------------------------------------------------------------------------------------
// Some useful stuff for PCI
//------------------------------------------------------------------------------------------
// * CreateSel
// * FreeSel
//------------------------------------------------------------------------------------------

#pragma optimize("", off)


static UINT CreateSel(DWORD base, DWORD limit)
{
    UINT Sel;

    Sel = AllocSelector(SELECTOROF((LPVOID)&pGLInfo->dwDeviceHandle));

    if (Sel == 0)
        return 0;

    SetSelectorBase(Sel, base);
                
    // SetSelectorLimit(FlatSel, -1);
    _asm    mov     ax,0008h            ; DPMI set limit
    _asm    mov     bx,Sel
    _asm    mov     dx,word ptr limit[0]
    _asm    mov     cx,word ptr limit[2]
    _asm    int     31h

    return Sel;
}
#pragma optimize("", on)

//------------------------------------------------------------------------------------------
static void FreeSel(UINT Sel)
{
    if (Sel)
    {
        SetSelectorLimit(Sel, 0);
        FreeSelector(Sel);
    }
}

//------------------------------------------------------------------------------------------
static UINT FAR PASCAL _loadds CreatePhysSel(DWORD PhysAddress, DWORD PhysSize)
{
    return CreateSel(PhysToLinear(PhysAddress, PhysSize), PhysSize-1);
}

//------------------------------------------------------------------------------------------
// * GetFlatSel
// * FreeFlatSel
//------------------------------------------------------------------------------------------


static UINT GetFlatSel(UINT FlatSel)
{
    if (FlatSel != 0)
        return FlatSel;

    FlatSel = CreateSel(0, 0xFFFFFFFF);
    return FlatSel;
}


static void FreeFlatSel(UINT FlatSel)
{
    if (FlatSel)
    {
        FreeSel(FlatSel);
        FlatSel = 0;
    }
}

//------------------------------------------------------------------------------------------
// * PhysToLinear
//------------------------------------------------------------------------------------------
#pragma optimize("", off)
static DWORD PhysToLinear(DWORD PhysAddress, DWORD PhysSize)
{
    DWORD LinearAddress;

    PhysSize = PhysSize-1;      // we want limit, not size for DPMI

    _asm
    {
        mov     cx, word ptr PhysAddress[0]
        mov     bx, word ptr PhysAddress[2]
        mov     di, word ptr PhysSize[0]
        mov     si, word ptr PhysSize[2]
        mov     ax, 0800h               ; DPMI phys to linear
        int     31h
        mov     word ptr LinearAddress[0], cx
        mov     word ptr LinearAddress[2], bx
    }

    return LinearAddress;
}

