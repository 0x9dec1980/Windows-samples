##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1999
#       All Rights Reserved.
#
##########################################################################

#
# Assume No Error
#
!UNDEF CHECK_ERROR

!ifndef MASM_ROOT
!MESSAGE >
!MESSAGE > The Environment variable MASM_ROOT must be defined 
!MESSAGE > Please set this to the root of MASM 611d directory
!MESSAGE >
CHECK_ERROR=1
!endif

!ifndef C16_ROOT
!MESSAGE >
!MESSAGE > The Environment variable C16_ROOT must be defined 
!MESSAGE > Please set this to the root of Microsoft Visual C++ 1.52 compiler directory.  
!MESSAGE > 
CHECK_ERROR=1
!endif

!ifndef C32_ROOT
!MESSAGE >
!MESSAGE > The Environment variable C32_ROOT must be defined 
!MESSAGE > Please set this to DevStudio directory of Microsoft Visual C++ 5.0 compiler . 
!MESSAGE >
CHECK_ERROR=1
!endif

!ifndef DDKROOT
!MESSAGE >
!MESSAGE > The Environment variable DDKROOT must be defined 
!MESSAGE > Please set this to the root of Windows 98 DDK directory.  
!MESSAGE >
CHECK_ERROR=1
!endif

!ifndef DXSDKROOT
!MESSAGE >
!MESSAGE > The Environment variable DXSDKROOT must be defined
!MESSAGE > Please set this to the root of DX7SDK directory. 
!MESSAGE >
CHECK_ERROR=1
!endif

!ifndef DXDDKROOT
!MESSAGE >
!MESSAGE > The Environment variable DXDDKROOT must be defined 
!MESSAGE > Please set this to the root of DirectX7 DDK directory. 
!MESSAGE >
CHECK_ERROR=1
!endif

!ifndef SDKROOT
!MESSAGE >
!MESSAGE > The Environment variable SDKROOT must be defined 
!MESSAGE > Please set this to the root of Platform SDK directory. 
!MESSAGE >
CHECK_ERROR=1
!endif
#
# Was there an error?!?
#
!ifdef CHECK_ERROR
!error
!endif

