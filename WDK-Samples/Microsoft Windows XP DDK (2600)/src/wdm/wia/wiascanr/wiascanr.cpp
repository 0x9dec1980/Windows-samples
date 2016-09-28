/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiascanr.cpp
*
*  VERSION:     1.0
*
*  DATE:        16 July, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA Sample scanner class factory and IUNKNOWN interface.
*
*******************************************************************************/

#include "pch.h"
#ifndef INITGUID
    #include <initguid.h>
#endif

#if !defined(dllexp)
#define DLLEXPORT __declspec( dllexport )
#endif

HINSTANCE g_hInst; // DLL module instance.

// {98B3790C-0D93-4f22-ADAF-51A45B33C998}
DEFINE_GUID(CLSID_SampleWIAScannerDevice,
0x98b3790c, 0xd93, 0x4f22, 0xad, 0xaf, 0x51, 0xa4, 0x5b, 0x33, 0xc9, 0x98);

/***************************************************************************\
*
*  CWIAScannerDeviceClassFactory
*
\****************************************************************************/

class CWIAScannerDeviceClassFactory : public IClassFactory
{
private:
    ULONG m_cRef;
public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP CreateInstance(IUnknown __RPC_FAR *pUnkOuter,REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);
    CWIAScannerDeviceClassFactory();
    ~CWIAScannerDeviceClassFactory();
};

