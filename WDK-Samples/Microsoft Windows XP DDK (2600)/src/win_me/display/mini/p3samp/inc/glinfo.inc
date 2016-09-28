
option expr32

; Begin of file glinfo.h
MAXCONTEXT		EQU		128t
MAX_SUBBUFFERS		EQU		32t
PERMEDIA_REV1_BAD_READBACK_REGISTER_SPACE		EQU		12t
GLINT_DMA		EQU		1t
GLINT_NON_DMA		EQU		2t
GLINT_FAKE_DMA		EQU		8t
VENDOR_ID_3DLABS		EQU		000003d3dh
VENDOR_ID_TI		EQU		00000104ch
NOCHIP_ID		EQU		000000000h
GLINT300SX_ID		EQU		000000001h
GLINT500TX_ID		EQU		000000002h
DELTA_ID		EQU		000000003h
PERMEDIA_ID		EQU		000000004h
TIPERMEDIA_ID		EQU		000003d04h
GLINTMX_ID		EQU		000000006h
PERMEDIA2_ID		EQU		000000007h
PERMEDIA3_ID		EQU		00000000ah
GLINT_R3_ID		EQU		00000000bh
TIPERMEDIA2_ID		EQU		000003d07h
UNKNOWN_DEVICE_ID		EQU		00000ffffh
GLINT_ID		EQU		000000001h
PERMEDIA_SUBSYSTEM_ID		EQU		000000096h
PERMEDIA_NT_SUBSYSTEM_ID		EQU		000000098h
PERMEDIA_LC_SUBSYSTEM_ID		EQU		000000099h
PERMEDIA2_SUBSYSTEM_ID		EQU		0000000a0h
SOFTWARECURSOR		EQU		0t
HARDWARECURSOR		EQU		1t
BOARD_MONTSERRAT		EQU		000000001h
BOARD_RACER		EQU		000000002h
BOARD_RACERPRO		EQU		000000003h
BOARD_PERMEDIA		EQU		000000096h
BOARD_PERMEDIA_DELTA		EQU		000000098h
BOARD_PERMEDIA_LC		EQU		000000099h
BOARD_PERMEDIA2		EQU		0000000a0h
BOARD_UNKNOWN		EQU		00000ffffh
GLINT_GETVERSION		EQU		1t
GLINT_IOCTL_ADD_CONTEXT_MEMORY		EQU		2t
GLINT_MEMORY_REQUEST		EQU		3t
GLINT_16TO32_POINTER		EQU		4t
GLINT_I2C		EQU		5t
GLINT_MEMORY_ALLOCATE		EQU		1t
GLINT_MEMORY_FREE		EQU		2t
tagALLOCREQUEST		STRUCT 2t
dwSize		DWORD		?
dwDevNode		DWORD		?
dwFlags		DWORD		?
dwBytes		DWORD		?
ptr16		DWORD		?
ptr32		DWORD		?
tagALLOCREQUEST		ENDS

ALLOCREQUEST		TYPEDEF		tagALLOCREQUEST

LPALLOCREQUEST		TYPEDEF		PTR tagALLOCREQUEST

GLINT_I2C_READ		EQU		0t
GLINT_I2C_WRITE		EQU		1t
GLINT_I2C_RESET		EQU		2t
GLINT_I2C_DEVICE_PRESENT		EQU		3t
tagI2CREQUEST		STRUCT 2t
dwOperation		DWORD		?
wSlaveAddress		WORD		?
NumItems		BYTE		?
Data		BYTE		256t DUP (?)
dwReserved1		DWORD		?
dwReserved2		DWORD		?
dwReserved3		DWORD		?
dwReserved4		DWORD		?
tagI2CREQUEST		ENDS

I2CREQUEST		TYPEDEF		tagI2CREQUEST

LPI2CREQUEST		TYPEDEF		PTR tagI2CREQUEST

GLINT_VMI_READ		EQU		0t
GLINT_VMI_WRITE		EQU		1t
tagVMIREQUEST		STRUCT 2t
dwOperation		DWORD		?
dwRegister		DWORD		?
dwCommand		DWORD		?
dwReserved1		DWORD		?
dwReserved2		DWORD		?
dwReserved3		DWORD		?
dwReserved4		DWORD		?
tagVMIREQUEST		ENDS

VMIREQUEST		TYPEDEF		tagVMIREQUEST

