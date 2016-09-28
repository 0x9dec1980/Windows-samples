/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 3: miniVDD SAMPLE CODE     *
 *                           ***************************************
 *
 * Module Name: perm3.h
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#define GAMMA_CORRECTION    1

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


//
// Missing common typedef on Win9X
//

typedef BOOL            BOOLEAN;
typedef unsigned char  *LPBYTE;
typedef unsigned long   ULONG_PTR, *PULONG_PTR;

#define MAX_GLINT_DEVICES 8
#define SIZE_INFORMATION_KEY (SIZE_CONFIGURATIONBASE+18)


//
// Default clock speed in MHz for the reference board. The actual speed
// is looked up in the registry. Use this if no registry entry is found
// or the registry entry is zero..
//

#define GLINT_DEFAULT_CLOCK_SPEED_SXREV1        40
#define GLINT_DEFAULT_CLOCK_SPEED_TXANDSXREV2   50
#define GLINT_DEFAULT_CLOCK_SPEED_DELTA         40


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
} *PREGISTERS; 

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
            USHORT  SubSystemVendorID, SubSystemID;
            ULONG   ROMBaseAddress;
            ULONG   Reserved2[2];

            UCHAR   InterruptLine;      //
            UCHAR   InterruptPin;       // (ro)
            UCHAR   MinimumGrant;       // (ro)
            UCHAR   MaximumLatency;     // (ro)
        } type0;
    } u;

} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;


// default clock speed in Hz for the reference board. The actual speed
// is looked up in the registry. Use this if no registry entry is found
// or the registry entry is zero.
//

#define GLINT_DEFAULT_GAMMA_CLOCK_SPEED     (85 * (1000*1000))

#define PERMEDIA3_DEFAULT_CLOCK_SPEED       ( 80 * (1000*1000))
#define PERMEDIA3_DEFAULT_CLOCK_SPEED_ALT   ( 80 * (1000*1000))
#define PERMEDIA3_MAX_CLOCK_SPEED           (250 * (1000*1000))
#define PERMEDIA3_DEFAULT_MCLK_SPEED        ( 80 * (1000*1000))
#define PERMEDIA3_DEFAULT_SCLK_SPEED        ( 70 * (1000*1000))

//
// Maximum pixelclock values that the RAMDAC can handle (in 100's hz).
//

#define P3_MAX_PIXELCLOCK   2700000   // RAMDAC rated at 270 Mhz

//
// Maximum amount of video data per second, that the rasterizer
// chip can send to the RAMDAC (limited by SDRAM/SGRAM throuput).
//

#define P3_MAX_PIXELDATA    15000000  // 1500 million bytes/sec (in 100's bytes)

//
// Size of P3 card's BIOS ROM
//

#define ROM_MAPPED_LENGTH   0x10000

//
// The capabilities of a PXRX board. The Gamma capability is specified
// in the above enum, so I haven't duplicated it here.
//

typedef enum {
    PXRX_NOCAPS             = 0x00000000,    // No additional capabilities
    PXRX_SGRAM              = 0x00000001,    // SGRAM board (else SDRAM)
    PXRX_DFP                = 0x00000002,    // Digital flat panel
    PXRX_TVOUT              = 0x00000004,    // TV-out capabilities
    PXRX_STEREO             = 0x00000008,    // Stereo-glasses support
    PXRX_DFP_MON_ATTACHED   = 0x00000010,    // DFP Monitor is attached
    PXRX_EXTERNAL_CLOCK     = 0x00000020,    // External clock suppport
    PXRX_USE_BYTE_DOUBLING  = 0x00000040     // Current mode requires byte-doubling
} PXRX_CAPS;


////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to the GLINT display driver. They're
// put in the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to the display driver via an 'VIDEO_QUERY_AVAIL_MODES' or
// 'VIDEO_QUERY_CURRENT_MODE' IOCTL.
//
// NOTE: These definitions must match those in the GLINT display driver's
//       'driver.h'!

