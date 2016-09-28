
option expr32

; Begin of file p2info.h
MAXDMABUFF		EQU		10t
P2MAX_SUBBUFFERS		EQU		32t
SIZE_CONFIGURATIONBASE		EQU		32t
MAX_QUEUE_SIZE		EQU		000000022h
_VDDDISPLAYINFO		STRUCT 2t
ddiHdrSize		WORD		?
ddiInfoFlags		WORD		?
ddiDevNodeHandle		DWORD		?
ddiDriverName		BYTE		16t DUP (?)
ddiXRes		WORD		?
ddiYRes		WORD		?
ddiDPI		WORD		?
ddiPlanes		BYTE		?
ddiBpp		BYTE		?
ddiRefreshRateMax		WORD		?
ddiRefreshRateMin		WORD		?
ddiLowHorz		WORD		?
ddiHighHorz		WORD		?
ddiLowVert		WORD		?
ddiHighVert		WORD		?
ddiMonitorDevNodeHandle		DWORD		?
ddiHorzSyncPolarity		BYTE		?
ddiVertSyncPolarity		BYTE		?
diUnitNumber		DWORD		?
diDisplayFlags		DWORD		?
diXDesktopPos		DWORD		?
diYDesktopPos		DWORD		?
diXDesktopSize		DWORD		?
diYDesktopSize		DWORD		?
_VDDDISPLAYINFO		ENDS

VDDDISPLAYINFO		TYPEDEF		_VDDDISPLAYINFO

_GlintInfo		STRUCT 2t
dwDSBase		DWORD		?
dwpRegisters		DWORD		?
dwpFrameBuffer		DWORD		?
dwpLocalBuffer		DWORD		?
dwRegSel		DWORD		?
dwFBSel		DWORD		?
dwLBSel		DWORD		?
dwText32Selector		DWORD		?
dwRamDacType		DWORD		?
dwFlags		DWORD		?
dwDisconCtrl		DWORD		?
ddFBSize		DWORD		?
ddLBSize		DWORD		?
dwPartialProducts		DWORD		?
dwPartialProducts2		DWORD		?
dwPartialProducts4		DWORD		?
dwVGAEnable		DWORD		?
dwVGADisable		DWORD		?
dwVideoControl		DWORD		?
dwScreenBase		DWORD		?
dwOffscreenBase		DWORD		?
dwOSFontCacheStart		DWORD		?
dwHiResMode		DWORD		?
cBlockFillSize		BYTE		?
cLBDepth		BYTE		?
wRefreshRate		WORD		?
dwDeviceHandle		DWORD		?
dwVideoMemorySize		DWORD		?
dwScreenWidth		DWORD		?
dwScreenHeight		DWORD		?
dwBpp		DWORD		?
dwScreenWidthBytes		DWORD		?
dwCurrentOffscreen		DWORD		?
lpDriverPDevice		DWORD		?
pRegs		DWORD		?
bPixelToBytesShift		BYTE		?
bPixelToWordShift		BYTE		?
DisabledByGLDD		WORD		?
lpDDHAL_SetInfo		DWORD		?
dwRenderChipID		DWORD		?
dwRenderChipRev		DWORD		?
dwRenderFamily		DWORD		?
dwSupportChipID		DWORD		?
dwSupportChipRev		DWORD		?
dwBoardID		DWORD		?
dwBoardRev		DWORD		?
GlintBoardStatus		DWORD		?
VPE_VSPartialConfigA		DWORD		?
I2C_VSPartialConfigA		DWORD		?
VPE_VSPartialConfigB		DWORD		?
I2C_VSPartialConfigB		DWORD		?
dwDMABufferSize		DWORD		?
dwDMABufferPhys		DWORD		10t DUP (?)
dwDMABufferVirt		DWORD		10t DUP (?)
wDMABufferCount		WORD		?
wDMABufferAlloc		WORD		?
dw2D_DMA_Phys		DWORD		?
frontIndex		DWORD		?
backIndex		DWORD		?
endIndex		DWORD		?
maximumIndex		DWORD		?
QCommand		DWORD		34t DUP (?)
QAddress		DWORD		34t DUP (?)
QCount		DWORD		34t DUP (?)
pIntEnable		DWORD		?
pIntFlags		DWORD		?
InterruptPending		DWORD		?
InterruptCount		DWORD		?
dwVSAPolarity		DWORD		?
dwVSAPrevIndex		DWORD		?
pContext		DWORD		?
pContext16		WORD		?
pContextPad		WORD		?
dwCurrentContext		DWORD		?
dwFontCacheVirt		DWORD		?
dwFontCache16		DWORD		?
dwFontCacheSize		DWORD		?
lpColorTableOff		WORD		?
lpColorTableSeg		WORD		?
pColorTable		DWORD		?
dwCursorType		DWORD		?
wCursorControl		WORD		?
wCursorPad		WORD		?
RegistryConfigBase		BYTE		32t DUP (?)
DesktopDisplayInfo		VDDDISPLAYINFO		<>
_GlintInfo		ENDS

GlintInfo		TYPEDEF		_GlintInfo

LPGLINTINFO		TYPEDEF		PTR _GlintInfo

PERMEDIA2INFO		TYPEDEF		_GlintInfo

LPPERMEDIA2INFO		TYPEDEF		PTR _GlintInfo

_GlintGamma		STRUCT 2t
ValidGammaTable		WORD		?
GammaRampTable		WORD		768t DUP (?)
_GlintGamma		ENDS

GlintGamma		TYPEDEF		_GlintGamma

GLINT_GAMMA		TYPEDEF		_GlintGamma

MAX_P2INTERRUPT_COUNT		EQU		00000ffffh
; End of file p2info.h