LPVMIREQUEST		TYPEDEF		PTR tagVMIREQUEST

CONTEXT_GENERIC		EQU		0t
CONTEXT_GLINT300SX		EQU		1t
CONTEXT_GLINT500TX		EQU		2t
CONTEXT_DELTA		EQU		3t
CONTEXT_PERMEDIA		EQU		4t
CONTEXT_GLINTMX		EQU		6t
CONTEXT_PERMEDIA2		EQU		7t
CONTEXT_GLINT_FAMILY		EQU		000004000h
CONTEXT_PERMEDIA_FAMILY		EQU		000004001h
CONTEXT_ENDOFBLOCK		EQU		00000ffffh
CONTEXT_TEMPLATE_DISPLAY_HANDLE		EQU		0t
CONTEXT_TEMPLATE_DIRECTDRAW_HANDLE		EQU		1t
CONTEXT_TEMPLATE_ALLREADABLE_HANDLE		EQU		2t
CONTEXT_TEMPLATE_DIRECT3D_HANDLE		EQU		3t
CONTEXT_DISPLAY_HANDLE		EQU		4t
CONTEXT_DIRECTDRAW_HANDLE		EQU		5t
CONTEXT_NONE		EQU		00000ffffh
MAX_CONTEXTS_IN_BLOCK		EQU		32t
NPAGES_IN_CONTEXT_BLOCK		EQU		6t
NO_COMMAND		EQU		0t
COLOR_SPACE_BUFFER_0		EQU		1t
COLOR_SPACE_BUFFER_1		EQU		2t
GLINT_RACER_BUFFER_0		EQU		3t
GLINT_RACER_BUFFER_1		EQU		4t

VBLANK_CONTROL_COMMAND		TYPEDEF		SWORD

DMA_INTERRUPT_AVAILABLE		EQU		1t
VBLANK_INTERRUPT_AVAILABLE		EQU		2t
SUSPEND_DMA_TILL_VBLANK		EQU		4t

INTERRUPT_CONTROL		TYPEDEF		SWORD

GLINT_SYNCED		EQU		000000001h
GLINT_DMA_COMPLETE		EQU		000000002h
GLINT_INTR_COMPLETE		EQU		000000004h
GLINT_INTR_CONTEXT		EQU		000000008h
SURFACE_GOURAUD		EQU		000000001h
SURFACE_ZENABLE		EQU		000000002h
SURFACE_SPECULAR		EQU		000000004h
SURFACE_FOGENABLE		EQU		000000008h
SURFACE_PERSPCORRECT		EQU		000000010h
SURFACE_TEXTURING		EQU		000000020h
SURFACE_ALPHAENABLE		EQU		000000040h
SURFACE_MONO		EQU		000000080h
SURFACE_WRAP_U		EQU		000000100h
SURFACE_WRAP_V		EQU		000000200h
SURFACE_ALPHASTIPPLE		EQU		000000400h
SURFACE_ZWRITEENABLE		EQU		000000800h
SURFACE_ENDPOINTENABLE		EQU		000001000h
SURFACE_ALPHACHROMA		EQU		000002000h
SURFACE_MIPMAPPING		EQU		000004000h
__ContextTable		STRUCT 2t
pD3DContext16		DWORD		32t DUP (?)
pD3DContext32		DWORD		32t DUP (?)
pNextContext		DWORD		?
pNextContext16		WORD		?
nInBlock		WORD		?
nUsed		WORD		?
FirstFree		WORD		?
nFree		WORD		?
COffset		WORD		32t DUP (?)
CSize		SWORD		32t DUP (?)
CTemplate		WORD		32t DUP (?)
CEndIndex		WORD		32t DUP (?)
__ContextTable		ENDS

CONTEXTTABLE		TYPEDEF		__ContextTable

PCONTEXTTABLE		TYPEDEF		PTR __ContextTable

@t_0		TYPEDEF		PTR DWORD
@t_1		TYPEDEF		PTR DWORD
@t_2		TYPEDEF		PTR DWORD
tagDMAPartition		STRUCT 2t
PhysAddr		@t_0		?
VirtAddr		@t_1		?
MaxAddress		@t_2		?
tagDMAPartition		ENDS

__DMAPartition		TYPEDEF		tagDMAPartition

lpDMAPartition		TYPEDEF		PTR tagDMAPartition