typedef enum {
    // NT4 uses the DeviceSpecificAttributes field so the low word is available
    CAPS_ZOOM_X_BY2         = 0x00000001,   // Hardware has zoomed by 2 in X
    CAPS_ZOOM_Y_BY2         = 0x00000002,   // Hardware has zoomed by 2 in Y
    CAPS_SPARSE_SPACE       = 0x00000004,   // Framebuffer is sparsely mapped 
                                            // (don't allow direct access). The machine
                                            // is probably an Alpha.
    CAPS_QUEUED_DMA         = 0x00000008,   // DMA address/count via the FIFO
    CAPS_LOGICAL_DMA        = 0x00000010,   // DMA through logical address table
    CAPS_USE_AGP_DMA        = 0x00000020,   // AGP DMA can be used.
    CAPS_P3RD_POINTER       = 0x00000040,   // Use the 3Dlabs P3RD RAMDAC
    CAPS_STEREO             = 0x00000080,   // Stereo mode enabled.
    CAPS_SW_POINTER         = 0x00010000,   // No hardware pointer; use software
                                            //  simulation
    CAPS_GLYPH_EXPAND       = 0x00020000,   // Use glyph-expand method to draw
                                            //  text.
    CAPS_RGB525_POINTER     = 0x00040000,   // Use IBM RGB525 cursor
    CAPS_FAST_FILL_BUG      = 0x00080000,   // chip fast fill bug exists
    CAPS_INTERRUPTS         = 0x00100000,   // interrupts supported
    CAPS_DMA_AVAILABLE      = 0x00200000,   // DMA is supported
    CAPS_TVP3026_POINTER    = 0x00400000,   // Use TI TVP3026 pointer
    CAPS_8BPP_RGB           = 0x00800000,   // Use RGB in 8bpp mode
    CAPS_RGB640_POINTER     = 0x01000000,   // Use IBM RGB640 cursor
    CAPS_DUAL_GLINT         = 0x02000000,   // Dual-TX board
    CAPS_GLINT2_RAMDAC      = 0x04000000,   // Second TX/MX attached to the RAMDAC
    CAPS_ENHANCED_TX        = 0x08000000,   // TX is in enhanced mode
    CAPS_ACCEL_HW_PRESENT   = 0x10000000,   // Accel Graphics Hardware
    CAPS_TVP4020_POINTER    = 0x20000000,   // Use the P2 pointer
    CAPS_SPLIT_FRAMEBUFFER  = 0x40000000,   // Dual-GLINT with a split framebuffer
    CAPS_P2RD_POINTER       = 0x80000000    // Use the 3Dlabs P2RD RAMDAC
} CAPS;


//
// The following 2 data structures are copied from ntddvdeo.h
//

//
// IOCTL_VIDEO_SET_COLOR_REGISTERS - Takes buffer containing VIDEO_CLUT.
//
// Information used by this function is passed using the following structure:
//

typedef struct _VIDEO_CLUTDATA {
    UCHAR Red;
    UCHAR Green;
    UCHAR Blue;
    UCHAR Unused;
} VIDEO_CLUTDATA, *PVIDEO_CLUTDATA;


//
// Highest valid DAC color register index.
//

#define VIDEO_MAX_COLOR_REGISTER  0xFF
#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * (VIDEO_MAX_COLOR_REGISTER+1)))


//
//Red - Bits to be put in the Red portion of the color registers.
//
//Green - Bits to be put in the Green portion of the color registers.
//
//Blue - Bits to be put in the Blue portion of the color registers.
//

typedef struct {
    USHORT   NumEntries;
    USHORT   FirstEntry;
    union {
        VIDEO_CLUTDATA RgbArray;
        ULONG RgbLong;
    } LookupTable[1];
} VIDEO_CLUT, *PVIDEO_CLUT;


// 
// The following are the definition for the LUT cache. The aim of the LUT cache
// is to stop sparkling from occurring, bu only writing those LUT entries that
// have changed to the chip, we can only do this by remembering what is already
// down there. The 'mystify' screen saver on P2 demonstrates the problem.
//

