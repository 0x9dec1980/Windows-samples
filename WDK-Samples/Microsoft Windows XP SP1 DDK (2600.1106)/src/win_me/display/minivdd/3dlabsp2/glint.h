/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: glint.h
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include <vmm.h>
#pragma warning (disable:4229)
#include <shell.h>
#pragma warning (default:4229)
#include <vpowerd.h>
#include <vpicd.h>
#include <vxdwraps.h>
#include <configmg.h>

#ifndef DDK_DX5_BETA1
#include <ddkmmini.h>
#endif

#define __far
#define far
#define FAR
#ifndef __SOFTCOPY
#include "softcopy.h"
#endif
#include "glglobal.h"
#include "glintreg.h"
#include "gldebug.h"
#define INCLUDE_DELAY 1
#include "ramdac.h"

#define MAX_GLINT_DEVICES 8
#define SIZE_INFORMATION_KEY (SIZE_CONFIGURATIONBASE+18)

// default clock speed in MHz for the reference board. The actual speed
// is looked up in the registry. Use this if no registry entry is found
// or the registry entry is zero..
//
#define GLINT_DEFAULT_CLOCK_SPEED_SXREV1		40
#define GLINT_DEFAULT_CLOCK_SPEED_TXANDSXREV2	50
#define GLINT_DEFAULT_CLOCK_SPEED_DELTA			40

//
// Racer Pro status register definition masks
//

#define  STAT_TWO_BANKS       0x01
#define  STAT_PAGE_SIZE       0x06
#define  STAT_EXT_TMG_GEN     0x08
#define  STAT_INDP_OVERLAYS   0x10
#define  STAT_DAUGHTER_ID     0xE0


typedef struct {
    struct GlintReg     Glint;
}   *PREGISTERS; 

#define PCI_TYPE0_ADDRESSES             6
#define PCI_TYPE1_ADDRESSES             2

typedef struct _PCI_COMMON_CONFIG {
    USHORT  VendorID;                   // (ro)
    USHORT  DeviceID;                   // (ro)
    USHORT  Command;                    // Device control
    USHORT  Status;
    UCHAR   RevisionID;                 // (ro)
    UCHAR   ProgIf;                     // (ro)
    UCHAR   SubClass;                   // (ro)
    UCHAR   BaseClass;                  // (ro)
    UCHAR   CacheLineSize;              // (ro+)
    UCHAR   LatencyTimer;               // (ro+)
    UCHAR   HeaderType;                 // (ro)
    UCHAR   BIST;                       // Built in self test

    union {
        struct _PCI_HEADER_TYPE_0 {
            ULONG   BaseAddresses[PCI_TYPE0_ADDRESSES];
            ULONG   CardBusCISPointer;
			USHORT	SubSystemVendorID, SubSystemID;
            ULONG   ROMBaseAddress;
            ULONG   Reserved2[2];

            UCHAR   InterruptLine;      //
            UCHAR   InterruptPin;       // (ro)
            UCHAR   MinimumGrant;       // (ro)
            UCHAR   MaximumLatency;     // (ro)
        } type0;


    } u;

} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;


// The first few registers are 
typedef struct _DEVTABLE {
    // Must mirror assembler structure
	PREGISTERS          pRegisters;           
	LPGLINTINFO			pGLInfo;
    HIRQ			    IRQHandle;
    // End of mirroring
	DWORD				*pFrameBuffer;        
	DWORD				*pLocalBuffer;
    DWORD               DataSegmentBase;
    DWORD               DataSegment;
	DEVNODE				DevNode;
	DEVNODE				ActualDevNode;
    struct _DEVTABLE    *pDeltaDev;
	DWORD				RevisionID;
	DWORD				dwFlags;
#ifdef POWER_STATE_MANAGE
	DWORD				dwPowerState;	
#endif //#ifdef POWER_STATE_MANAGE
	DWORD				ClkSpeed;
    DWORD               dwVSConfiguration;
    DWORD               dwMemControl;
    DWORD               dwBootAddress;
    DWORD               dwMemConfig;
    DWORD               dwFifoControl;
    DWORD               dwVClkCtl;
	DWORD				dwFrameBufferSize;
	DWORD				dwLocalBufferSize;
	DWORD				dwLocalBufferDepth;
	DWORD				dw2D_DMA_Phys;
	WORD				ChipID;
    WORD                FamilyID;
	WORD				wBoardID;
    WORD				DacId;
    WORD				ActualDacId;
    WORD				PCIBus;
    WORD				PCIDevice;
    WORD				PCIFunction;
	PCI_COMMON_CONFIG	PCIConfig;
	CMCONFIG			CMConfig;
	char				szInformationKey[SIZE_INFORMATION_KEY];
} DEVICETABLE, *PDEVICETABLE;