/**************************************************************************\
* CWIAScannerDeviceClassFactory::CWIAScannerDeviceClassFactory(void)
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    None
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

CWIAScannerDeviceClassFactory::CWIAScannerDeviceClassFactory(void)
{
    m_cRef = 0;
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::~CWIAScannerDeviceClassFactory(void)
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    None
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

CWIAScannerDeviceClassFactory::~CWIAScannerDeviceClassFactory(void)
{
    
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::QueryInterface
*
*
*
* Arguments:
*
*   riid      -
*   ppvObject -
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDeviceClassFactory::QueryInterface(
    REFIID                      riid,
    void __RPC_FAR *__RPC_FAR   *ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::AddRef
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIAScannerDeviceClassFactory::AddRef(void)
{
    return ++m_cRef;
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::Release
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIAScannerDeviceClassFactory::Release(void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }
    return m_cRef;
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::CreateInstance
*
*
*
* Arguments:
*
*    punkOuter -
*    riid,     -
*    ppvObject -
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDeviceClassFactory::CreateInstance(
    IUnknown __RPC_FAR          *punkOuter,
    REFIID                      riid,
    void __RPC_FAR *__RPC_FAR   *ppvObject)
{    
    if (!IsEqualIID(riid, IID_IStiUSD) &&
        !IsEqualIID(riid, IID_IUnknown)) {
        return STIERR_NOINTERFACE;
    }

    // When created for aggregation, only IUnknown can be requested.
    if (punkOuter && !IsEqualIID(riid, IID_IUnknown)) {
        return CLASS_E_NOAGGREGATION;
    }

    HRESULT hr = S_OK;
    CWIAScannerDevice  *pDev = NULL;
    
    pDev = new CWIAScannerDevice(punkOuter);
    if (!pDev) {
        return STIERR_OUTOFMEMORY;
    }

    hr = pDev->PrivateInitialize();
    if(hr != S_OK) {
        delete pDev;
        return hr;
    }

    //  Move to the requested interface if we aren't aggregated.
    //  Don't do this if aggregated, or we will lose the private
    //  IUnknown and then the caller will be hosed.

    hr = pDev->NonDelegatingQueryInterface(riid,ppvObject);
    pDev->NonDelegatingRelease();

    return hr;
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::LockServer
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDeviceClassFactory::LockServer(BOOL fLock)
{    
    return NOERROR;
}

/**************************************************************************\
* CWIAScannerDevice::NonDelegatingQueryInterface
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::NonDelegatingQueryInterface(
    REFIID  riid,
    LPVOID  *ppvObj)
{
    HRESULT hr = S_OK;

    if (!IsValid() || !ppvObj) {
        return STIERR_INVALID_PARAM;
    }

    *ppvObj = NULL;

    if (IsEqualIID( riid, IID_IUnknown )) {
        *ppvObj = static_cast<INonDelegatingUnknown*>(this);
    }
    else if (IsEqualIID( riid, IID_IStiUSD )) {
        *ppvObj = static_cast<IStiUSD*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaMiniDrv )) {
        *ppvObj = static_cast<IWiaMiniDrv*>(this);
    }
    else {
        hr =  STIERR_NOINTERFACE;
    }

    if (SUCCEEDED(hr)) {
        (reinterpret_cast<IUnknown*>(*ppvObj))->AddRef();
    }

    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::NonDelegatingAddRef
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Object reference count.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIAScannerDevice::NonDelegatingAddRef(void)
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

/**************************************************************************\
* CWIAScannerDevice::NonDelegatingRelease
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Object reference count.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIAScannerDevice::NonDelegatingRelease(void)
{
    ULONG ulRef = 0;
    ulRef = InterlockedDecrement((LPLONG)&m_cRef);

    if (!ulRef) {
        delete this;
    }
    return ulRef;
}

/**************************************************************************\
* CWIAScannerDevice::QueryInterface
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::QueryInterface(REFIID riid, LPVOID* ppvObj)
{    
    return m_punkOuter->QueryInterface(riid,ppvObj);
}

/**************************************************************************\
* CWIAScannerDevice::AddRef
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIAScannerDevice::AddRef(void)
{
    return m_punkOuter->AddRef();
}

/**************************************************************************\
* CWIAScannerDevice::Release
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIAScannerDevice::Release(void)
{
    return m_punkOuter->Release();
}

/**************************************************************************\
* DllEntryPoint
*
*   Main library entry point. Receives DLL event notification from OS.
*
*       We are not interested in thread attaches and detaches,
*       so we disable thread notifications for performance reasons.
*
* Arguments:
*
*    hinst      -
*    dwReason   -
*    lpReserved -
*
* Return Value:
*
*    Returns TRUE to allow the DLL to load.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/


extern "C" DLLEXPORT BOOL APIENTRY DllEntryPoint(
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      lpReserved)
{    
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hInst = hinst;            
            DisableThreadLibraryCalls(hinst);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

/**************************************************************************\
* DllCanUnloadNow
*
*   Determines whether the DLL has any outstanding interfaces.
*
* Arguments:
*
*    None
*
* Return Value:
*
*   Returns S_OK if the DLL can unload, S_FALSE if it is not safe to unload.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

extern "C" STDMETHODIMP DllCanUnloadNow(void)
{
    return S_OK;
}

/**************************************************************************\
* DllGetClassObject
*
*   Create an IClassFactory instance for this DLL. We support only one
*   class of objects, so this function does not need to go through a table
*   of supported classes, looking for the proper constructor.
*
* Arguments:
*
*    rclsid - The object being requested.
*    riid   - The desired interface on the object.
*    ppv    - Output pointer to object.
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

extern "C" STDAPI DllGetClassObject(
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID      *ppv)
{    
    if (!ppv) {
        return ResultFromScode(E_FAIL);
    }

    if (!IsEqualCLSID(rclsid, CLSID_SampleWIAScannerDevice) ) {
        return ResultFromScode(E_FAIL);
    }

    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IClassFactory)) {
        return ResultFromScode(E_NOINTERFACE);
    }

    if (IsEqualCLSID(rclsid, CLSID_SampleWIAScannerDevice)) {
        CWIAScannerDeviceClassFactory *pcf = new CWIAScannerDeviceClassFactory;
        if (pcf) {
            *ppv = (LPVOID)pcf;
        }
    }
    return NOERROR;
}