#define LUT_CACHE_INIT()        {VideoPortZeroMemory (&(pDev->LUTCache), sizeof (pDev->LUTCache));}
#define LUT_CACHE_SETSIZE(sz)    {pDev->LUTCache.LUTCache.NumEntries = (sz);}
#define LUT_CACHE_SETFIRST(frst){pDev->LUTCache.LUTCache.FirstEntry = (frst);}
#define LUT_CACHE_SETRGB(idx,zr,zg,zb) {    \
    pDev->LUTCache.LUTCache.LookupTable [idx].RgbArray.Red   = (UCHAR) (zr); \
    pDev->LUTCache.LUTCache.LookupTable [idx].RgbArray.Green = (UCHAR) (zg); \
    pDev->LUTCache.LUTCache.LookupTable [idx].RgbArray.Blue  = (UCHAR) (zb); \
}

//
// The LUT cache
//

typedef struct {
    VIDEO_CLUT      LUTCache;       // Header  plus 1 LUT entry
    VIDEO_CLUTDATA  LUTData [255];  // the other 255 LUT entries
} LUT_CACHE;


// The GeoTwin reserves the greatest number of regions:-
// [0] - gamma ctl [1] - LB (unused) [2] - Gamma multiGLINT aperture
// [3] - MX1 ctl   [4] - MX0 FB      [5] - MX1 FB
// [6] - P2 ctl    [7] - P2 apertur1 [8] - P2 aperture2
#define MAX_RESERVED_REGIONS 9

#define MAX_REGISTER_INITIALIZATION_TABLE_ENTRIES 10
#define MAX_REGISTER_INITIALIZATION_TABLE_ULONGS (2 * MAX_REGISTER_INITIALIZATION_TABLE_ENTRIES)


//
// The first few registers are 
//

typedef struct _DEVTABLE {

    // Must mirror assembler structure
    PREGISTERS          pRegisters;           
    LPGLINTINFO         pGLInfo;
    HIRQ                IRQHandle;
    // End of mirroring
    DWORD               *pFrameBuffer;        
    DWORD               *pLocalBuffer;
    DWORD               DataSegmentBase;
    DWORD               DataSegment;
    DEVNODE             DevNode;
    DEVNODE             ActualDevNode;
    struct _DEVTABLE    *pDeltaDev;
    DWORD               RevisionID;
    DWORD               dwFlags;
#ifdef POWER_STATE_MANAGE
    DWORD               dwPowerState;   
#endif //#ifdef POWER_STATE_MANAGE

    DWORD               ClkSpeed;

    DWORD               dwVSConfiguration;
    DWORD               dwMemControl;
    DWORD               dwBootAddress;
    DWORD               dwMemConfig;

    DWORD               dwFifoControl;
    DWORD               dwVClkCtl;

    DWORD               dwFrameBufferSize;
    DWORD               dwLocalBufferSize;
    DWORD               dwLocalBufferDepth;

    DWORD               dw2D_DMA_Phys;

    WORD                ChipID;
    WORD                FamilyID;
    WORD                wBoardID;

    WORD                DacId;
    WORD                ActualDacId;

    WORD                PCIBus;
    WORD                PCIDevice;
    WORD                PCIFunction;
    PCI_COMMON_CONFIG   PCIConfig;

    CMCONFIG            CMConfig;

    char                szInformationKey[SIZE_INFORMATION_KEY];

    //
    // New fields copied over from WIN2K miniport
    //

    ULONG               ChipClockSpeed;
    ULONG               ChipClockSpeedAlt;
    ULONG               GlintDeltaClockSpeed;
    ULONG               GlintGammaClockSpeed;
    ULONG               PXRXLastClockSpeed;
    ULONG               RefClockSpeed;

    ULONG               BiosVersionMajorNumber;
    ULONG               BiosVersionMinorNumber;

    // PXRX memory register
    ULONG               PXRXDefaultMemoryType;
    PXRX_CAPS           PXRXCapabilities;

    ULONG               bVGAEnabled;

    // Initialisation table
    ULONG               aulInitializationTable[MAX_REGISTER_INITIALIZATION_TABLE_ULONGS];
    ULONG               culTableEntries;

    // Extended BIOS initialisation variables
    BOOLEAN             bHaveExtendedClocks;
    ULONG               ulPXRXCoreClock;
    ULONG               ulPXRXMemoryClock;
    ULONG               ulPXRXMemoryClockSrc;
    ULONG               ulPXRXSetupClock;
    ULONG               ulPXRXSetupClockSrc;
    ULONG               ulPXRXGammaClock;
    ULONG               ulPXRXCoreClockAlt;

    ULONG               Capabilities;

    // LUT cache
    LUT_CACHE           LUTCache;

    ULONG               IntEnable;
    ULONG               VideoFifoControlCountdown;

    BOOLEAN             bVTGRunning;

    //
    // state save variables (for during power-saving)
    //

    ULONG               VideoControlMonitorON;
    ULONG               PreviousPowerState;
    ULONG               VideoFifoControl;

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
} DMAINFO, *PDMAINFO;