#define PCI_COMMON_HDR_LENGTH (FIELD_OFFSET (PCI_COMMON_CONFIG, DeviceSpecific))

#define PCI_MAX_DEVICES                     32
#define PCI_MAX_FUNCTION                    8

#define PCI_INVALID_VENDORID                0xFFFF

//
// Bit encodings for  PCI_COMMON_CONFIG.HeaderType
//

#define PCI_MULTIFUNCTION                   0x80
#define PCI_DEVICE_TYPE                     0x00
#define PCI_BRIDGE_TYPE                     0x01

//
// Bit encodings for PCI_COMMON_CONFIG.Command
//

#define PCI_ENABLE_IO_SPACE                 0x0001
#define PCI_ENABLE_MEMORY_SPACE             0x0002
#define PCI_ENABLE_BUS_MASTER               0x0004
#define PCI_ENABLE_SPECIAL_CYCLES           0x0008
#define PCI_ENABLE_WRITE_AND_INVALIDATE     0x0010
#define PCI_ENABLE_VGA_COMPATIBLE_PALETTE   0x0020
#define PCI_ENABLE_PARITY                   0x0040  // (ro+)
#define PCI_ENABLE_WAIT_CYCLE               0x0080  // (ro+)
#define PCI_ENABLE_SERR                     0x0100  // (ro+)
#define PCI_ENABLE_FAST_BACK_TO_BACK        0x0200  // (ro)

//
// Bit encodings for PCI_COMMON_CONFIG.Status
//

#define PCI_STATUS_FAST_BACK_TO_BACK        0x0080  // (ro)
#define PCI_STATUS_DATA_PARITY_DETECTED     0x0100
#define PCI_STATUS_DEVSEL                   0x0600  // 2 bits wide
#define PCI_STATUS_SIGNALED_TARGET_ABORT    0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT    0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT    0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR    0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR    0x8000


//
// Bit encodes for PCI_COMMON_CONFIG.u.type0.BaseAddresses
//

#define PCI_ADDRESS_IO_SPACE                0x00000001  // (ro)
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x00000006  // (ro)
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x00000008  // (ro)

#define PCI_TYPE_32BIT      0
#define PCI_TYPE_20BIT      2
#define PCI_TYPE_64BIT      4

//
// Bit encodes for PCI_COMMON_CONFIG.u.type0.ROMBaseAddresses
//

#define PCI_ROMADDRESS_ENABLED              0x00000001

typedef struct {
    unsigned long           dwDMABufferSize;// size of dma buffers
    unsigned long           dwDMABufferPhys[MAXDMABUFF]; // physical addresses of buffers
    unsigned long           dwDMABufferVirt[MAXDMABUFF]; // virtual ring 0 addresses
    unsigned short          wDMABufferCount;// count of dma buffers
    unsigned short          wDMABufferAlloc;// mask of available dma buffers
    unsigned long           dwFontCacheVirt;// 32 bit virtual address
    unsigned long           dwFontCache16;  // 16 bit segment for font cache
    unsigned long           dwFontCacheSize;// No of pages in font cache
}	DMAINFO, *PDMAINFO;


extern DEVICETABLE	DevTable[MAX_GLINT_DEVICES];
extern PDEVICETABLE pDevWithVGAEnabled;
extern DWORD        WindowsVMHandle;

