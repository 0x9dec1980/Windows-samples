
option expr32
option casemap:none

; Begin of file ramdac.h
;  RAMDAC registers live on 64 bit boundaries. Leave it up to individual
;  RAMDAC definitions to determine what registers are available and how
;  many bits wide the registers really are.
; 
DacReg		STRUCT 2t
reg		DWORD		?
pad		DWORD		?
DacReg		ENDS

RAMDAC_REG		TYPEDEF		DacReg

; Begin of file rgb525.h
; ++
; 
; Copyright (c) 1994  3Dlabs Inc.
; 
; Module Name:
; 
;     rgb525.h
; 
; Abstract:
; 
;     This module contains the definitions for the IBM RGB525 RAMDAC.
; 
; Environment:
; 
; 
; Revision History:
; 
; --
; Begin of file rgb526.h
; ++
; 
; Copyright (c) 1995  3Dlabs Inc.
; 
; Module Name:
; 
;     rgb526.h
; 
; Abstract:
; 
;     This module contains definitions for the IBM RGB526 RAMDAC. The 526 is a
;     superset of the 525 so only define things which have changed.
; 
; Environment:
; 
; 
; Revision History:
; 
; --
;  RGB526_REVISION_LEVEL
RGB526_PRODUCT_REV_LEVEL		EQU		0000000c0h
RGB526DB_PRODUCT_REV_LEVEL		EQU		000000080h
;  RGB526_ID
RGB526_PRODUCT_ID		EQU		000000002h
; 
;  Key Support
; 
RGB526_KEY_VALUE		EQU		000000068h
RGB526_KEY_MASK		EQU		00000006ch
RGB526_KEY_CONTROL		EQU		000000078h
;  RGB526_32BPP_CTRL in addition to those on the RGB525
;  overlay goes thru palette
B32_DCOL_B8_INDIRECT		EQU		000000000h
;  overlay bypasses palette
B32_DCOL_B8_DIRECT		EQU		000000040h
RGB526_SYSTEM_CLOCK_CTRL		EQU		000000008h
RGB526_SYSCLK_M		EQU		000000015h
RGB526_SYSCLK_N		EQU		000000016h
RGB526_SYSCLK_P		EQU		000000017h
RGB526_SYSCLK_C		EQU		000000018h
RGB526_M0		EQU		000000020h
RGB526_N0		EQU		000000021h
RGB526_P0		EQU		000000022h
RGB526_C0		EQU		000000023h
; End of file rgb526.h
; Begin of file rgb528.h
; ++
; 
; Copyright (c) 1995  3Dlabs Inc.
; 
; Module Name:
; 
;     rgb528.h
; 
; Abstract:
; 
;     This module contains definitions for the IBM RGB528 RAMDAC. The 528 is a
;     superset of the 525 so only define things which have changed.
; 
; Environment:
; 
; 
; Revision History:
; 
; --
;  RGB528_REVISION_LEVEL
RGB528_PRODUCT_REV_LEVEL		EQU		0000000f0h
RGB528A_PRODUCT_REV_LEVEL		EQU		0000000e0h
;  RGB528_ID
RGB528_PRODUCT_ID		EQU		000000002h
;  RGB525_MISC_CTRL_1 extra value
VRAM_SIZE_128		EQU		000000003h
; End of file rgb528.h
; 
;  IBM RGB525 RAMDAC definitions
;  This set of registers resides at &(pCtrlRegs->ExternalVideo)
; 
;  loads internal register for palette writes
;  read/write to get/set palette data
;  mask to AND with input pixel data
;  loads internal register for palette reads
;  low byte of internal control/cursor register
;  high byte of internal control/cursor register
;  read/write to get/set control/cursor data
;  controls auto-increment of internal addresses
_rgb525_regs		STRUCT 2t
palAddrWr		RAMDAC_REG		<>
palData		RAMDAC_REG		<>
pixelMask		RAMDAC_REG		<>
palAddrRd		RAMDAC_REG		<>
indexLow		RAMDAC_REG		<>
indexHigh		RAMDAC_REG		<>
indexData		RAMDAC_REG		<>
indexCtl		RAMDAC_REG		<>
_rgb525_regs		ENDS

RGB525RAMDAC		TYPEDEF		_rgb525_regs

pRGB525RAMDAC		TYPEDEF		PTR _rgb525_regs