extern DEVICETABLE  DevTable[MAX_GLINT_DEVICES];
extern PDEVICETABLE pDevWithVGAEnabled;
extern DWORD        WindowsVMHandle;

extern PDEVICETABLE _cdecl InitializeDynamicInit(ULONG DevNode);
extern PDEVICETABLE _cdecl InitializeHardware(PDEVICETABLE pDev, BOOL Destructive);
extern DWORD _cdecl InitializeMode(PDEVICETABLE pDev);
extern DWORD _cdecl ReinitialiseMode(void);
extern DWORD _cdecl InitializeSharedData(PDEVICETABLE pDev);
extern DWORD _cdecl InitializeVideo(PDEVICETABLE pDev);
extern DWORD _cdecl InitializeDisplay(PDEVICETABLE pDev);
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
extern PDEVICETABLE InitializeHardwareNode(PDEVICETABLE pDeviceTable, ULONG DevNode);
extern DWORD LocateHardwareNode(PDEVICETABLE pDeviceTable);
extern DWORD InitializeRegistry(PDEVICETABLE pDev);
extern DWORD InitializeInterrupts(PDEVICETABLE pDev);
extern DWORD InitializeDMA(PDEVICETABLE pDev);
extern DWORD InitializeChip(PDEVICETABLE pDev);
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

extern DWORD _cdecl HandleInterrupt(PDEVICETABLE pDev); //
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
extern WORD _cdecl InitializePrimaryDispatchTable(void); 
extern WORD _cdecl InitializeSecondaryDispatchTable(void); 
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

#define LD_GLINT_REG(reg, val)          (pDev->pRegisters->Glint.reg = val)

#define LOAD_GLINT_CTRL_REG(reg, val)   (pDev->pRegisters->Glint.reg = val)
#define READ_GLINT_CTRL_REG(reg, val)   (val = pDev->pRegisters->Glint.reg)

#define WAIT_GLINT_FIFO(num)            { while (pDev->pRegisters->Glint.InFIFOSpace < num); }
#define GET_DMA_COUNT(num)              (num = pDev->pRegisters->Glint.DMACount)

#define ASSERTDD(x, y)

#define WAIT_DMA_COMPLETE                                                   \
{                                                                           \
    if ((pDev->pGLInfo) &&                                                  \
        (! (pDev->pGLInfo->GlintBoardStatus & GLINT_DMA_COMPLETE))) {       \
                                                                            \
        volatile LONG count;                                                \
                                                                            \
        if (pDev->pGLInfo->GlintBoardStatus & GLINT_INTR_CONTEXT) {         \
            while (pDev->pGLInfo->frontIndex != pDev->pGLInfo->backIndex);  \
        }                                                                   \
                                                                            \
        if ((count = pDev->pRegisters->Glint.DMACount) > 0) {               \
                                                                            \
            do {                                                            \
                                                                            \
                while(--count > 0) {                                        \
                } ;                                                         \
                                                                            \
            } while ((count = pDev->pRegisters->Glint.DMACount) > 0);       \
        }                                                                   \
                                                                            \
        pDev->pGLInfo->GlintBoardStatus |= GLINT_DMA_COMPLETE;              \
    }                                                                       \
}

#define SYNC_WITH_GLINT {                                                   \
    WAIT_DMA_COMPLETE                                                       \
    while( pDev->pRegisters->Glint.InFIFOSpace < 2 ) /* void */ ;           \
    LD_GLINT_REG(FilterMode, 0x400);                                        \
    LD_GLINT_REG(Sync, 0);                                                  \
    LD_GLINT_REG(FilterMode, 0x0);                                          \
    do {                                                                    \
        while (pDev->pRegisters->Glint.OutFIFOWords == 0) /* void */ ;      \
    } while (pDev->pRegisters->Glint.GPFifo[0] != 0x188);                   \
}


