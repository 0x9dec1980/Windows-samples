/******************************Module*Header**********************************\
 *
 *                           *******************************
 *                           * Permedia 2: GDI SAMPLE CODE *
 *                           *******************************
 *
 * Module Name: glinfo.h
 *
 * This module contains Permedia 2 hardware related definitions
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#ifndef _INC_GLINFO_H_
#define _INC_GLINFO_H_

// Registry locations
#define REGKEYROOT "SOFTWARE\\"
#define REGKEYDIRECTXSUFFIX "\\DirectX"
#define REGKEYDISPLAYSUFFIX "\\Display"

#define MAXCONTEXT      128
#define MAX_SUBBUFFERS  32 //!!CTM Note that we are not currently supporting 
                           //Interrupt DMA in this release

// This many dwords to resend the bad regs every DMAA buffer
#define PERMEDIA_REV1_BAD_READBACK_REGISTER_SPACE       12   
                                                            
#define GLINT_DMA           1
#define GLINT_NON_DMA       2
#define GLINT_FAKE_DMA      8

// Definitions for the various chip types. These must
// be kept up to date with the ones in glddif.h
#define VENDOR_ID_3DLABS    0x3D3D
#define VENDOR_ID_TI        0x104C

// Allof these ID's are guaranteed to fit into 16 bits
#define NOCHIP_ID           0x0
#define GLINT300SX_ID       0x1
#define GLINT500TX_ID       0x2
#define DELTA_ID            0x3
#define PERMEDIA_ID         0x4
#define TIPERMEDIA_ID       0x3d04
#define GLINTMX_ID          0x6
#define PERMEDIA2_ID        0x7
#define TIPERMEDIA2_ID      0x3d07
#define UNKNOWN_DEVICE_ID   0xffff

#define GLINT_ID            GLINT300SX_ID

#define PERMEDIA_SUBSYSTEM_ID       0x96
#define PERMEDIA_NT_SUBSYSTEM_ID    0x98
#define PERMEDIA_LC_SUBSYSTEM_ID    0x99
#define PERMEDIA2_SUBSYSTEM_ID      0xa0

// Pointer types - used in mini\pointer.c and in gldd32\opengl.c
#define SOFTWARECURSOR 0
#define HARDWARECURSOR 1

// Board types.
#define BOARD_MONTSERRAT        0x1
#define BOARD_RACER             0x2
#define BOARD_RACERPRO          0x3

#define BOARD_PERMEDIA          PERMEDIA_SUBSYSTEM_ID   // Permedia ref. board
#define BOARD_PERMEDIA_DELTA    PERMEDIA_NT_SUBSYSTEM_ID// Permedia ref. board 
                                                        // + Delta chip
#define BOARD_PERMEDIA_LC       PERMEDIA_LC_SUBSYSTEM_ID// The newer Permedia 
                                                        // LC (low Cost)
#define BOARD_PERMEDIA2         PERMEDIA2_SUBSYSTEM_ID  // P2 Reference board

#define BOARD_UNKNOWN           0xffff

#define GLINT300SX_REV1     (0 | ((long)GLINT300SX_ID   << 16))
#define GLINT300SX_REV2     (2 | ((long)GLINT300SX_ID   << 16))
#define GLINT500TX_REV1     (1 | ((long)GLINT500TX_ID   << 16))
#define GLINTMX_REV1        (1 | ((long)GLINTMX_ID      << 16))
#define DELTA_REV1          (1 | ((long)DELTA_ID        << 16))
#define PERMEDIA_REV1       (1 | ((long)PERMEDIA_ID     << 16))
#define TI_PERMEDIA_REV1    (1 | ((long)TIPERMEDIA_ID   << 16))
#define PERMEDIA2_REV1      (1 | ((long)PERMEDIA2_ID    << 16))
#define TIPERMEDIA2_REV1    (1 | ((long)TIPERMEDIA2_ID  << 16))

#define GLINT_GETVERSION                1
#define GLINT_IOCTL_ADD_CONTEXT_MEMORY  2
#define GLINT_MEMORY_REQUEST            3
#define GLINT_16TO32_POINTER            4
#define GLINT_I2C                       5

// What is this request for?
#define GLINT_MEMORY_ALLOCATE           1
#define GLINT_MEMORY_FREE               2

typedef struct tagALLOCREQUEST
{
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwBytes;
    unsigned long ptr16;    // in/out
    unsigned long ptr32;    // in/out
} ALLOCREQUEST, *LPALLOCREQUEST;

#define GLINT_I2C_READ                  0
#define GLINT_I2C_WRITE                 1
#define GLINT_I2C_RESET                 2
#define GLINT_I2C_DEVICE_PRESENT        3
typedef struct tagI2CREQUEST
{
    unsigned long dwOperation;      // What do we want to do
    unsigned short wSlaveAddress;   // Slave we are talking to
    unsigned char NumItems;         // Number of items to send/receive
    unsigned char Data[256];        // Data to send/receive
    unsigned long dwReserved1;      // A reserved DWORD
    unsigned long dwReserved2;      // A reserved DWORD
    unsigned long dwReserved3;      // A reserved DWORD
    unsigned long dwReserved4;      // A reserved DWORD
} I2CREQUEST, *LPI2CREQUEST;

#define GLINT_VMI_READ                  0
#define GLINT_VMI_WRITE                 1
typedef struct tagVMIREQUEST
{
    unsigned long dwOperation;      // What do we want to do
    unsigned long dwRegister;       // Register to talk to
    unsigned long dwCommand;        // Command to send
    unsigned long dwReserved1;      // A reserved DWORD
    unsigned long dwReserved2;      // A reserved DWORD
    unsigned long dwReserved3;      // A reserved DWORD
    unsigned long dwReserved4;      // A reserved DWORD
} VMIREQUEST, *LPVMIREQUEST;

#define CONTEXT_GENERIC         0
#define CONTEXT_GLINT300SX      1
#define CONTEXT_GLINT500TX      2
#define CONTEXT_DELTA           3
#define CONTEXT_PERMEDIA        4
#define CONTEXT_GLINTMX         6
#define CONTEXT_PERMEDIA2       7
#define CONTEXT_GLINT_FAMILY    0x4000
#define CONTEXT_PERMEDIA_FAMILY 0x4001
#define CONTEXT_ENDOFBLOCK      0xffff

// Some well known context and template handles.
#define CONTEXT_TEMPLATE_DISPLAY_HANDLE      0
#define CONTEXT_TEMPLATE_DIRECTDRAW_HANDLE   1
#define CONTEXT_TEMPLATE_ALLREADABLE_HANDLE  2
#define CONTEXT_TEMPLATE_DIRECT3D_HANDLE     3
#define CONTEXT_DISPLAY_HANDLE               4
#define CONTEXT_DIRECTDRAW_HANDLE            5
#define CONTEXT_NONE                         0xffff

#define MAX_CONTEXTS_IN_BLOCK 32
#define NPAGES_IN_CONTEXT_BLOCK 6
#define SIZE_OF_CONTEXT_BLOCK (NPAGES_IN_CONTEXT_BLOCK * PAGESIZE)

// commands to the interrupt controller on the next VBLANK
typedef enum {
    NO_COMMAND = 0,
    COLOR_SPACE_BUFFER_0,
    COLOR_SPACE_BUFFER_1,
    GLINT_RACER_BUFFER_0,
    GLINT_RACER_BUFFER_1
} VBLANK_CONTROL_COMMAND;

// interrupt status bits
typedef enum {
    DMA_INTERRUPT_AVAILABLE     = 0x01, // can use DMA interrupts
    VBLANK_INTERRUPT_AVAILABLE  = 0x02, // can use VBLANK interrupts
    SUSPEND_DMA_TILL_VBLANK     = 0x04, // Stop doing DMA till after next VBLANK
} INTERRUPT_CONTROL;

// bit definitions for the status words in GlintBoardStatus[]:
// Currently used to indicate sync and DMA status. We have the following rules:
// synced means no outstanding DMA as well as synced. DMA_COMPLETE means n
// outstanding DMA but not necessarily synced. Thus when we do a wait on DMA
// complete we turn off the synced bit.
// XXX for the moment we don't use the synced bit as it's awkward to see where
// to unset it - doing so for every access to the chip is too expensive. We
// probably need a "I'm about to start downloading to the FIFO" macro which
// gets put at the start of any routine which writes to the FIFO.
//
#define GLINT_SYNCED            0x1
#define GLINT_DMA_COMPLETE      0x2 // set when there is no outstanding DMA
#define GLINT_INTR_COMPLETE     0x4
#define GLINT_INTR_CONTEXT      0x8 // set if the current context is 
                                    // interrupt enabled

// Render flags
#define SURFACE_GOURAUD         (1 << 0)
#define SURFACE_ZENABLE         (1 << 1)
#define SURFACE_SPECULAR        (1 << 2)
#define SURFACE_FOGENABLE       (1 << 3)
#define SURFACE_PERSPCORRECT    (1 << 4)
#define SURFACE_TEXTURING       (1 << 5)
#define SURFACE_ALPHAENABLE     (1 << 6)
#define SURFACE_MONO            (1 << 7)

#define SURFACE_WRAP_U          (1 << 8)
#define SURFACE_WRAP_V          (1 << 9)

// Use the alpha value to calculate a stipple pattern
#define SURFACE_ALPHASTIPPLE    (1 << 10)
#define SURFACE_ZWRITEENABLE    (1 << 11)

// Enable last point on lines
#define SURFACE_ENDPOINTENABLE  (1 << 12)

// Is the alpha blending a chromakeying operation?
#define SURFACE_ALPHACHROMA     (1 << 13)

// Is the filter mode setup for MipMapping?
#define SURFACE_MIPMAPPING      (1 << 14)

typedef struct __ContextTable {
    unsigned long   pD3DContext16[MAX_CONTEXTS_IN_BLOCK];
    unsigned long   pD3DContext32[MAX_CONTEXTS_IN_BLOCK];
    unsigned long   pNextContext;
    unsigned short  pNextContext16;
    unsigned short  nInBlock;
    unsigned short  nUsed;
    unsigned short  FirstFree;
    unsigned short  nFree;
    unsigned short  COffset[MAX_CONTEXTS_IN_BLOCK];
    signed short    CSize[MAX_CONTEXTS_IN_BLOCK];
    unsigned short  CTemplate[MAX_CONTEXTS_IN_BLOCK];
    unsigned short  CEndIndex[MAX_CONTEXTS_IN_BLOCK];
}   CONTEXTTABLE, *PCONTEXTTABLE;


// For holding information about a single DMA Buffer
typedef struct tagDMAPartition
{
    unsigned long *PhysAddr;        // Physical address of this sub-buffer
#ifndef WIN32
    //int              pad1;
#endif
    unsigned long *VirtAddr;        // Virtual address of this sub-buffer
#ifndef WIN32
    //int              pad2;
#endif
    unsigned long *MaxAddress;  // Maximum address of this sub-buffer
#ifndef WIN32
    //int              pad3;
#endif
} __DMAPartition, *lpDMAPartition;


typedef struct _D3DContextHeader {
    // Magic number to verify validity of pointer
    unsigned long MagicNo ;

    // 16 & 32 Bit pointers to this
    unsigned long ptr16, ptr32;

    // defines interface buffer as either GLINT_DMA or GLINT_NON_DMA
    // For GLINT_DMA, an interrupt drivend DMA transfer mechanism operates
    // For GLINT_NON_DMA, a single buffered non-DMA mechanism operates
    // DMABufferNum is the buffer to free at the end of day
    unsigned long InterfaceType;
    unsigned long DMABufferNum;

    __DMAPartition  DMAPartition[MAX_SUBBUFFERS];
    unsigned long NumberOfSubBuffers;
    
    long    NumPartitions;                  // Number of partitions
    unsigned long   CurrentPartition;
    unsigned long   MaxSendSize;
    unsigned long   PartitionSize;

    // Current address of buffer
    unsigned long volatile *CurrentBuffer;  // Current Buffer Pointer
#ifndef WIN32
    //int              pad1;
#endif

    unsigned long AlphaBlendSend;
    unsigned long Flags;
    unsigned long FillMode;
    unsigned long KeptFog;

    // Software copy of Registers for glint, gigi, permedia
#ifdef WIN32
    __GlintSoftwareCopy SoftCopyGlint;  
#else
    unsigned long softCopyFake[23];
#endif
} D3DCONTEXTHEADER;


#ifdef MINIVDD
/* Begin nasty hack...
   The MINIVDD needs to be able to access the early parts of the D3DCONTEXT 
   structure so I tell it about the early parts.  Other, later, parts are 
   definately never going to become available. */