@t_3		TYPEDEF		PTR DWORD
_D3DContextHeader		STRUCT 2t
MagicNo		DWORD		?
ptr16		DWORD		?
ptr32		DWORD		?
InterfaceType		DWORD		?
DMABufferNum		DWORD		?
DMAPartition		__DMAPartition		32t DUP (<>)
NumberOfSubBuffers		DWORD		?
NumPartitions		SDWORD		?
CurrentPartition		DWORD		?
MaxSendSize		DWORD		?
PartitionSize		DWORD		?
CurrentBuffer		@t_3		?
AlphaBlendSend		DWORD		?
Flags		DWORD		?
FillMode		DWORD		?
KeptFog		DWORD		?
softCopyFake		DWORD		23t DUP (?)
_D3DContextHeader		ENDS

D3DCONTEXTHEADER		TYPEDEF		_D3DContextHeader

_att21505off		STRUCT 2t
WriteAddr1		BYTE		?
PixelColRam		BYTE		?
PixelRdMask		BYTE		?
ReadAdd1		BYTE		?
WriteAddr2		BYTE		?
CursorColRam		BYTE		?
Ctrl0		BYTE		?
ReadAdd2		BYTE		?
Ctrl1		BYTE		?
Ctrl2		BYTE		?
Status		BYTE		?
CursorPattern		BYTE		?
CursorXLow		BYTE		?
CursorXHigh		BYTE		?
CursorYLow		BYTE		?
CursorYHigh		BYTE		?
_att21505off		ENDS

ATT21505OFF		TYPEDEF		_att21505off

_DMAQueue		STRUCT 2t
dwContext		DWORD		?
dwSize		DWORD		?
dwPhys		DWORD		?
dwEvent		DWORD		?
_DMAQueue		ENDS

DMAQUEUE		TYPEDEF		_DMAQueue

_ContextRegs		STRUCT 2t
wNumRegs		WORD		?
wFirstReg		WORD		1t DUP (?)
_ContextRegs		ENDS

CONTEXTREGS		TYPEDEF		_ContextRegs

PM_CHIPCONFIG_AGPSIDEBAND		EQU		000000100h
PM_CHIPCONFIG_AGP1XCAPABLE		EQU		000000200h
PM_CHIPCONFIG_AGP2XCAPABLE		EQU		000000400h
PM_CHIPCONFIG_AGP4XCAPABLE		EQU		000000800h
G1_CHIPCONFIG_AGPSIDEBAND		EQU		000000002h
G1_CHIPCONFIG_AGP1XCAPABLE		EQU		000000001h
RamDacRGB525		EQU		1t
RamDacATT		EQU		2t
RamDacTVP3026		EQU		3t
BID_MONTSERRAT		EQU		0t
BID_RACER		EQU		1t
BID_ACCEL		EQU		2t
INTR_DISABLE_ALL		EQU		000000000h
INTR_ENABLE_DMA		EQU		000000001h
INTR_ENABLE_SYNC		EQU		000000002h
INTR_ENABLE_EXTERNAL		EQU		000000004h
INTR_ENABLE_ERROR		EQU		000000008h
INTR_ENABLE_VBLANK		EQU		000000010h
INTR_ENABLE_VIDSTREAM_B		EQU		000000100h
INTR_ENABLE_VIDSTREAM_A		EQU		000000200h
INTR_DMA_SET		EQU		000000001h
INTR_SYNC_SET		EQU		000000002h
INTR_EXTERNAL_SET		EQU		000000004h
INTR_ERROR_SET		EQU		000000008h
INTR_VBLANK_SET		EQU		000000010h
INTR_VIDSTREAM_B_SET		EQU		000000100h
INTR_VIDSTREAM_A_SET		EQU		000000200h
INTR_CLEAR_ALL		EQU		00000001fh
INTR_CLEAR_DMA		EQU		000000001h
INTR_CLEAR_SYNC		EQU		000000002h
INTR_CLEAR_EXTERNAL		EQU		000000004h
INTR_CLEAR_ERROR		EQU		000000008h
INTR_CLEAR_VBLANK		EQU		000000010h
INTR_CLEAR_VIDSTREAM_B		EQU		000000100h
INTR_CLEAR_VIDSTREAM_A		EQU		000000200h
GMVF_REV2		EQU		000000001h
GMVF_FFON		EQU		000000002h
GMVF_NOIRQ		EQU		000000004h
GMVF_SETUP		EQU		000000008h
GMVF_GCOP		EQU		000000010h
GMVF_DMAIP		EQU		000000020h
GMVF_POWERDOWN		EQU		000000040h
GMVF_P2FIFOUNDERRUN		EQU		000000080h
GMVF_DELTA		EQU		000000100h
GMVF_8BPPRGB		EQU		000000200h
GMVF_16BPPDB		EQU		000000400h
GMVF_SWCURSOR		EQU		000000800h
GMVF_INTCPTGDI		EQU		000001000h
GMVF_OFFSCRNBM		EQU		000002000h
GMVF_HWWRITEMASK		EQU		000004000h
GMVF_VBLANK_OCCURED		EQU		000010000h
GMVF_VBLANK_ENABLED		EQU		000020000h
GMVF_VSA_INTERRUPT_OCCURED		EQU		000040000h
GMVF_FRAME_BUFFER_IS_WC		EQU		000080000h
GMVF_EXPORT24BPP		EQU		010000000h
GMVF_DONOTRESET		EQU		020000000h
GMVF_TRYTOVIRTUALISE4PLANEVGA		EQU		040000000h
GMVF_VIRTUALISE4PLANEVGA		EQU		080000000h
@proto_0		TYPEDEF		PROTO C :PTR tagThunkedData, :PTR _GlintInfo, :DWORD, :PTR _d3d_context, :PTR _d3d_context
CreateContext		PROTO		@proto_0