//
// Constants used to lock/unlock the VGA
//

#define PERMEDIA_VGA_CTRL_INDEX     5
#define PERMEDIA_VGA_ENABLE         (1 << 3)
#define PERMEDIA_VGA_STAT_VSYNC     (1 << 3)

#define PERMEDIA_VGA_LOCK_INDEX1    6
#define PERMEDIA_VGA_LOCK_INDEX2    7
#define PERMEDIA_VGA_LOCK_DATA1     0x0
#define PERMEDIA_VGA_LOCK_DATA2     0x0
#define PERMEDIA_VGA_UNLOCK_DATA1   0x3D
#define PERMEDIA_VGA_UNLOCK_DATA2   0xDB

#define GLINT_GAMMA_PRESENT         0

//
// Video timing related definitions
//

typedef struct {

    ULONG    width;
    ULONG    height;
    ULONG    refresh;

} MODE_INFO;

typedef struct {

    // UCHAR       Min;    // Min Refresh Rate
    // UCHAR       Max;    // Max Refresh Rate
    USHORT      HTot;   // Hor Total Time
    UCHAR       HFP;    // Hor Front Porch
    UCHAR       HST;    // Hor Sync Time
    UCHAR       HBP;    // Hor Back Porch
    UCHAR       HSP;    // Hor Sync Polarity
    USHORT      VTot;   // Ver Total Time
    UCHAR       VFP;    // Ver Front Porch
    UCHAR       VST;    // Ver Sync Time
    UCHAR       VBP;    // Ver Back Porch
    UCHAR       VSP;    // Ver Sync Polarity
    ULONG       pClk;   // Pixel clock

} VESA_TIMING_STANDARD;

//
// Extract timings from VESA structure in a form that can be programmed into
// the P2/P3 timing generator.
//

#define  GetHtotFromVESA(VESATmgs) ((VESATmgs)->HTot)
#define  GetHssFromVESA(VESATmgs)  ((VESATmgs)->HFP)
#define  GetHseFromVESA(VESATmgs)  ((VESATmgs)->HFP + (VESATmgs)->HST)
#define  GetHbeFromVESA(VESATmgs)  ((VESATmgs)->HFP + (VESATmgs)->HST + (VESATmgs)->HBP)
#define  GetHspFromVESA(VESATmgs)  ((VESATmgs)->HSP)
#define  GetVtotFromVESA(VESATmgs) ((VESATmgs)->VTot)
#define  GetVssFromVESA(VESATmgs)  ((VESATmgs)->VFP)
#define  GetVseFromVESA(VESATmgs)  ((VESATmgs)->VFP + (VESATmgs)->VST)
#define  GetVbeFromVESA(VESATmgs)  ((VESATmgs)->VFP + (VESATmgs)->VST + (VESATmgs)->VBP)
#define  GetVspFromVESA(VESATmgs)  ((VESATmgs)->VSP)

typedef struct {

    MODE_INFO               basic;
    VESA_TIMING_STANDARD    vesa;
} TIMING_INFO;

//
// Helper defined in perm3dat.c
//

BOOLEAN
GetVideoTiming(
    PDEVICETABLE            pDev,
    ULONG                   xRes,
    ULONG                   yRes,
    ULONG                   Freq,
    ULONG                   Depth,
    VESA_TIMING_STANDARD   *VESATimings);

//
// Helper functions defined in perm3.c
//

BOOLEAN 
ReadROMInfo(
    PDEVICETABLE            pDev);

LONG
ReadChipClockSpeedFromROM (
    PDEVICETABLE            pDev,
    ULONG                  *pChipClkSpeed);

ULONG
Perm3SetColorLookup(
    PDEVICETABLE            pDev,
    PVIDEO_CLUT             ClutBuffer,
    ULONG                   ClutBufferSize,
    BOOLEAN                 ForceRAMDACWrite,
    BOOLEAN                 UpdateCache);

// Values for the MISC_CONTROL
#define PXRX_MISC_CONTROL_STRIPE_MODE_DISABLE    (0 << 0)    // Stripe mode
#define PXRX_MISC_CONTROL_STRIPE_MODE_PRIMARY    (1 << 0)
#define PXRX_MISC_CONTROL_STRIPE_MODE_SECONDARY  (2 << 0)

