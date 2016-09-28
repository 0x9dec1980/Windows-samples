/******************************Module*Header**********************************\
 *
 *                           *******************************
 *                           * Permedia 2:     SAMPLE CODE *
 *                           *******************************
 *
 * Module Name: glddif.h
 *
 * Interface to Direct Draw extension for GLINT family
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


// Interface Initialisation

#define GLDD_FAILED				((DWORD)(-1))
#define GLDD_SUCCESS			0
#define GLDD_VERSION			0x10000
#define GLDD_NOMEM				1
#define GLDD_INVALIDARGS		2
#define GLDD_FREE_REFERENCE		3
#define GLDD_ALREADYFREE		4
#define GLDD_ALREADYLOCKED		5
#define GLDD_SURFACESLOST		6

#define OPENGL_CMD				4352
#define OPENGL_GETINFO			4353
#define GLINT_GETCHIPINFO		10001
#define GLINT_DEBUGHELP			10002
#define GLINT_HEAPSTRESS		10003
#define GLINT_I2C_ESCAPE		10004
#define GLINT_GETVERSION_ESCAPE 10005
#define GLINT_GETINFO			10006
#define GLINT_VMI_ESCAPE		10010

#pragma warning( disable: 4229)		// To stop MSDev 4.1 moaning...

typedef DWORD (*PASCAL LPGLDD_INITIALIZE)(DWORD dwDisplayHandle);

#define GLDD_INITIALIZE         GLDD_Initialize

// Favour which end of memory?
#define MEM3DL_FRONT					1
#define MEM3DL_BACK						2

// Allocation strategy
#define MEM3DL_BEST_FIT					4
#define MEM3DL_FIRST_FIT				8
#define MEM3DL_APPROX_FIT				16

// OpenGL flags
#define MEM3DL_BACK_BUFFER				32
#define MEM3DL_Z_BUFFER					64
#define MEM3DL_TEXTURE_BUFFER			128
#define MEM3DL_SHARED					256
#define MEM3DL_SQUARE					512
#define MEM3DL_PRIMARY					1024
#define MEM3DL_SIGNALED					2048

typedef struct tagMEMREQUEST
{
	DWORD dwSize; 	// Size of this structure

	DWORD dwFlags;	// Flags for this allocation

	DWORD dwAlign;	// Alignment (minimum 4 Bytes)
	DWORD dwBytes;	// Bytes to allocated (aligned up to DWORD multiples)

	DWORD pMem;		// Pointer to the start of the memory returned

} MEMREQUEST, *LPMEMREQUEST;

typedef struct tagCURRENT_VERSION_DETAILS
{
        DWORD dwVerMajor;
        DWORD dwVerMinor;
	DWORD dwReserved;
} CURRENT_VERSION_DETAILS, *LPCURRENT_VERSION_DETAILS;

typedef struct _GLDD_CHIP_INFO {
    // Some chip and board information.
    DWORD       RenderID;           // ID of the rendering chip (eg GLINT_SX)
    DWORD       RenderRev;          // and the Revision
    DWORD       SupportID;          // ID of the support chip (eg DELTA)
    DWORD       SupportRev;         // and its revision.
    DWORD       BoardID;            // ID of the board
    DWORD       BoardRev;           // And its revision.
} GLDD_CHIP_INFO, *LPGLDD_CHIP_INFO;

#define GLDD_CHIPID_NONE            	0x0
#define GLDD_CHIPID_GLINT_SX        	0x1
#define GLDD_CHIPID_GLINT_TX        	0x2
#define GLDD_CHIPID_DELTA           	0x3
#define GLDD_CHIPID_PERMEDIA        	0x4
#define GLDD_CHIPID_TIPERMEDIA        	0x3d04
#define GLDD_CHIPID_PERMEDIAPLUS       	0x5
#define GLDD_CHIPID_GLINT_MX        	0x6
#define GLDD_CHIPID_PERMEDIA2        	0x7
#define GLDD_CHIPID_TIPERMEDIA2        	0x3d07
#define GLDD_CHIPID_UNKNOWN         	0xFFFF

#define GLDD_REVID_1                	0x1
#define GLDD_REVID_2                	0x2
#define GLDD_REVID_3                	0x3

#define GLDD_BOARDID_UNKNOWN        	0x0
#define GLDD_BOARDID_MONTSERRAT     	0x1
#define GLDD_BOARDID_RACER		    	0x2
#define GLDD_BOARDID_ACCEL_PANTHER  	0x100
#define GLDD_BOARDID_OMNICOMP_3DEMON  	0x200
#define GLDD_BOARDID_3DBLASTER      	0x8000

typedef struct _GLDD_INFO
{
    // Size structure. Should be set to sizeof(GLDD_INFO)
    DWORD       dwSize;

    DWORD       pRegisters;         // pointer to register area
    DWORD       pFrameBuffer;       // pointer to frame buffer
    DWORD       pLocalBuffer;       // pointer to local buffer
    WORD        wIOPort;            // I/O port if relevant
    WORD        wFlags;             // style flags

    // Some information about the physical capabilities of the board
    DWORD       LocalBufferSize;    // Size of the Local Buffer in Bytes
    DWORD       FrameBufferSize;    // Size of the Frame Buffer in Bytes
    DWORD       LocalBufferDepth;
    DWORD       BlockFillSize;

    // Identification of the chips and board.
    GLDD_CHIP_INFO Chips;

	// Address of the Screen offset into Framestore in bytes
	DWORD		dwScreenBase;

	// String which identifies the vendor as specified in the registry.
	// eg the "3dlabs" in HKLM\SOFTWARE\3dlabs\Display
    char		RegistryConfigBase[32];

} GLDD_INFO, *LPGLDD_INFO;

#define GLDDF_REV2          0x01   // chip is rev 2
#define GLDDF_FFON          0x02   // fast fill enabled
#define GLDDF_NOIRQ         0x04   // IRQ disabled
#define GLDDF_SECONDARY     0x0100 // There is a secondary chip (eg DELTA) on the board.
#define GLDDF_HWWRITEMASK   0x4000 // Chip supports hardware write masks

typedef VOID (*PASCAL LPGLDD_GETINFO)(DWORD dwDisplayHandle, LPGLDD_INFO);

#define GLDD_GETINFO            GLDD_GetInfo

// Context switching

typedef DWORD (*PASCAL LPGLDD_CREATECONTEXT)(DWORD dwDisplayHandle, DWORD);
typedef DWORD (*PASCAL LPGLDD_SWITCHCONTEXT)(DWORD dwDisplayHandle, DWORD);
typedef DWORD (*PASCAL LPGLDD_DELETECONTEXT)(DWORD dwDisplayHandle, DWORD);
typedef DWORD (*PASCAL LPGLDD_DEBUGHELP)(DWORD dwDisplayHandle, DWORD,DWORD*, DWORD, DWORD);
typedef DWORD (*PASCAL LPGLDD_ALLOCATEVIDMEM)(DWORD dwDisplayHandle, LPMEMREQUEST);
typedef DWORD (*PASCAL LPGLDD_FREEVIDMEM)(DWORD dwDisplayHandle, DWORD);

typedef DWORD (*PASCAL LPGLDD_SERVICE)(DWORD dwService, struct tagThisData* pData, void* pBuffer);

#define GLDD_CREATECONTEXT      GLDD_CreateContext
#define GLDD_SWITCHCONTEXT      GLDD_SwitchContext
#define GLDD_DELETECONTEXT      GLDD_DeleteContext

// DMA extensions

typedef struct _GlintDMARequest
{
    DWORD       dwContext;
    DWORD       dwSize;
    DWORD       dwPhys;
    DWORD       dwEvent;
} GLDMAREQ, *LPGLDMAREQ;

typedef struct _GlintDMABuffer
{
    DWORD       dwBuffSize;
    DWORD       dwBuffPhys;
    DWORD       dwBuffVirt;
	DWORD		dwSubBuff;
} GLDMABUFF, *LPGLDMABUFF;

typedef DWORD (*PASCAL LPGLDD_GETDMABUFFER)(DWORD dwDisplayHandle, LPGLDMABUFF);
typedef DWORD (*PASCAL LPGLDD_STARTDMA)(DWORD dwDisplayHandle, DWORD dwContext, DWORD dwSize,
                                        DWORD dwPhys, DWORD dwVirt, DWORD dwEvent);
typedef DWORD (*PASCAL LPGLDD_FREEDMABUFFER)(DWORD dwDisplayHandle, DWORD);
typedef DWORD (*PASCAL LPGLDD_WAITFORDMA)(DWORD dwDisplayHandle);
typedef DWORD (*PASCAL LPGLDD_GETDRIVERLOCK)(DWORD dwDisplayHandle, DWORD pDCISurfInfo, BOOL, RECT* pRect);
typedef DWORD (*PASCAL LPGLDD_FREEDRIVERLOCK)(DWORD dwDisplayHandle, DWORD pDCISurfInfo);
typedef DWORD (*PASCAL LPGLDD_DRIVERISLOCKED)(DWORD dwDisplayHandle);

// For OpenGL to stop and start cursor.
typedef DWORD (*PASCAL LPGLDD_STOPSOFTWARECURSOR)(DWORD dwDisplayHandle);
typedef DWORD (*PASCAL LPGLDD_STARTSOFTWARECURSOR)(DWORD dwDisplayHandle);

typedef VOID  (*PASCAL LPGLDD_COPYBLT)(DWORD dwDisplayHandle,
    RECTL  	*rDest,       // Area in the destination buffer to blt to.
    DWORD   windowBase,   // Offset in pixels to the destination buffer
    LONG    sourceOffset  // Offset from Destination to Source in pixels
);

#define GLDD_GETDMABUFFER			GLDD_GetDMABuffer
#define GLDD_STARTDMA				GLDD_StartDMA
#define GLDD_GETDMASEQUENCE			GLDD_GetDMASequence
#define GLDD_FREEDMABUFFER			GLDD_FreeDMABuffer
#define GLDD_WAITFORDMA				GLDD_WaitForDMA
#define GLDD_COPYBLT				GLDD_CopyBlt
#define GLDD_DEBUGHELP				GLDD_DebugHelp
#define GLDD_STARTSOFTWARECURSOR	GLDD_StartSoftwareCursor
#define GLDD_STOPSOFTWARECURSOR		GLDD_StopSoftwareCursor
#define GLDD_ALLOCATEVIDEOMEMORY	GLDD_AllocateVideoMemory
#define GLDD_FREEVIDEOMEMORY		GLDD_FreeVideoMemory
#define GLDD_GETDRIVERLOCK			GLDD_GetDriverLock
#define GLDD_FREEDRIVERLOCK			GLDD_FreeDriverLock
#define GLDD_DRIVERISLOCKED			GLDD_DriverIsLocked

#define GLDD_SERVICE				GLDD_Service