extern PDEVICETABLE _cdecl InitialiseDynamicInit(ULONG DevNode);
extern PDEVICETABLE _cdecl InitialiseHardware(PDEVICETABLE pDev, BOOL Destructive);
extern DWORD _cdecl InitialiseMode(PDEVICETABLE pDev);
extern DWORD _cdecl ReinitialiseMode(void);
extern DWORD _cdecl InitialiseSharedData(PDEVICETABLE pDev);
extern DWORD _cdecl InitialiseVideo(PDEVICETABLE pDev);
extern DWORD _cdecl InitialiseDisplay(PDEVICETABLE pDev);
extern DWORD _cdecl SetDPMS(PDEVICETABLE pDev, DWORD Mode);
extern DWORD _cdecl GetDPMS(PDEVICETABLE pDev);
extern DWORD _cdecl EnableVGA(PDEVICETABLE pDev);  // 
extern DWORD _cdecl DisableVGA(void);
extern DWORD _cdecl DisableHardware(void);
extern DWORD _cdecl VesaSupport(DWORD, CLIENT_STRUCT*);
extern DWORD _cdecl TurnVGAOn(DWORD, CLIENT_STRUCT*);
extern DWORD _cdecl TurnVGAOff(DWORD, CLIENT_STRUCT*);

#ifndef DDK_DX5_BETA1
extern DWORD _cdecl vddGetIRQInfo(DWORD dwDevHandle, DDGETIRQINFO* pGetIRQ);
extern DWORD _cdecl vddIsOurIRQ(DWORD dwDevHandle);
extern DWORD _cdecl vddEnableIRQ(DWORD dwDevHandle, DDENABLEIRQINFO* pEnableInfo);
extern DWORD _cdecl vddFlipVideoPort(DWORD dwDevHandle, DDFLIPVIDEOPORTINFO* pFlipInfo);
extern DWORD _cdecl vddGetCurrentAutoflip(DWORD dwDevHandle, DDGETAUTOFLIPININFO* pGetIn, DDGETAUTOFLIPOUTINFO* pGetOut);
extern DWORD _cdecl vddGetPreviousAutoflip(DWORD dwDevHandle, DDGETAUTOFLIPININFO* pGetIn, DDGETAUTOFLIPOUTINFO* pGetOut);

extern DWORD _cdecl MiniVDD_GetIRQInfo(void);
extern DWORD _cdecl MiniVDD_IsOurIRQ(void);
extern DWORD _cdecl MiniVDD_EnableIRQ(void);
extern DWORD _cdecl MiniVDD_FlipVideoPort(void);
extern DWORD _cdecl MiniVDD_GetCurrentAutoflip(void);
extern DWORD _cdecl MiniVDD_GetPreviousAutoflip(void);
extern DWORD _cdecl MiniVDD_FlipOverlay(void);
extern DWORD _cdecl MiniVDD_Lock(void);
extern DWORD _cdecl MiniVDD_SetState(void);
extern DWORD _cdecl MiniVDD_GetFieldPolarity(void);
extern DWORD _cdecl MiniVDD_SkipNextField(void);
#endif

extern DWORD Vesa_DDC(PDEVICETABLE pDev, CLIENT_STRUCT *Regs, DWORD VirtualMachine);
extern VOID  ResetChip(PDEVICETABLE pDeviceTable);
extern PDEVICETABLE InitialiseHardwareNode(PDEVICETABLE pDeviceTable, ULONG DevNode);
extern DWORD LocateHardwareNode(PDEVICETABLE pDeviceTable);
extern DWORD InitialiseRegistry(PDEVICETABLE pDev);
extern DWORD InitialiseInterrupts(PDEVICETABLE pDev);
extern DWORD InitialiseDMA(PDEVICETABLE pDev);
extern DWORD InitialiseChip(PDEVICETABLE pDev);
extern DWORD MapInitAndSizeMemoryRegions(PDEVICETABLE pDev, PDEVICETABLE pDeltaDev, BOOL Destructive);
extern DWORD FindRamDacType(PDEVICETABLE pDev);
extern DWORD ZeroMemoryRegion(VOID *pMemory, ULONG Size);
extern PDEVICETABLE FindpDevFromDevNode(DEVNODE DevNode);
extern PDEVICETABLE _cdecl FindpDevFromDevNodeTable(DEVNODE DevNode); // 
extern PDEVICETABLE _cdecl FindPrimarypDev(void);
extern DWORD _cdecl FindDevNodeForCurrentlyActiveVGA(void);
extern DWORD GetBusDevFnNumbers(PCHAR, PDWORD, PDWORD, PDWORD);

extern DWORD _cdecl AllocateGlobalMemory(DWORD Size);
extern DWORD _cdecl FreeGlobalMemory(DWORD Selector);

extern DWORD _cdecl HandleInterrupt(PDEVICETABLE pDev);	//
extern PVOID _cdecl MapFlat(ULONG, ULONG); 
extern BOOL __cdecl ShellBroadcastCallback(DWORD uMsg, DWORD wParam, DWORD lParam, PDEVICETABLE pDev);