#define PXRX_MISC_CONTROL_STRIPE_SIZE_1          (0 << 4)    // Stripe size
#define PXRX_MISC_CONTROL_STRIPE_SIZE_2          (1 << 4)
#define PXRX_MISC_CONTROL_STRIPE_SIZE_4          (2 << 4)
#define PXRX_MISC_CONTROL_STRIPE_SIZE_8          (3 << 4)
#define PXRX_MISC_CONTROL_STRIPE_SIZE_16         (4 << 4)

#define PXRX_MISC_CONTROL_BYTE_DBL_DISABLE       (0 << 7)    // Byte doubling
#define PXRX_MISC_CONTROL_BYTE_DBL_ENABLE        (1 << 7)


//
// VTG_POLARITY and VIDEO_CONTROL horz & vert sync polarity bits
// NB. DPMS_ON is dependent on the polarity of the mode, some modes may
//     require Vsp and/or Hsp -ve. The correct DPMS_ON settings should be 
//     stored in hwDeviceExtension->VideoControlMonitorON P2 is always +ve 
//     (i.e. VC_HSYNC(VC_ACTIVE_HIGH) | VC_VSYNC(VC_ACTIVE_HIGH), the
//     polarities are actually set in MISC_CONTROL); 
//     GMX is always -ve  (i.e. VTG_HSYNC(VTG_ACTIVE_LOW) | VTG_VSYNC(VTG_ACTIVE_LOW))
//

#define VTG_ACTIVE_HIGH    0
#define VTG_FORCED_HIGH    1
#define VTG_ACTIVE_LOW     2
#define VTG_FORCED_LOW     3
#define VTG_HSYNC(x)       (x << 0)
#define VTG_VSYNC(x)       (x << 2)
#define VTG_DPMS_MASK      (VTG_HSYNC(3) | VTG_VSYNC(3))

#define VTG_DPMS_STANDBY   (VTG_HSYNC(VTG_FORCED_LOW)  | VTG_VSYNC(VTG_ACTIVE_HIGH))
#define VTG_DPMS_SUSPEND   (VTG_HSYNC(VTG_ACTIVE_HIGH) | VTG_VSYNC(VTG_FORCED_LOW))
#define VTG_DPMS_OFF       (VTG_HSYNC(VTG_FORCED_LOW)  | VTG_VSYNC(VTG_FORCED_LOW))

#define VC_FORCED_HIGH     0
#define VC_ACTIVE_HIGH     1
#define VC_FORCED_LOW      2
#define VC_ACTIVE_LOW      3
#define VC_HSYNC(x)        (x << 3)
#define VC_VSYNC(x)        (x << 5)
#define VC_ON              1
#define VC_OFF             0
#define VC_DPMS_MASK       (VC_HSYNC(3) | VC_VSYNC(3) | VC_ON)

#define VC_DPMS_STANDBY    (VC_HSYNC(VC_FORCED_LOW)  | VC_VSYNC(VC_ACTIVE_HIGH) | VC_OFF)
#define VC_DPMS_SUSPEND    (VC_HSYNC(VC_ACTIVE_HIGH) | VC_VSYNC(VC_FORCED_LOW)  | VC_OFF)
#define VC_DPMS_OFF        (VC_HSYNC(VC_FORCED_LOW)  | VC_VSYNC(VC_FORCED_LOW)  | VC_OFF)


//
// Helper function defined in perm3rom.c
//

VOID 
CopyROMInitializationTable(
    PDEVICETABLE            pDev,
    PVOID                   pvROMAddress);

VOID 
GenerateInitializationTable(
    PDEVICETABLE            pDev);

VOID 
ProcessInitializationTable(
    PDEVICETABLE            pDev);

//
// macros which takes a GLINT tag name or control register name and translates
// it to a register address. Data must be written to these addresses using
// VideoPortWriteRegisterUlong and read using VideoPortReadRegisterUlong.
// e.g. dma_count = VideoPortReadRegisterUlong(DMA_COUNT);
//

#define CTRL_REG_ADDR(reg)      ((PULONG)&(pCtrlRegs->reg))

