
option expr32

; Begin of file p3info.h
MAXDMABUFF		EQU		10t
MAX_SUBBUFFER		EQU		32t
SIZE_CONFIGURATIONBASE		EQU		32t
MAX_QUEUE_SIZE		EQU		000000022h
ULONG_PTR		TYPEDEF		DWORD

ULONG		TYPEDEF		DWORD

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

_P3_DMAPartition		STRUCT 2t
PhysAddr		DWORD		?
VirtAddr		ULONG_PTR		?
MaxAddress		ULONG_PTR		?
Locked		WORD		?
bStampedDMA		WORD		?
_P3_DMAPartition		ENDS

P3_DMAPartition		TYPEDEF		_P3_DMAPartition

_GlintInfo		STRUCT 2t
dwDevNode		DWORD		?
dwDSBase		DWORD		?
dwpRegisters		DWORD		?
dwpFrameBuffer		DWORD		?
dwpLocalBuffer		DWORD		?
dwRamDacType		DWORD		?
dwFlags		DWORD		?
ddFBSize		DWORD		?
dwScreenBase		DWORD		?
dwOffscreenBase		DWORD		?
bTVEnabled		DWORD		?
bTVPresent		DWORD		?
dwStreamCardType		DWORD		?
dwVSBLastAddressIndex		DWORD		?
dwBaseOffset		DWORD		?
dwMacroVision		DWORD		?
dwVideoMemorySize		DWORD		?
dwScreenWidth		DWORD		?
dwScreenHeight		DWORD		?
dwVideoWidth		DWORD		?
dwVideoHeight		DWORD		?
dwBpp		DWORD		?
dwScreenWidthBytes		DWORD		?
bPixelToBytesShift		BYTE		?
bPad1		BYTE		3t DUP (?)
pRegs		DWORD		?
PixelClockFrequency		DWORD		?
MClkFrequency		DWORD		?
dwRenderChipID		DWORD		?
dwRenderChipRev		DWORD		?
dwRenderFamily		DWORD		?
dwGammaRev		DWORD		?
dwTLChipID		DWORD		?
dwTLFamily		DWORD		?
dwSupportChipID		DWORD		?
dwSupportChipRev		DWORD		?
dwBoardID		DWORD		?
dwBoardRev		DWORD		?
DisabledByGLDD		WORD		?
wPad2		WORD		?
bDXDriverEnabled		DWORD		?
bDRAMBoard		DWORD		?
InterfaceType		DWORD		?
CurrentPartition		DWORD		?
NumberOfSubBuffers		DWORD		?
DMAPartition		P3_DMAPartition		32t DUP (<>)
CurrentBuffer		DWORD		?
dwDirectXState		ULONG_PTR		?
dwDMAFlushCount		DWORD		?
dw3DDMABufferSize		DWORD		?
dw3DDMABufferPhys		DWORD		?
dw3DDMABufferVirt		ULONG_PTR		?
frontIndex		DWORD		?
backIndex		DWORD		?
endIndex		DWORD		?
maximumIndex		DWORD		?
iSurfaceInfoBlocksAllocated		DWORD		?
dwVideoControl		DWORD		?
dwDeviceHandle		DWORD		?
szDeviceName		SBYTE		16t DUP (?)
dwCurrentContext		DWORD		?
GlintBoardStatus		DWORD		?
bOverlayEnabled		DWORD		?
bVBLANKUpdateOverlay		DWORD		?
VBLANKUpdateOverlayWidth		DWORD		?
VBLANKUpdateOverlayHeight		DWORD		?
dwVGAEnable		DWORD		?
dwVGADisable		DWORD		?
dwOSFontCacheStart		DWORD		?
dwHiResMode		DWORD		?
cBlockFillSize		BYTE		?
cLBDepth		BYTE		?
wRefreshRate		WORD		?
lpDriverPDevice		DWORD		?
dwCurrentOffscreen		DWORD		?
bPixelToWordShift		BYTE		?
bPad3		BYTE		3t DUP (?)
lpDDHAL_SetInfo		DWORD		?
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
dwFontCacheVirt		DWORD		?
dwFontCache16		DWORD		?
dwFontCacheSize		DWORD		?
lpColorTableOff		WORD		?
lpColorTableSeg		WORD		?
pColorTable		DWORD		?
dwCursorType		DWORD		?
wCursorMode		WORD		?
wCursorPad		WORD		?
RegistryConfigBase		BYTE		32t DUP (?)
dwRegSel		DWORD		?
dwFBSel		DWORD		?
dwLBSel		DWORD		?
dwText32Selector		DWORD		?
dwDisconCtrl		DWORD		?
ddLBSize		DWORD		?
dwPartialProducts		DWORD		?
dwPartialProducts2		DWORD		?
dwPartialProducts4		DWORD		?
DesktopDisplayInfo		VDDDISPLAYINFO		<>
_GlintInfo		ENDS

GlintInfo		TYPEDEF		_GlintInfo

LPGLINTINFO		TYPEDEF		FAR PTR _GlintInfo

PERMEDIA2INFO		TYPEDEF		_GlintInfo

LPPERMEDIA2INFO		TYPEDEF		FAR PTR _GlintInfo

_GlintGamma		STRUCT 2t
ValidGammaTable		WORD		?
GammaRampTable		WORD		768t DUP (?)
_GlintGamma		ENDS

GlintGamma		TYPEDEF		_GlintGamma

GLINT_GAMMA		TYPEDEF		_GlintGamma

MAX_P2INTERRUPT_COUNT		EQU		00000ffffh
; End of file p3info.h