typedef struct _glint_d3dcontext {
    // The magic number and DMA segments MUST come at the 
    // start of the structure...
    D3DCONTEXTHEADER    Hdr;
} GLINT_D3DCONTEXT ;
// End nasty hack
#endif


typedef struct _att21505off
{
    unsigned char WriteAddr1;       // 0000
    unsigned char PixelColRam;      // 0001
    unsigned char PixelRdMask;      // 0010
    unsigned char ReadAdd1;         // 0011
    unsigned char WriteAddr2;       // 0100
    unsigned char CursorColRam;     // 0101
    unsigned char Ctrl0;            // 0110
    unsigned char ReadAdd2;         // 0111
    unsigned char Ctrl1;            // 1000
    unsigned char Ctrl2;            // 1001
    unsigned char Status;           // 1010
    unsigned char CursorPattern;    // 1011
    unsigned char CursorXLow;       // 1100
    unsigned char CursorXHigh;      // 1101
    unsigned char CursorYLow;       // 1110
    unsigned char CursorYHigh;      // 1111
} ATT21505OFF;

typedef struct _DMAQueue
{
    unsigned long dwContext;      // context for fragment
    unsigned long dwSize;         // size of it (DWORDs)
    unsigned long dwPhys;         // physical address
    unsigned long dwEvent;        // event if required
} DMAQUEUE;