// I2C utility functions
void I2CSendBit(PDEVICETABLE pDev, BOOL Data);
void I2CStart(PDEVICETABLE pDev);
void I2CStop(PDEVICETABLE pDev);
void I2CSendByte(PDEVICETABLE pDev, BYTE data);
BOOL I2CSendDataByte(PDEVICETABLE pDev, BYTE data);
void I2CGetBit(PDEVICETABLE pDev, BOOL* data);
void I2CGetByte(PDEVICETABLE pDev, BYTE* data);
void I2CGetDataByte(PDEVICETABLE pDev, BYTE  *data);
void I2CGetDevice(PDEVICETABLE pDev, WORD wUnit, BYTE* pData);
void I2CSendSeq(PDEVICETABLE pDev, WORD wUnit, BYTE num, BYTE* data);
void I2CReceiveSeq(PDEVICETABLE pDev, WORD wUnit, BYTE StartAddress, BYTE num, BYTE* data);
void I2CInitBus(PDEVICETABLE pDev);

extern void RegistryLog(PDEVICETABLE pDev);

// Defined in glintmvd.asm
extern WORD _cdecl AllocateSelector(PVOID, ULONG); 
extern WORD _cdecl InitialisePrimaryDispatchTable(void); 
extern WORD _cdecl InitialiseSecondaryDispatchTable(void); 
extern WORD _cdecl FreeSelector(DWORD); 
extern VOID* _cdecl SelectorMapFlat(DWORD); 
extern PVOID _cdecl MiniVDD_Hw_Int(void);

// Write-combined DMA buffers support

extern BOOL TestWCCapability( void );
extern BOOL SetFixedMTRRToWC( unsigned long baseAddr, unsigned long nPages );
extern BOOL SetVariableMTRRToWC( unsigned long baseAddr, unsigned long nBytes );
extern VOID _cdecl ReadMSR( DWORD regno, unsigned long* loPart, unsigned long* hiPart );
extern VOID _cdecl WriteMSR( DWORD regno, unsigned long* loPart, unsigned long* hiPart );
extern DWORD _cdecl GetCPUID( void );
extern DWORD _cdecl HasCPUID( void );

#define LD_GLINT_REG(reg, val)			(pDev->pRegisters->Glint.reg = val)

#define LOAD_GLINT_CTRL_REG(reg, val)	(pDev->pRegisters->Glint.reg = val)
#define READ_GLINT_CTRL_REG(reg, val)	(val = pDev->pRegisters->Glint.reg)

#define WAIT_GLINT_FIFO(num) 			{ while (pDev->pRegisters->Glint.InFIFOSpace < num); }
#define GET_DMA_COUNT(num)				(num = pDev->pRegisters->Glint.DMACount)

#define ASSERTDD(x, y)
#define WAIT_DMA_COMPLETE \
{ \
    if ((pDev->pGLInfo) && !(pDev->pGLInfo->GlintBoardStatus & GLINT_DMA_COMPLETE)) {	\
        volatile LONG count;									\
        if (pDev->pGLInfo->GlintBoardStatus & GLINT_INTR_CONTEXT) {	\
            while (pDev->pGLInfo->frontIndex != pDev->pGLInfo->backIndex);	\
        }														\
		if ((count = pDev->pRegisters->Glint.DMACount) > 0)			\
		{														\
			do { while(--count > 0);							\
			} while ((count = pDev->pRegisters->Glint.DMACount) > 0);	\
		}														\
        pDev->pGLInfo->GlintBoardStatus |= GLINT_DMA_COMPLETE;		\
    }															\
}

#define SYNC_WITH_GLINT {                                               \
	WAIT_DMA_COMPLETE													\
	while( pDev->pRegisters->Glint.InFIFOSpace < 2 ) /* void */ ;				\
    LD_GLINT_REG(FilterMode, 0x400);									\
    LD_GLINT_REG(Sync, 0);												\
	LD_GLINT_REG(FilterMode, 0x0);   									\
    do {                                                                \
	    while (pDev->pRegisters->Glint.OutFIFOWords == 0) /* void */ ;        \
    } while (pDev->pRegisters->Glint.GPFifo[0] != 0x188);                     \
}