;  macro declared by any function wishing to use the RGB525 RAMDAC. MUST be declared
;  after GLINT_DECL.
; 
;  use the following macros as the address to pass to the
;  VideoPortWriteRegisterUlong function
; 
;  need a delay between each write to the 525. The only way to guarantee
;  that the write has completed is to read from a GLINT control register.
;  Reading forces any posted writes to be flushed out. PPC needs 2 reads
;  to give us enough time.
;  macro to load a given data value into an internal RGB525 register. The
;  second macro loads an internal index register assuming that we have
;  already zeroed the high address register.
; 
;  macros to load a given RGB triple into the RGB525 palette. Send the starting
;  index and then send RGB triples. Auto-increment is turned on.
;  Use RGB525_PALETTE_START and multiple RGB525_LOAD_PALETTE calls to load
;  a contiguous set of entries. Use RGB525_LOAD_PALETTE_INDEX to load a set
;  of sparse entries.
; 
;  macro to read back a given RGB triple from the RGB525 palette. Use after
;  a call to RGB525_PALETTE_START_RD
; 
;  macros to set/get the pixel read mask. The mask is 8 bits wide and gets
;  replicated across all bytes that make up a pixel.
; 
;  macros to load values into the cursor array
; 
;  macro to move the cursor
; 
;  macro to change the cursor hotspot
; 
;  macro to change the cursor color
; 
; 
;  RGB525 internal register indexes
; 
RGB525_REVISION_LEVEL		EQU		000000000h
RGB525_ID		EQU		000000001h
RGB525_MISC_CLK_CTRL		EQU		000000002h
RGB525_SYNC_CTRL		EQU		000000003h
RGB525_HSYNC_CTRL		EQU		000000004h
RGB525_POWER_MANAGEMENT		EQU		000000005h
RGB525_DAC_OPERATION		EQU		000000006h
RGB525_PALETTE_CTRL		EQU		000000007h
RGB525_PIXEL_FORMAT		EQU		00000000ah
RGB525_8BPP_CTRL		EQU		00000000bh
RGB525_16BPP_CTRL		EQU		00000000ch
RGB525_24BPP_CTRL		EQU		00000000dh
RGB525_32BPP_CTRL		EQU		00000000eh
RGB525_PLL_CTRL_1		EQU		000000010h
RGB525_PLL_CTRL_2		EQU		000000011h
RGB525_PLL_REF_DIV_COUNT		EQU		000000014h
RGB525_F0		EQU		000000020h
RGB525_F1		EQU		000000021h
RGB525_F2		EQU		000000022h
RGB525_F3		EQU		000000023h
RGB525_F4		EQU		000000024h
RGB525_F5		EQU		000000025h
RGB525_F6		EQU		000000026h
RGB525_F7		EQU		000000027h
RGB525_F8		EQU		000000028h
RGB525_F9		EQU		000000029h
RGB525_F10		EQU		00000002ah
RGB525_F11		EQU		00000002bh
RGB525_F12		EQU		00000002ch
RGB525_F13		EQU		00000002dh
RGB525_F14		EQU		00000002eh
RGB525_F15		EQU		00000002fh
;  RGB525 Internal Cursor Registers
RGB525_CURSOR_CONTROL		EQU		000000030h
RGB525_CURSOR_X_LOW		EQU		000000031h
RGB525_CURSOR_X_HIGH		EQU		000000032h
RGB525_CURSOR_Y_LOW		EQU		000000033h
RGB525_CURSOR_Y_HIGH		EQU		000000034h
RGB525_CURSOR_X_HOT_SPOT		EQU		000000035h
RGB525_CURSOR_Y_HOT_SPOT		EQU		000000036h
RGB525_CURSOR_COLOR_1_RED		EQU		000000040h
RGB525_CURSOR_COLOR_1_GREEN		EQU		000000041h
RGB525_CURSOR_COLOR_1_BLUE		EQU		000000042h
RGB525_CURSOR_COLOR_2_RED		EQU		000000043h
RGB525_CURSOR_COLOR_2_GREEN		EQU		000000044h
RGB525_CURSOR_COLOR_2_BLUE		EQU		000000045h
RGB525_CURSOR_COLOR_3_RED		EQU		000000046h
RGB525_CURSOR_COLOR_3_GREEN		EQU		000000047h
RGB525_CURSOR_COLOR_3_BLUE		EQU		000000048h
RGB525_BORDER_COLOR_RED		EQU		000000060h
RGB525_BORDER_COLOR_GREEN		EQU		000000061h
RGB525_BORDER_COLOR_BLUE		EQU		000000062h
RGB525_MISC_CTRL_1		EQU		000000070h
RGB525_MISC_CTRL_2		EQU		000000071h
RGB525_MISC_CTRL_3		EQU		000000072h
;  M0-M7, N0-N7 need defining
RGB525_DAC_SENSE		EQU		000000082h
RGB525_MISR_RED		EQU		000000084h
RGB525_MISR_GREEN		EQU		000000086h
RGB525_MISR_BLUE		EQU		000000088h
RGB525_PLL_VCO_DIV_INPUT		EQU		00000008eh
RGB525_PLL_REF_DIV_INPUT		EQU		00000008fh
RGB525_VRAM_MASK_LOW		EQU		000000090h
RGB525_VRAM_MASK_HIGH		EQU		000000091h
; 
;  Bit definitions for individual internal RGB525 registers
; 
;  RGB525_REVISION_LEVEL
RGB525_PRODUCT_REV_LEVEL		EQU		0000000f0h
;  RGB525_ID
RGB525_PRODUCT_ID		EQU		000000001h
;  RGB525_MISC_CTRL_1
MISR_CNTL_ENABLE		EQU		000000080h
VMSK_CNTL_ENABLE		EQU		000000040h
PADR_RDMT_RDADDR		EQU		000000000h
PADR_RDMT_PAL_STATE		EQU		000000020h
SENS_DSAB_DISABLE		EQU		000000010h
SENS_SEL_BIT3		EQU		000000000h
SENS_SEL_BIT7		EQU		000000008h
VRAM_SIZE_32		EQU		000000000h
VRAM_SIZE_64		EQU		000000001h
;  RGB525_MISC_CTRL_2
PCLK_SEL_LCLK		EQU		000000000h
PCLK_SEL_PLL		EQU		000000040h
PCLK_SEL_EXT		EQU		000000080h
INTL_MODE_ENABLE		EQU		000000020h
BLANK_CNTL_ENABLE		EQU		000000010h
COL_RES_6BIT		EQU		000000000h
COL_RES_8BIT		EQU		000000004h
PORT_SEL_VGA		EQU		000000000h
PORT_SEL_VRAM		EQU		000000001h
;  RGB525_MISC_CTRL_3
SWAP_RB		EQU		000000080h
SWAP_WORD_LOHI		EQU		000000000h
SWAP_WORD_HILO		EQU		000000010h
SWAP_NIB_HILO		EQU		000000000h
SWAP_NIB_LOHI		EQU		000000002h
;  RGB525_MISC_CLK_CTRL
DDOT_CLK_ENABLE		EQU		000000000h
DDOT_CLK_DISABLE		EQU		000000080h
SCLK_ENABLE		EQU		000000000h
SCLK_DISABLE		EQU		000000040h
B24P_DDOT_PLL		EQU		000000000h
B24P_DDOT_SCLK		EQU		000000020h
DDOT_DIV_PLL_1		EQU		000000000h
DDOT_DIV_PLL_2		EQU		000000002h
DDOT_DIV_PLL_4		EQU		000000004h
DDOT_DIV_PLL_8		EQU		000000006h
DDOT_DIV_PLL_16		EQU		000000008h
PLL_DISABLE		EQU		000000000h
PLL_ENABLE		EQU		000000001h
;  RGB525_SYNC_CTRL
DLY_CNTL_ADD		EQU		000000000h
DLY_SYNC_NOADD		EQU		000000080h
CSYN_INVT_DISABLE		EQU		000000000h
CSYN_INVT_ENABLE		EQU		000000040h
VSYN_INVT_DISABLE		EQU		000000000h
VSYN_INVT_ENABLE		EQU		000000020h
HSYN_INVT_DISABLE		EQU		000000000h
HSYN_INVT_ENABLE		EQU		000000010h
VSYN_CNTL_NORMAL		EQU		000000000h
VSYN_CNTL_HIGH		EQU		000000004h
VSYN_CNTL_LOW		EQU		000000008h
VSYN_CNTL_DISABLE		EQU		00000000ch
HSYN_CNTL_NORMAL		EQU		000000000h
HSYN_CNTL_HIGH		EQU		000000001h
HSYN_CNTL_LOW		EQU		000000002h
HSYN_CNTL_DISABLE		EQU		000000003h
;  RGB525_HSYNC_CTRL
;  RGB525_POWER_MANAGEMENT
SCLK_PWR_NORMAL		EQU		000000000h
SCLK_PWR_DISABLE		EQU		000000010h
DDOT_PWR_NORMAL		EQU		000000000h
DDOT_PWR_DISABLE		EQU		000000008h
SYNC_PWR_NORMAL		EQU		000000000h
SYNC_PWR_DISABLE		EQU		000000004h
ICLK_PWR_NORMAL		EQU		000000000h
ICLK_PWR_DISABLE		EQU		000000002h
DAC_PWR_NORMAL		EQU		000000000h
DAC_PWR_DISABLE		EQU		000000001h
;  RGB525_DAC_OPERATION
SOG_DISABLE		EQU		000000000h
SOG_ENABLE		EQU		000000008h
BRB_NORMAL		EQU		000000000h
BRB_ALWAYS		EQU		000000004h
DSR_DAC_SLOW		EQU		000000000h
DSR_DAC_FAST		EQU		000000002h
DPE_DISABLE		EQU		000000000h
DPE_ENABLE		EQU		000000001h
;  RGB525_PALETTE_CTRL
SIXBIT_LINEAR_ENABLE		EQU		000000000h
SIXBIT_LINEAR_DISABLE		EQU		000000080h
;  RGB525_PIXEL_FORMAT
PIXEL_FORMAT_4BPP		EQU		000000002h
PIXEL_FORMAT_8BPP		EQU		000000003h
PIXEL_FORMAT_16BPP		EQU		000000004h
PIXEL_FORMAT_24BPP		EQU		000000005h
PIXEL_FORMAT_32BPP		EQU		000000006h
;  RGB525_8BPP_CTRL
B8_DCOL_INDIRECT		EQU		000000000h
B8_DCOL_DIRECT		EQU		000000001h
;  RGB525_16BPP_CTRL
B16_DCOL_INDIRECT		EQU		000000000h
B16_DCOL_DYNAMIC		EQU		000000040h
B16_DCOL_DIRECT		EQU		0000000c0h
B16_POL_FORCE_BYPASS		EQU		000000000h
B16_POL_FORCE_LOOKUP		EQU		000000020h
B16_ZIB		EQU		000000000h
B16_LINEAR		EQU		000000004h
B16_555		EQU		000000000h
B16_565		EQU		000000002h
B16_SPARSE		EQU		000000000h
B16_CONTIGUOUS		EQU		000000001h
;  RGB525_24BPP_CTRL
B24_DCOL_INDIRECT		EQU		000000000h
B24_DCOL_DIRECT		EQU		000000001h
;  RGB525_32BPP_CTRL
B32_POL_FORCE_BYPASS		EQU		000000000h
B32_POL_FORCE_LOOKUP		EQU		000000004h
B32_DCOL_INDIRECT		EQU		000000000h
B32_DCOL_DYNAMIC		EQU		000000001h
B32_DCOL_DIRECT		EQU		000000003h
;  RGB525_PLL_CTRL_1
REF_SRC_REFCLK		EQU		000000000h
REF_SRC_EXTCLK		EQU		000000010h
PLL_EXT_FS_3_0		EQU		000000000h
PLL_EXT_FS_2_0		EQU		000000001h
PLL_CNTL2_3_0		EQU		000000002h
PLL_CNTL2_2_0		EQU		000000003h
;  RGB525_PLL_CTRL_2
;  RGB525_PLL_REF_DIV_COUNT
;  RGB525_F0 - RGB525_F15
;  RGB525_PLL_REFCLK values
;  RGB525_CURSOR_CONTROL
SMLC_PART_0		EQU		000000000h
SMLC_PART_1		EQU		000000040h
SMLC_PART_2		EQU		000000080h
SMLC_PART_3		EQU		0000000c0h
PIX_ORDER_RL		EQU		000000000h
PIX_ORDER_LR		EQU		000000020h
LOC_READ_LAST		EQU		000000000h
LOC_READ_ACTUAL		EQU		000000010h
UPDT_CNTL_DELAYED		EQU		000000000h
UPDT_CNTL_IMMEDIATE		EQU		000000008h
CURSOR_SIZE_32		EQU		000000000h
CURSOR_SIZE_64		EQU		000000040h
CURSOR_MODE_OFF		EQU		000000000h
CURSOR_MODE_3_COLOR		EQU		000000001h
CURSOR_MODE_2_COLOR_HL		EQU		000000002h
CURSOR_MODE_2_COLOR		EQU		000000003h
;  RGB525_REVISION_LEVEL
;  predefined
REVISION_LEVEL		EQU		0000000f0h
;  RGB525_ID
;  predefined
ID_CODE		EQU		000000001h
; End of file rgb525.h
; Begin of file tvp3026.h
; ++
; 
; Copyright (c) 1995  AccelGraphics, Inc.
; 
; Module Name:
; 
;     TVP3026.h
; 
; Abstract:
; 
;     This module contains the definitions for the TI TVP3026 RAMDAC.
; 
; Environment:
; 	NT
; 
; Revision History:
; 	1.0 - created 18-Aug-1995  -by-  [olegsher]
; --
; 
;  TI TVP3026 RAMDAC definitions
;  This set of registers resides at &(pCtrlRegs->ExternalVideo)
; 
;  0x00 - palette/cursor RAM write address, Index Register
;  0x01 - palette RAM data
;  0x02 - pixel read mask
;  0x03 - palette/cursor RAM read address
;  0x04 - cursor/overscan color write address
;  0x05 - cursor/overscan color data
;  0x06 - reserved
;  0x07 - cursor/overscan color read address
;  0x08 - reserved
;  0x09 - direct cursor control
;  0x0A - indexed data
;  0x0B - cursor RAM data
;  0x0C - cursor position X low byte 
;  0x0D - cursor position X high byte 
;  0x0E - cursor position Y low byte 
;  0x0F - cursor position Y high byte 
_tvp3026_regs		STRUCT 2t
pciAddrWr		DacReg		<>
palData		DacReg		<>
pixelMask		DacReg		<>
pcAddrRd		DacReg		<>
curAddrWr		DacReg		<>
curData		DacReg		<>
Reserverd1		DacReg		<>
curAddrRd		DacReg		<>
Reserverd2		DacReg		<>
curCtl		DacReg		<>
indexData		DacReg		<>
curRAMData		DacReg		<>
cursorXLow		DacReg		<>
cursorXHigh		DacReg		<>
cursorYLow		DacReg		<>
cursorYHigh		DacReg		<>
_tvp3026_regs		ENDS