typedef struct _ContextRegs
{
    unsigned short      wNumRegs;
    unsigned short      wFirstReg[1];
} CONTEXTREGS;


// Config register
#define PM_CHIPCONFIG_AGPCAPABLE (1 << 9)

// DAC types

#define RamDacRGB525    1           // value for RGB525
#define RamDacATT       2           // value for AT&T 21505
#define RamDacTVP3026   3           // TI TVP 3026 (Accel board)

// Board types

#define BID_MONTSERRAT  0
#define BID_RACER       1
#define BID_ACCEL       2

// definitions for dwFlags
// Glint Interrupt Control Bits
//
// InterruptEnable register
#define INTR_DISABLE_ALL                0x00
#define INTR_ENABLE_DMA                 0x01
#define INTR_ENABLE_SYNC                0x02
#define INTR_ENABLE_EXTERNAL            0x04
#define INTR_ENABLE_ERROR               0x08
#define INTR_ENABLE_VBLANK              0x10
#define INTR_ENABLE_VIDSTREAM_B         0x100
#define INTR_ENABLE_VIDSTREAM_A         0x200

// InterruptFlags register
#define INTR_DMA_SET                    0x01
#define INTR_SYNC_SET                   0x02
#define INTR_EXTERNAL_SET               0x04
#define INTR_ERROR_SET                  0x08
#define INTR_VBLANK_SET                 0x10
#define INTR_VIDSTREAM_B_SET            0x100
#define INTR_VIDSTREAM_A_SET            0x200