//
// PXRX memory timing registers
//

#define PXRX_LOCAL_MEM_CAPS          CTRL_REG_ADDR(LocalMemCaps)
#define PXRX_LOCAL_MEM_CONTROL       CTRL_REG_ADDR(LocalMemControl)
#define PXRX_LOCAL_MEM_POWER_DOWN    CTRL_REG_ADDR(LocalMemPowerDown)
#define PXRX_LOCAL_MEM_REFRESH       CTRL_REG_ADDR(LocalMemRefresh)
#define PXRX_LOCAL_MEM_TIMING        CTRL_REG_ADDR(LocalMemTiming)

//
// Defines whether we setup the default values for PXRX memory timings
// to be for SGRAM, SDRAM or we don't bother programming them and we use
// the values built into the chip.
//

#define PXRX_MEMORY_TYPE_SGRAM_32MB         0
#define PXRX_MEMORY_TYPE_SDRAM_16MB         1
#define PXRX_MEMORY_TYPE_DONTPROGRAM        2
#define PXRX_MEMORY_TYPE_SGDEMUL            3
#define PXRX_MEMORY_TYPE_SGDEMUL64          4
#define PXRX_MEMORY_TYPE_HANDMADE           5
#define PXRX_MEMORY_TYPE_SGRAM_16MB         6

//
// Shift values for LOCAL_MEM_CAPS
//

#define PXRX_LM_CAPS_COLUMN_SHIFT           0
#define PXRX_LM_CAPS_ROW_SHIFT              4
#define PXRX_LM_CAPS_BANK_SHIFT             8
#define PXRX_LM_CAPS_CHIPSEL_SHIFT          12
#define PXRX_LM_CAPS_PAGESIZE_SHIFT         16
#define PXRX_LM_CAPS_REGIONSIZE_SHIFT       20
#define PXRX_LM_CAPS_NOPRECHARGE_SHIFT      24
#define PXRX_LM_CAPS_SPECIALMODE_SHIFT      25
#define PXRX_LM_CAPS_TWOCOLOR_SHIFT         26
#define PXRX_LM_CAPS_COMBINEBANKS_SHIFT     27
#define PXRX_LM_CAPS_NOWRITEMASK_SHIFT      28
#define PXRX_LM_CAPS_NOBLOCKFILL_SHIFT      29
#define PXRX_LM_CAPS_HALFWIDTH_SHIFT        30
#define PXRX_LM_CAPS_NOLOOKAHEAD_SHIFT      31

//
// Shift values for LOCAL_MEM_CONTROL
//

#define PXRX_LM_CTL_CASLATENCY_SHIFT        0
#define PXRX_LM_CTL_INTERLEAVE_SHIFT        3
#define PXRX_LM_CTL_MODE_SHIFT              22

//
// Shift values for LOCAL_MEM_POWER_DOWN
//

#define PXRX_LM_PWR_ENABLE_SHIFT            0
#define PXRX_LM_PWR_DELAY_SHIFT             17

//
// Shift values for LOCAL_MEM_REFRESH
//

#define PXRX_LM_REF_ENABLE_SHIFT            0
#define PXRX_LM_REF_DELAY_SHIFT             1

//
// Shift values for LOCAL_MEM_TIMING
//

#define PXRX_LM_TMG_TURNON_SHIFT            0
#define PXRX_LM_TMG_TURNOFF_SHIFT           2
#define PXRX_LM_TMG_REGLOAD_SHIFT           4
#define PXRX_LM_TMG_BLKWRITE_SHIFT          6
#define PXRX_LM_TMG_ACTTOCMD_SHIFT          8
#define PXRX_LM_TMG_PRETOACT_SHIFT          11
#define PXRX_LM_TMG_BLKWRTTOPRE_SHIFT       14
#define PXRX_LM_TMG_WRTTOPRE_SHIFT          17
#define PXRX_LM_TMG_ACTTOPRE_SHIFT          20
#define PXRX_LM_TMG_REFCYCLE_SHIFT          24


//
// Copied from Win2K miniport
//

#define CTRL_REG_OFFSET(regAddr) ((ULONG)(((ULONG_PTR)regAddr) - ((ULONG_PTR)pCtrlRegs)))