TVP3026RAMDAC		TYPEDEF		_tvp3026_regs

pTVP3026RAMDAC		TYPEDEF		PTR _tvp3026_regs

;  macro declared by any function wishing to use the TI TVP3026 RAMDAC . MUST be declared
;  after GLINT_DECL.
; 
;  use the following macros as the address to pass to the
;  VideoPortWriteRegisterUlong function
; 
;   Palette Access
;  Pixel mask
;  Access to the indexed registers
;  Access to the Cursor
;  Access to the overscan color
;  Cursor position control
;  ----------------------Values for some direct registers-----------------------
; ******************************************************************************
; 				DIRECT REGISTER - CURSOR AND OVERSCAN COLOR						
; ******************************************************************************
;   ** TVP3026_OVRC_WR_ADDR
;   ** TVP3026_OVRC_RD_ADDR 
;   ** TVP3026_CUR_WR_ADDR
;   ** TVP3026_CUR_RD_ADDR
; 		Default - undefined
TVP3026_OVERSCAN_COLOR		EQU		000000000h
TVP3026_CURSOR_COLOR0		EQU		000000001h
TVP3026_CURSOR_COLOR1		EQU		000000002h
TVP3026_CURSOR_COLOR2		EQU		000000003h
; ******************************************************************************
; 				DIRECT REGISTER - CURSOR CONTROL								
; ******************************************************************************
;   ** TVP3026_CUR_CTL
; 		Default - 0x00
;  Cursor off
TVP3026_CURSOR_OFF		EQU		000000000h
;  2-bits select color
TVP3026_CURSOR_COLOR		EQU		000000001h
;  2-bits select XOR
TVP3026_CURSOR_XGA		EQU		000000002h
;  2-bits select transparency/color
TVP3026_CURSOR_XWIN		EQU		000000003h
; ******************************************************************************
; 				DIRECT REGISTER - CURSOR POSITION CONTROL						
; ******************************************************************************
;   ** TVP3026_CUR_X_LSB 
;   ** TVP3026_CUR_X_MSB 
;   ** TVP3026_CUR_Y_LSB 
;   ** TVP3026_CUR_Y_MSB 
; 		Default - undefined
;  Values written into those registers represent the BOTTOM-RIGHT corner
;  of the cursor. If 0 is in X or Y position - the cursor is off the screen
;  Only 12 bits are used, giving the range from 0 to 4095 ( 0x0000 - 0x0FFF)
;  The size of the cursor is (64,64) (0x40, 0x40)
;  Cursor offscreen
TVP3026_CURSOR_OFFSCREEN		EQU		000000000h
;  ------------------------Indirect indexed registers map--------------------------
; ******************************************************************************
; 				INDIRECT REGISTER - SILICON REVISION							
; ******************************************************************************
;  Chip revision: 
__TVP3026_TVP3026_SILICON_REVISION		EQU		000000001h
;   bits 4-7 - major number, 0-3 - minor number
;  TVP3026_REVISION_LEVEL
;  predefined
TVP3026_REVISION_LEVEL		EQU		000000001h
; ******************************************************************************
; 				INDIRECT REGISTER - CURSOR CONTROL								
; ******************************************************************************
;  Indirect cursor control - 
__TVP3026_CURSOR_CONTROL		EQU		000000006h
; 		Default - 0x00
;  Enable Direct Cursor Control Register
TVP3026_CURSOR_USE_DIRECT_CCR		EQU		000000080h
;  Disable Direct Cursor Control Register
TVP3026_CURSOR_USE_INDEX_CCR		EQU		000000000h
;  Detect odd field as 1
TVP3026_CURSOR_INTERLACE_ODD		EQU		000000040h
;  Detect even field as 1
TVP3026_CURSOR_INTERLACE_EVEN		EQU		000000000h
;  Enable interlaced cursor
TVP3026_CURSOR_INTERLACE_ON		EQU		000000020h
;  Disable interlaced cursor
TVP3026_CURSOR_INTERLACE_OFF		EQU		000000000h
;  Blank is detected after 4096
TVP3026_CURSOR_VBLANK_4096		EQU		000000010h
;  		or 2048 dot clocks
TVP3026_CURSOR_VBLANK_2048		EQU		000000000h
;  High bits of cursor RAM address
;  Mask for high bits of cursor RAM address
TVP3026_CURSOR_RAM_MASK		EQU		00000000ch
;  CURSOR_OFF							0x00	// Cursor off
;  CURSOR_COLOR							0x01	// 2-bits select color
;  CURSOR_XGA							0x02	// 2-bits select XOR
;  CURSOR_XWIN							0x03	// 2-bits select transparency/color
; ******************************************************************************
; 				INDIRECT REGISTER - LATCH CONTROL								
; ******************************************************************************
;   Latch control register - 
__TVP3026_LATCH_CONTROL		EQU		00000000fh
; 		Default - 0x06
;  All modes except packed-24
TVP3026_LATCH_ALL_MODES		EQU		000000006h
;  4:3 or 8:3 packed-24 
TVP3026_LATCH_4_3		EQU		000000007h
;  5:2  packed-24 
TVP3026_LATCH_5_2		EQU		000000020h
;  5:4  packed-24 x1 horz zoom
TVP3026_LATCH_5_4_1		EQU		00000001fh
;  5:4  packed-24 x2 horz zoom
TVP3026_LATCH_5_4_2		EQU		00000001eh
;  5:4  packed-24 x4 horz zoom
TVP3026_LATCH_5_4_4		EQU		00000001ch
;  5:4  packed-24 x8 horz zoom
TVP3026_LATCH_5_4_8		EQU		000000018h
; ******************************************************************************
; 				INDIRECT REGISTER - TRUE COLOR CONTROL							
; ******************************************************************************
;   True Color control
__TVP3026_TRUE_COLOR		EQU		000000018h
; 		Default - 0x80
; ******************************************************************************
; 				INDIRECT REGISTER - MULTIPLEX CONTROL							
; ******************************************************************************
;   Multiplex control
__TVP3026_MULTIPLEX_CONTROL		EQU		000000019h
; 		Default - 0x98
; ******************************************************************************
; 				INDIRECT REGISTER - CLOCK SELECTION								
; ******************************************************************************
;   
__TVP3026_CLOCK		EQU		00000001ah
; 		Default - 0x07
;  Enable SCLK output
TVP3026_SCLK_ENABLE		EQU		000000080h
;  Disable SCLK output
TVP3026_SCLK_DISABLE		EQU		000000000h
;  VCLK forced to Logical "0"
TVP3026_VCLK_ZERO		EQU		000000070h
;  VCLK is equal to Dot clock
TVP3026_VCLK_DOTCLOCK		EQU		000000000h
;  VCLK is equal to Dot clock/2
TVP3026_VCLK_DOTCLOCK_DIV2		EQU		000000010h
;  VCLK is equal to Dot clock/4
TVP3026_VCLK_DOTCLOCK_DIV4		EQU		000000020h
;  VCLK is equal to Dot clock/8
TVP3026_VCLK_DOTCLOCK_DIV8		EQU		000000030h
;  VCLK is equal to Dot clock/16
TVP3026_VCLK_DOTCLOCK_DIV16		EQU		000000040h
;  VCLK is equal to Dot clock/32
TVP3026_VCLK_DOTCLOCK_DIV32		EQU		000000050h
;  VCLK is equal to Dot clock/64
TVP3026_VCLK_DOTCLOCK_DIV64		EQU		000000060h
;  Select CLK0 as clock source
TVP3026_CLK_CLK0		EQU		000000000h
;  Select CLK1 as clock source
TVP3026_CLK_CLK1		EQU		000000001h
;  Select CLK2 as clock source
TVP3026_CLK_CLK2_TTL		EQU		000000002h
;  Select /CLK2 as clock source
TVP3026_CLK_CLK2N_TTL		EQU		000000003h
;  Select CLK2 and /CLK2 as ECL clock source
TVP3026_CLK_CLK2_ECL		EQU		000000004h
;  Select Pixel Clock PLL as clock source
TVP3026_CLK_PIXEL_PLL		EQU		000000005h
;  Disable clock source / Power-save mode
TVP3026_CLK_DISABLE		EQU		000000006h
;  Select CLK0 as clock source with VGA latching
TVP3026_CLK_CLK0_VGA		EQU		000000007h
; ******************************************************************************
; 				INDIRECT REGISTER - PALETTE PAGE								
; ******************************************************************************
;   
__TVP3026_PALETTE_PAGE		EQU		00000001ch
; 		Default - 0x00
; ******************************************************************************
; 				INDIRECT REGISTER - GENERAL CONTROL								
; ******************************************************************************
;   
__TVP3026_GENERAL_CONTROL		EQU		00000001dh
; 		Default - 0x00
;  Enable overscan coloring
TVP3026_OVERSCAN_ENABLE		EQU		000000040h
;  Disable overscan coloring
TVP3026_OVERSCAN_DISABLE		EQU		000000000h
;  Enable SYNC signal on IOG
TVP3026_SYNC_ENABLE		EQU		000000020h
;  Disable SYNC signal on IOG
TVP3026_SYNC_DISABLE		EQU		000000000h
;  Enable 7.5 IRE blanking pedestal 
TVP3026_PEDESTAL_ON		EQU		000000010h
;  Disable blanking pedestal
TVP3026_PEDESTAL_OFF		EQU		000000000h
;  Big Endian format on pixel bus
TVP3026_BIG_ENDIAN		EQU		000000008h
;  Little Endian format on pixel bus
TVP3026_LITTLE_ENDIAN		EQU		000000000h
;  Invert VSYNC signal on VSYNCOUT
TVP3026_VSYNC_INVERT		EQU		000000002h
;  Do not invert VSYNC signal on VSYNCOUT
TVP3026_VSYNC_NORMAL		EQU		000000000h
;  Invert HSYNC signal on HSYNCOUT
TVP3026_HSYNC_INVERT		EQU		000000001h
;  Do not invert HSYNC signal on HSYNCOUT
TVP3026_HSYNC_NORMAL		EQU		000000000h
; ******************************************************************************
; 				INDIRECT REGISTER - MISC CONTROL								
; ******************************************************************************
;   
__TVP3026_MISC_CONTROL		EQU		00000001eh
; 		Default - 0x00
;  PSEL == 1 - Pseudo/True Color
TVP3026_PSEL_INVERT		EQU		000000020h
;  PSEL == 1 - Direct Color
TVP3026_PSEL_NORMAL		EQU		000000000h
;  PSEL controls Color Switching
TVP3026_PSEL_ENABLE		EQU		000000010h
;  PSEL is disabled
TVP3026_PSEL_DISABLE		EQU		000000000h
;  DAC is in 8-bit mode
TVP3026_DAC_8BIT		EQU		000000008h
;  DAC is in 6-bit mode
TVP3026_DAC_6BIT		EQU		000000000h
;  Disable 6/8 pin and use bit 3 of this register
TVP3026_DAC_6BITPIN_DISABLE		EQU		000000004h
;  Use 6/8 pin and ignore bit 3 of this register
TVP3026_DAC_6BITPIN_ENABLE		EQU		000000000h
;  Turn DAC Power on 
TVP3026_DAC_POWER_ON		EQU		000000000h
;  Turn DAC Power off 
TVP3026_DAC_POWER_OFF		EQU		000000001h
; ******************************************************************************
; 				INDIRECT REGISTER - GP I/O CONTROL								
; ******************************************************************************
;   
__TVP3026_GP_CONTROL		EQU		00000002ah
; 		Default - 0x00
; ******************************************************************************
; 				INDIRECT REGISTER - GP I/O DATA									
; ******************************************************************************
;   
__TVP3026_GP_DATA		EQU		00000002bh
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - PLL ADDRESS									
; ******************************************************************************
;   
__TVP3026_PLL_ADDRESS		EQU		00000002ch
; 		Default - undefined
;  Start Pixel Clock Programming
TVP3026_PIXEL_CLOCK_START		EQU		0000000fch
;  Start MCLK Programming
TVP3026_MCLK_START		EQU		0000000f3h
;  Start Loop Clock Programming
TVP3026_LOOP_CLOCK_START		EQU		0000000cfh
; ******************************************************************************
; 				INDIRECT REGISTER - PLL PIXEL DATA								
; ******************************************************************************
;   
__TVP3026_PLL_PIX_DATA		EQU		00000002dh
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - PLL MEMORY DATA								
; ******************************************************************************
;   
__TVP3026_PLL_MEM_DATA		EQU		00000002eh
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - PLL LOOP DATA								
; ******************************************************************************
;   
__TVP3026_PLL_LOOP_DATA		EQU		00000002fh
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY OVERLAY LOW						
; ******************************************************************************
;   
__TVP3026_CCOVR_LOW		EQU		000000030h
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY OVERLAY HIGH						
; ******************************************************************************
;   
__TVP3026_CCOVR_HIGH		EQU		000000031h
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY RED LOW							
; ******************************************************************************
;   
__TVP3026_CCRED_LOW		EQU		000000032h
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY RED HIGH							
; ******************************************************************************
;   
__TVP3026_CCRED_HIGH		EQU		000000033h
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY GREEN LOW							
; ******************************************************************************
;   
__TVP3026_CCGREEN_LOW		EQU		000000034h
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY RED HIGH							
; ******************************************************************************
;   
__TVP3026_CCGREEN_HIGH		EQU		000000035h
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY BLUE LOW							
; ******************************************************************************
;   
__TVP3026_CCBLUE_LOW		EQU		000000036h
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY BLUE HIGH							
; ******************************************************************************
;   
__TVP3026_CCBLUE_HIGH		EQU		000000037h
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - COLOR KEY CONTROL							
; ******************************************************************************
;   
__TVP3026_CC_CONTROL		EQU		000000038h
; 		Default - 0x00
; ******************************************************************************
; 				INDIRECT REGISTER - MCLK/LOOP CONTROL							
; ******************************************************************************
;   
__TVP3026_MCLK_CONTROL		EQU		000000039h
; 		Default - 0x18
;  RCLK is made from LCLK - all other modes
TVP3026_RCLK_LOOP		EQU		000000020h
;  RCLK is clocked by Pixel Clock (VGA Mode)
TVP3026_RCLK_PIXEL		EQU		000000000h
;  MCLK from PLL - normal mode
TVP3026_MCLK_PLL		EQU		000000010h
;  MCLK from dot clock - during freq. change
TVP3026_MCLK_DOT		EQU		000000000h
;  Strobe high for bit 4
TVP3026_MCLK_STROBE_HIGH		EQU		000000008h
;  Strobe low for bit 4
TVP3026_MCLK_STROBE_LOW		EQU		000000000h
;  Divide Loop clock by 2
TVP3026_LOOP_DIVIDE2		EQU		000000000h
;  Divide Loop clock by 4
TVP3026_LOOP_DIVIDE4		EQU		000000001h
;  Divide Loop clock by 6
TVP3026_LOOP_DIVIDE6		EQU		000000002h
;  Divide Loop clock by 8
TVP3026_LOOP_DIVIDE8		EQU		000000003h
;  Divide Loop clock by 10
TVP3026_LOOP_DIVIDE10		EQU		000000004h
;  Divide Loop clock by 12
TVP3026_LOOP_DIVIDE12		EQU		000000005h
;  Divide Loop clock by 14
TVP3026_LOOP_DIVIDE14		EQU		000000006h
;  Divide Loop clock by 16
TVP3026_LOOP_DIVIDE16		EQU		000000007h
; ******************************************************************************
; 				INDIRECT REGISTER - SENSE TEST									
; ******************************************************************************
;   
__TVP3026_SENSE_TEST		EQU		00000003ah
; 		Default - 0x00
; ******************************************************************************
; 				INDIRECT REGISTER - TEST MODE DATA									
; ******************************************************************************
;   
__TVP3026_TEST_MODE		EQU		00000003bh
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - CRC REMAINDER LSB							
; ******************************************************************************
;   
__TVP3026_CRC_LSB		EQU		00000003ch
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - CRC REMAINDER MSB							
; ******************************************************************************
;   
__TVP3026_CRC_MSB		EQU		00000003dh
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - CRC BITS SELECT								
; ******************************************************************************
;   
__TVP3026_CRC_SELECT		EQU		00000003eh
; 		Default - undefined
; ******************************************************************************
; 				INDIRECT REGISTER - CHIP ID										
; ******************************************************************************
;   
__TVP3026_CHIP_ID		EQU		00000003fh
; 		Default - 0x26
;  predefined
TVP3026_ID_CODE		EQU		000000026h
; ******************************************************************************
; 				INDIRECT REGISTER - SOFTWARE RESET										
; ******************************************************************************
;   
__TVP3026_SOFT_RESET		EQU		0000000ffh
; 		Default - undefined
; 
;  On rev 1 chips we need to SYNC with GLINT while accessing the RAMDAC. This
;  is because accesses to the RAMDAC can be corrupted by localbuffer
;  activity. Put this macro before accesses that can co-exist with GLINT
;  3D activity, Must have initialized glintInfo before using this.
; 
;  macro to load a given data value into an internal TVP3026 register.
; 
;  macros to write a given RGB triplet into cursors 0, 1 and 2
;  macros to load a given RGB triple into the TVP3026 palette. Send the starting
;  index and then send RGB triples. Auto-increment is turned on.
;  Use TVP3026_PALETTE_START and multiple TVP3026_LOAD_PALETTE calls to load
;  a contiguous set of entries. Use TVP3026_LOAD_PALETTE_INDEX to load a set
;  of sparse entries.
; 
;  macro to read back a given RGB triple from the TVP3026 palette. Use after
;  a call to TVP3026_PALETTE_START_RD
; 
;  macros to set/get the pixel read mask. The mask is 8 bits wide and gets
;  replicated across all bytes that make up a pixel.
; 
;  macros to load values into the cursor array
; 
;  macro to move the cursor
; 
;  macro to change the cursor hotspot
; 
; End of file tvp3026.h
; 
;  Supported RAMDAC definitions.
; 
RGB525_RAMDAC		EQU		0t
RGB526_RAMDAC		EQU		1t
RGB526DB_RAMDAC		EQU		2t
RGB528_RAMDAC		EQU		3t
RGB528A_RAMDAC		EQU		4t
RGB524_RAMDAC		EQU		6t
RGB524A_RAMDAC		EQU		7t

TVP3026_RAMDAC		EQU		50t

TVP4020_RAMDAC		EQU		100t
; End of file ramdac.h