#define INTR_CLEAR_ALL                  0x1f
#define INTR_CLEAR_DMA                  0x01
#define INTR_CLEAR_SYNC                 0x02
#define INTR_CLEAR_EXTERNAL             0x04
#define INTR_CLEAR_ERROR                0x08
#define INTR_CLEAR_VBLANK               0x10
#define INTR_CLEAR_VIDSTREAM_B          0x100
#define INTR_CLEAR_VIDSTREAM_A          0x200

#define GMVF_REV2                     0x00000001 // chip is rev 2
#define GMVF_FFON                     0x00000002 // fast fill enabled
#define GMVF_NOIRQ                    0x00000004 // IRQ disabled
#define GMVF_SETUP                    0x00000008 // primitive setup in progress ??
#define GMVF_GCOP                     0x00000010 // something is using 4K area
                                                 // (affects mouse)
#define GMVF_DMAIP                    0x00000020 // DMA started
#define GMVF_POWERDOWN                0x00000040 // Monitor is switched off
#define GMVF_P2FIFOUNDERRUN           0x00000080 // Allow one underrun error
#define GMVF_DELTA                    0x00000100 // using delta
#define GMVF_8BPPRGB                  0x00000200 // use 322 RGB at 8bpp
#define GMVF_16BPPDB                  0x00000400 // use 5551 double buffering 
                                                 // mode
#define GMVF_SWCURSOR                 0x00000800 // Never use a hardware cursor
#define GMVF_INTCPTGDI                0x00001000 // Intercept GDI mode
#define GMVF_OFFSCRNBM                0x00002000 // Offscreen BitMaps mode
#define GMVF_HWWRITEMASK              0x00004000 // Offscreen BitMaps mode

