/******************************Module*Header**********************************\
 *
 *                           *******************************
 *                           * Permedia 2: GDI SAMPLE CODE *
 *                           *******************************
 *
 * Module Name: p2info.h
 *
 * This module contains Permedia 2 hardware related definitions
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/
#ifndef _INC_P2INFO_H_
#define _INC_P2INFO_H_

#define MAXDMABUFF  	10
#define P2MAX_SUBBUFFERS	32 //!!CTM Note that we are not currently 
                               //supporting Interrupt DMA in this release

#define SIZE_CONFIGURATIONBASE 32
#define MAX_QUEUE_SIZE (P2MAX_SUBBUFFERS + 2)

typedef struct _VDDDISPLAYINFO 
{
    unsigned short ddiHdrSize;
    unsigned short ddiInfoFlags;		
    unsigned long  ddiDevNodeHandle;
    unsigned char  ddiDriverName[16];
    unsigned short ddiXRes;			
    unsigned short ddiYRes;			
    unsigned short ddiDPI;			
    unsigned char  ddiPlanes;	
    unsigned char  ddiBpp;	
    unsigned short ddiRefreshRateMax;	
    unsigned short ddiRefreshRateMin;	
    unsigned short ddiLowHorz;		
    unsigned short ddiHighHorz;		
    unsigned short ddiLowVert;		
    unsigned short ddiHighVert;		
    unsigned long  ddiMonitorDevNodeHandle;
    unsigned char  ddiHorzSyncPolarity;	
    unsigned char  ddiVertSyncPolarity;
    // new 4.1 stuff
    unsigned long  diUnitNumber;             // device unit number
    unsigned long  diDisplayFlags;           // mode specific flags
    unsigned long  diXDesktopPos;            // position of desktop
    unsigned long  diYDesktopPos;            // ...
    unsigned long  diXDesktopSize;           // size of desktop (for panning)
    unsigned long  diYDesktopSize;           // ...
} VDDDISPLAYINFO;


typedef struct _GlintInfo
{
    // Pointers
    unsigned long           dwDSBase;           // 32 bit base of data seg
    unsigned long           dwpRegisters;      
    unsigned long           dwpFrameBuffer;    
    unsigned long           dwpLocalBuffer;    
    // Selectors:
    unsigned long           dwRegSel;
    unsigned long           dwFBSel; 	   
    unsigned long           dwLBSel; 	   
    unsigned long           dwText32Selector;   // selector of 32 bit text area
    // Chip Information
    unsigned long           dwRamDacType; 
    volatile unsigned long  dwFlags;        
    unsigned long           dwDisconCtrl;   
    unsigned long           ddFBSize;	        // frame buffer size
    unsigned long           ddLBSize;	        // local buffer size
    unsigned long           dwPartialProducts;  // Cached partial products for 
                                                // this screen width
    unsigned long           dwPartialProducts2; // Partial products for 
                                                // half this screen width
    unsigned long           dwPartialProducts4; // Partial products for quarter 
                                                // of this screen width
    unsigned long           dwVGAEnable;        // Value to switch on VGA
    unsigned long           dwVGADisable;       // Value to switch off VGA
    unsigned long           dwVideoControl;     // Value to put in the Video 
                                                // Control field.
    unsigned long           dwScreenBase;       // Screen base value for the 
                                                // screen
    unsigned long           dwOffscreenBase;    // Start of Offscreen heap   
    unsigned long           dwOSFontCacheStart; // Start of OS Font Cache    
    unsigned long           dwHiResMode; 
    unsigned char           cBlockFillSize;     // Width in pixels of the 
                                                // block fill
    unsigned char           cLBDepth;           // depth in bits of the 
                                                // localbuffer
    unsigned short          wRefreshRate;       // refresh rate used
    // Driver information
    unsigned long           dwDeviceHandle;
    unsigned long           dwVideoMemorySize;
    unsigned long           dwScreenWidth;
    unsigned long           dwScreenHeight;
    unsigned long           dwBpp;
    unsigned long           dwScreenWidthBytes;
    unsigned long           dwCurrentOffscreen;
    unsigned long           lpDriverPDevice;
    unsigned long           pRegs;
    unsigned char           bPixelToBytesShift;
    unsigned char           bPixelToWordShift;     
    unsigned short          DisabledByGLDD;
    unsigned long           lpDDHAL_SetInfo;
    // Chip information. This should be filled out as much as
    // possible. We may not know all the information though.
    unsigned long           dwRenderChipID;
    unsigned long           dwRenderChipRev;   
    unsigned long           dwRenderFamily;     
    unsigned long           dwSupportChipID;  
    unsigned long           dwSupportChipRev;  
    unsigned long           dwBoardID;        
    unsigned long           dwBoardRev;       
    unsigned long           GlintBoardStatus;
    // P2 Video support
    unsigned long           VPE_VSPartialConfigA;
    unsigned long           I2C_VSPartialConfigA;
    unsigned long           VPE_VSPartialConfigB;
    unsigned long           I2C_VSPartialConfigB;
    // DMA information
    unsigned long           dwDMABufferSize;	        // size of dma buffers
    unsigned long           dwDMABufferPhys[MAXDMABUFF];// physical addresses 
                                                        // of buffers
    unsigned long           dwDMABufferVirt[MAXDMABUFF];// virtual ring 0 
                                                        // addresses
    unsigned short          wDMABufferCount;	        // count of dma buffers
    unsigned short          wDMABufferAlloc;	        // mask of available 
                                                        // dma buffers
    unsigned long           dw2D_DMA_Phys;	            // Phys addr of 2D 
                                                        // DMA Buffer
    // index offsets into the queue for the front, back and end. Using separate
    // front and back offsets allows the display driver to add and the interrupt
    // controller to remove entries without a need for locking code.
    unsigned long           frontIndex;
    volatile unsigned long  backIndex;
    unsigned long           endIndex;
    unsigned long           maximumIndex;
    // arrays to contain the DMA queue
    unsigned long	        QCommand[MAX_QUEUE_SIZE];
    unsigned long	        QAddress[MAX_QUEUE_SIZE];
    unsigned long	        QCount[MAX_QUEUE_SIZE];
    // Interrupts
    volatile unsigned long  pIntEnable;         // interrupt enable address
    volatile unsigned long  pIntFlags;          // interrupt flags
    volatile unsigned long  InterruptPending;
    unsigned long           InterruptCount;     // Support for DDFLIP_INTERVALN
    // Markers for determining VideoStream polarity
    unsigned long	        dwVSAPolarity;      // The current polarities
    unsigned long	        dwVSAPrevIndex;     // The last index seen.
    // Contexts
    unsigned long           pContext;           // 32 bit pointer to contexts
    unsigned short          pContext16;
    unsigned short          pContextPad;
    volatile unsigned long  dwCurrentContext;   // current context index                            
    // Font Cache
    unsigned long           dwFontCacheVirt;    // 32 bit pointer to font cache
    unsigned long           dwFontCache16;      
    unsigned long           dwFontCacheSize;    
    // Color Table
    unsigned short          lpColorTableOff;	// Offset of Color Table (in 8bpp)
    unsigned short          lpColorTableSeg;	// Segment of Color Table (in 8bpp)
    unsigned long           pColorTable;		// 32 bit address of Color Table
    // Cursor Information. Used mainly in VBLANK interrupts
    unsigned long           dwCursorType;		
    unsigned short          wCursorControl;		
    unsigned short          wCursorPad;
    // Name of the registry vendor key in which to locate the configuration
    unsigned char           RegistryConfigBase[SIZE_CONFIGURATIONBASE];
    // NOTE: Not DWORD aligned, must be at end of structure.
    VDDDISPLAYINFO          DesktopDisplayInfo;
} GlintInfo, *LPGLINTINFO, PERMEDIA2INFO, *LPPERMEDIA2INFO;

typedef struct _GlintGamma {
    unsigned short          ValidGammaTable;
    unsigned short          GammaRampTable[256*3];
} GlintGamma, GLINT_GAMMA;

#define MAX_P2INTERRUPT_COUNT   0xFFFF  // Support for DDFLIP_INTERVALN

#endif /* #ifndef _INC_P2INFO_H_ */