@proto_1		TYPEDEF		PROTO C :PTR tagThunkedData, :PTR _GlintInfo, :DWORD
ChangeContext		PROTO		@proto_1

@proto_2		TYPEDEF		PROTO C :PTR tagThunkedData, :PTR _GlintInfo, :DWORD
DeleteContext		PROTO		@proto_2

@proto_3		TYPEDEF		PROTO C :PTR _GlintInfo, :DWORD, :WORD
SetEndIndex		PROTO		@proto_3

MINIVDD_SHAREGLINFO		EQU		000000000h
MINIVDD_INITIALISEMODE		EQU		000000001h
MINIVDD_GETGLINFO		EQU		000000002h
MINIVDD_ALLOCATEMEMORY		EQU		000000010h
MINIVDD_FREEMEMORY		EQU		000000011h
MINIVDD_READREGISTRY		EQU		000000020h
MINIVDD_WRITEREGISTRY		EQU		000000021h
MINIVDD_MEMORYREQUEST		EQU		000000030h
MINIVDD_I2CREQUEST		EQU		000000040h
MINIVDD_VMIREQUEST		EQU		000000041h
MINIVDD_ENABLEVGA		EQU		000000080h
REG_HKLM_PREFIX		EQU		000000001h
REG_HKU_PREFIX		EQU		000000002h
REG_HKCU_PREFIX		EQU		000000003h
REG_HKCR_PREFIX		EQU		000000004h
REG_HKCC_PREFIX		EQU		000000005h
REG_HKDD_PREFIX		EQU		000000006h
REG_DEVNODEDEFAULTPREFIX		EQU		000000007h
REG_DEVNODEPREFIX		EQU		000000008h
REGTYPE_STRING		EQU		000000100h
REGTYPE_BINARY		EQU		000000300h
REGTYPE_DWORD		EQU		000000400h
DATA_SEGMENT_OFFSET		EQU		000000000h
GLINT_REGISTERS_OFFSET		EQU		000010000h
DMA_BUFFER_2D		EQU		000030000h
DMA_BUFFER_3D		EQU		000038000h
FONT_CACHE_OFFSET		EQU		000100000h
FINAL_DATA_SEGMENT_SIZE		EQU		000200000h
DATA_SEGMENT_SIZE		EQU		000010000h
GLINT_REGISTERS_SIZE		EQU		000020000h
DMA_BUFFER_2D_SIZE		EQU		000008000h
DMA_BUFFER_3D_SIZE		EQU		0000c8000h
FONT_CACHE_SIZE		EQU		000100000h
D3D_DISABLED		EQU		1t
DRIVER_DISABLED		EQU		2t
DEFAULT_P2TEXTUREADDRESSMODE		EQU		000000001h
DEFAULT_P2TEXTUREREADMODE		EQU		00001760bh
DEFAULT_P2TEXTUREDATAFORMAT		EQU		000000200h
; End of file glinfo.h