#define GMVF_VBLANK_OCCURED           0x00010000 // VBlank has occured
#define GMVF_VBLANK_ENABLED           0x00020000 // VBlank interrupt is enabled
#define GMVF_VSA_INTERRUPT_OCCURED    0x00040000 // VPort interrupt has occured
#define GMVF_FRAME_BUFFER_IS_WC       0x00080000 // Frame buffer is 
                                                 // write-combined

#define GMVF_EXPORT24BPP              0x10000000 // Set if we should export 
                                                 // 24bpp modes
#define GMVF_DONOTRESET               0x20000000 
#define GMVF_TRYTOVIRTUALISE4PLANEVGA 0x40000000 // Set if we should try to 
                                                 // virtualise 4 plane VGA
#define GMVF_VIRTUALISE4PLANEVGA      0x80000000 // Set if we are virtualising 
                                                 // 4 plane VGA modes.


#ifndef MINIVDD
extern unsigned long CreateContext(struct tagThunkedData* pThisDisplay, 
                                   struct _GlintInfo *, 
                                   unsigned long, 
                                   struct _d3d_context *ptr16, 
                                   struct _d3d_context *ptr32);
extern void _cdecl ChangeContext(struct tagThunkedData* pThisDisplay, 
                                 struct _GlintInfo *, 
                                 unsigned long);
extern void DeleteContext(struct tagThunkedData* pThisDisplay, 
                          struct _GlintInfo *, 
                          unsigned long);
extern void _cdecl SetEndIndex(struct _GlintInfo *, 
                               unsigned long, 
                               unsigned short);
#endif

// Definitions for RegisterDisplayDriver options
#define MINIVDD_SHAREGLINFO         0x00
#define MINIVDD_INITIALISEMODE      0x01
#define MINIVDD_GETGLINFO           0x02

#define MINIVDD_ALLOCATEMEMORY      0x10
#define MINIVDD_FREEMEMORY          0x11

#define MINIVDD_READREGISTRY        0x20
#define MINIVDD_WRITEREGISTRY       0x21

// For free and allocating memory and selectors for use on the
// 16 bit side.
#define MINIVDD_MEMORYREQUEST       0x30

// For sending I2C data across the bus
#define MINIVDD_I2CREQUEST          0x40

// For sending VMI data to the VideoPort
#define MINIVDD_VMIREQUEST          0x41

// For shutting of interrupts and setting device into VGA mode
// from DISABLE function in driver
#define MINIVDD_ENABLEVGA           0x80

#define REG_HKLM_PREFIX             0x01
#define REG_HKU_PREFIX              0x02
#define REG_HKCU_PREFIX             0x03
#define REG_HKCR_PREFIX             0x04
#define REG_HKCC_PREFIX             0x05
#define REG_HKDD_PREFIX             0x06
#define REG_DEVNODEDEFAULTPREFIX    0x07
#define REG_DEVNODEPREFIX           0x08

#define REGTYPE_STRING              0x100
#define REGTYPE_BINARY              0x300
#define REGTYPE_DWORD               0x400

// Defines for the offsets of regions within the Data Segment:
#define DATA_SEGMENT_OFFSET         0x0
#define GLINT_REGISTERS_OFFSET      0x10000
#define DMA_BUFFER_2D               0x30000
#define DMA_BUFFER_3D               0x38000
#define FONT_CACHE_OFFSET           0x100000
#define FINAL_DATA_SEGMENT_SIZE     0x200000

// Defines the maximum size of the regions
#define DATA_SEGMENT_SIZE       GLINT_REGISTERS_OFFSET - DATA_SEGMENT_OFFSET
#define GLINT_REGISTERS_SIZE    DMA_BUFFER_2D - GLINT_REGISTERS_OFFSET
#define DMA_BUFFER_2D_SIZE      DMA_BUFFER_3D - DMA_BUFFER_2D
#define DMA_BUFFER_3D_SIZE      FONT_CACHE_OFFSET - DMA_BUFFER_3D
#define FONT_CACHE_SIZE         FINAL_DATA_SEGMENT_SIZE - FONT_CACHE_OFFSET

// Various independant things that can disable the offscreen bitmap heap.
#define D3D_DISABLED    1
#define DRIVER_DISABLED 2

// Various default values for 2D context registers
#define DEFAULT_P2TEXTUREADDRESSMODE    0x1  
#define DEFAULT_P2TEXTUREREADMODE       0x1760b  
#define DEFAULT_P2TEXTUREDATAFORMAT     0x200  

#endif /* #ifndef _INC_GLINFO_H_ */
