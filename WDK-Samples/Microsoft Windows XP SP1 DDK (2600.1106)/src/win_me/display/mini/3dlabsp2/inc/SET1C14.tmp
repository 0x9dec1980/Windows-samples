;/******************************Module*Header**********************************\
; *
; *                           *****************************
; *                           * Permedia 2: SAMPLE CODE   *
; *                           *****************************
; *
; * Module Name: glintreg.h
; *
; *              GLINT register window definition
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/
;
;       Segment defining GLINT registers
;

.586

ifdef MINIVDD

; ******************************
; Minivdd defines as a structure
; ******************************
    USESTRUCT=1

; Header to define the structure
GLINTREGHEADER macro
GlintReg struc
    endm

; Tail to define the structure
GLINTREGTAIL macro
GlintReg ends
    endm

; Single register definition
GLINTREGISTER macro name, offset
name            dd   ?,?   
    endm

; 4K Funny definition
GLINTREGISTER4K macro name, offset
name            dd   ?   
    endm

; Same offset with 2 different names
GLINTREGISTERPAIR macro name1, name2, offset
UNION                                                    
      name1     dd   ?,? 
      name2     dd   ?,? 
ENDS
endm 

; Space between register definitions                                                    
SPACEREGISTERS macro nBytes
	db		nBytes dup (?)
    endm

else


ifdef CODE32BPP
; *****************************
; 32 bit code defines as labels
; *****************************
    USESTRUCT=0

; Header to define the structure
GLINTREGHEADER macro
    endm

; Tail to define the structure
GLINTREGTAIL macro
    endm

; Single register definition
GLINTREGISTER macro name, offset
name    equ dword ptr ds:[GLINT_REGISTERS_OFFSET + offset]    
    endm

; 4K Funny definition
GLINTREGISTER4K macro name, offset
name    equ dword ptr ds:[GLINT_REGISTERS_OFFSET + offset]    
    endm

; Same offset with 2 different names
GLINTREGISTERPAIR macro name1, name2, offset
name1    equ dword ptr ds:[GLINT_REGISTERS_OFFSET + offset]    
name2    equ dword ptr ds:[GLINT_REGISTERS_OFFSET + offset]    
    endm

; Space between register definitions                                                    
SPACEREGISTERS macro nBytes
    endm
else



; ********************************
; 16 bit code defines as a segment
; ********************************
    USESTRUCT=0

; Header to define the structure
GLINTREGHEADER macro
Glint segment use16 at 0
    endm

; Tail to define the structure
GLINTREGTAIL macro
Glint ends
    endm

; Single register definition
GLINTREGISTER macro name, offset
name            dd   ?,?   
    endm

; 4K Funny definition
GLINTREGISTER4K macro name, offset
name            dd   ?   
    endm

; Same offset with 2 different names
GLINTREGISTERPAIR macro name1, name2, offset
name1     label dword
name2           dd   ?,?
endm

; Space between register definitions                                                    
SPACEREGISTERS macro nBytes
	db		nBytes dup (?)
    endm
endif
endif

GLINTREGHEADER
;
;       Control and Status registers
;

GLINTREGISTER       ResetStatus                 , 0h
GLINTREGISTER       IntEnable                   , 8h
GLINTREGISTER       IntFlags                    , 10h
GLINTREGISTER       InFIFOSpace                 , 18h
GLINTREGISTER       OutFIFOWords                , 20h
GLINTREGISTER       DMAAddress                  , 28h
GLINTREGISTER       DMACount                    , 30h
GLINTREGISTER       ErrorFlags                  , 38h
GLINTREGISTER       VClkCtl                     , 40h
GLINTREGISTER       TestRegister                , 48h
GLINTREGISTER       Aperture0                   , 50h
GLINTREGISTER       Aperture1                   , 58h
GLINTREGISTER       DMAControl                  , 60h
GLINTREGISTER       FIFODiscon		            , 68h
GLINTREGISTER       ChipConfig		            , 70h
GLINTREGISTER       Unused78		            , 78h
GLINTREGISTER       OutDMAAddress	            , 80h
GLINTREGISTER       OutDMACount		            , 88h
GLINTREGISTER       AGPTexBaseAddress           , 90h
GLINTREGISTER       Unused98                    , 98h
GLINTREGISTER       ByDMAAddress                , A0h   P2
GLINTREGISTER       UnusedA8                    , A8h
GLINTREGISTER       UnusedB0                    , B0h
GLINTREGISTER       ByDMAStride                 , B8h   P2
GLINTREGISTER       ByDMAMemAddr                , C0h   P2
GLINTREGISTER       ByDMASize                   , C8h   P2
GLINTREGISTER       ByDMAByteMask               , D0h   P2
GLINTREGISTER       ByDMAControl                , D8h   P2
GLINTREGISTER       UnusedE0                    , E0h
GLINTREGISTER       ByDMAComplete               , E8h   P2
GLINTREGISTER       UnusedF0                    , F0h
GLINTREGISTER       UnusedF8                    , F8h
SPACEREGISTERS      800h-100h

GLINTREGISTER       DeltaReset                  , 800h
GLINTREGISTER       DeltaIntEnable              , 808h
GLINTREGISTER       DeltaIntFlags               , 810h
GLINTREGISTER       DeltaInFIFOSpaceXXX         , 818h
GLINTREGISTER       DeltaOutFIFOWordsXXX        , 820h
GLINTREGISTER       DeltaDMAAddressXXX          , 828h
GLINTREGISTER       DeltaDMACountXXX            , 830h
GLINTREGISTER       DeltaErrorFlags             , 838h
GLINTREGISTER       DeltaVClkCtlXXX             , 840h
GLINTREGISTER       DeltaTestRegister           , 848h
GLINTREGISTER       DeltaAperture0XXX           , 850h
GLINTREGISTER       DeltaAperture1XXX           , 858h
GLINTREGISTER       DeltaDMAControlXXX          , 860h
GLINTREGISTER       DeltaDisconnectControl      , 868h
SPACEREGISTERS      1000h-870h
    
;
;       Localbuffer Registers
;

GLINTREGISTERPAIR   LBMemoryCtl, Reboot         , 1000h     ; LBMemoryCtl TX, Reboot Permedia
GLINTREGISTER       LBMemoryEDO			        , 1008h     ; TX
SPACEREGISTERS      1040h-1010h
GLINTREGISTER       RomControl			        , 1040h     ; Permedia
SPACEREGISTERS      1080h-1048h
GLINTREGISTER       BootAddress			        , 1080h     ; Permedia
SPACEREGISTERS      10C0h-1088h
GLINTREGISTER       MemConfig 			        , 10C0h     ; Permedia
SPACEREGISTERS      1100h-10C8h
GLINTREGISTER       BypassWriteMask		        , 1100h     ; Permedia
SPACEREGISTERS      1140h-1108h
GLINTREGISTER       FramebufferWriteMask        , 1140h     ; Permedia
SPACEREGISTERS      1180h-1148h
GLINTREGISTER       MClkCount			        , 1180h     ; Permedia
SPACEREGISTERS      1800h-1188h

;
;       Framebuffer Registers
;

GLINTREGISTER       FBMemoryCtl                 , 1800h
GLINTREGISTER       FBModeSel                   , 1808h
GLINTREGISTER       FBGPWrMask                  , 1810h
GLINTREGISTER       FBGPColorMask               , 1818h
SPACEREGISTERS      2000h-1820h

;
;       GP Fifo Interface
;

GLINTREGISTER4K     GPFifo1                     , 2000h
GLINTREGISTER4K     GPFifo2                     , 2004h
GLINTREGISTER4K     GPFifo3                     , 2008h
GLINTREGISTER4K     GPFifo4                     , 200ch
GLINTREGISTER4K     GPFifo5                     , 2010h
GLINTREGISTER4K     GPFifo6                     , 2014h
GLINTREGISTER4K     GPFifo7                     , 2018h
GLINTREGISTER4K     GPFifo8                     , 201ch
GLINTREGISTER4K     GPFifo9                     , 2020h
GLINTREGISTER4K     GPFifo10                    , 2024h
GLINTREGISTER4K     GPFifo11                    , 2028h
GLINTREGISTER4K     GPFifo12                    , 202ch
GLINTREGISTER4K     GPFifo13                    , 2030h
GLINTREGISTER4K     GPFifo14                    , 2034h
GLINTREGISTER4K     GPFifo15                    , 2038h
GLINTREGISTER4K     GPFifo16                    , 203ch
GLINTREGISTER4K     GPFifo17                    , 2040h
GLINTREGISTER4K     GPFifo18                    , 2044h
GLINTREGISTER4K     GPFifo19                    , 2048h
GLINTREGISTER4K     GPFifo20                    , 204ch
GLINTREGISTER4K     GPFifo21                    , 2050h
GLINTREGISTER4K     GPFifo22                    , 2054h
GLINTREGISTER4K     GPFifo23                    , 2058h
GLINTREGISTER4K     GPFifo24                    , 205ch
GLINTREGISTER4K     GPFifo25                    , 2060h
GLINTREGISTER4K     GPFifo26                    , 2064h
GLINTREGISTER4K     GPFifo27                    , 2068h
GLINTREGISTER4K     GPFifo28                    , 206ch
GLINTREGISTER4K     GPFifo29                    , 2070h
GLINTREGISTER4K     GPFifo30                    , 2074h
GLINTREGISTER4K     GPFifo31                    , 2078h
GLINTREGISTER4K     GPFifo32                    , 207ch
GLINTREGISTER4K     GPFifo33                    , 2080h
GLINTREGISTER4K     GPFifo34                    , 2084h
GLINTREGISTER4K     GPFifo35                    , 2088h
GLINTREGISTER4K     GPFifo36                    , 208ch
GLINTREGISTER4K     GPFifo37                    , 2090h
GLINTREGISTER4K     GPFifo38                    , 2094h
GLINTREGISTER4K     GPFifo39                    , 2098h
GLINTREGISTER4K     GPFifo40                    , 209ch
GLINTREGISTER4K     GPFifo41                    , 20a0h
GLINTREGISTER4K     GPFifo42                    , 20a4h
GLINTREGISTER4K     GPFifo43                    , 20a8h
GLINTREGISTER4K     GPFifo44                    , 20ach
GLINTREGISTER4K     GPFifo45                    , 20b0h
GLINTREGISTER4K     GPFifo46                    , 20b4h
GLINTREGISTER4K     GPFifo47                    , 20b8h
GLINTREGISTER4K     GPFifo48                    , 20bch
GLINTREGISTER4K     GPFifo49                    , 20c0h
GLINTREGISTER4K     GPFifo50                    , 20c4h
GLINTREGISTER4K     GPFifo51                    , 20c8h
GLINTREGISTER4K     GPFifo52                    , 20cch
GLINTREGISTER4K     GPFifo53                    , 20d0h
GLINTREGISTER4K     GPFifo54                    , 20d4h
GLINTREGISTER4K     GPFifo55                    , 20d8h
GLINTREGISTER4K     GPFifo56                    , 20dch
GLINTREGISTER4K     GPFifo57                    , 20e0h
GLINTREGISTER4K     GPFifo58                    , 20e4h
GLINTREGISTER4K     GPFifo59                    , 20e8h
GLINTREGISTER4K     GPFifo60                    , 20ech
GLINTREGISTER4K     GPFifo61                    , 20f0h
GLINTREGISTER4K     GPFifo62                    , 20f4h
GLINTREGISTER4K     GPFifo63                    , 20f8h
GLINTREGISTER4K     GPFifo64                    , 20fch
SPACEREGISTERS      3000h-2100h

;
;       Internal Video Registers SX/TX only at the moment
;

GLINTREGISTERPAIR   VTGHCount       , ScreenBase    , 3000h
GLINTREGISTERPAIR   VTGHSyncStart   , ScreenStride  , 3008h
GLINTREGISTERPAIR   VTGHSyncEnd     , HTotal        , 3010h
GLINTREGISTERPAIR   VTGHBlankEnd    , HgEnd         , 3018h
GLINTREGISTERPAIR   VTGVCount       , HbEnd         , 3020h
GLINTREGISTERPAIR   VTGVSyncStart   , HsStart       , 3028h
GLINTREGISTERPAIR   VTGVSyncEnd     , HsEnd         , 3030h
GLINTREGISTERPAIR   VTGVBlankEnd    , VTotal        , 3038h
GLINTREGISTERPAIR   VTGHGateStart   , VbEnd         , 3040h
GLINTREGISTERPAIR   VTGHGateEnd     , VsStart       , 3048h
GLINTREGISTERPAIR   VTGVGateStart   , VsEnd         , 3050h
GLINTREGISTERPAIR   VTGVGateEnd     , VideoControl  , 3058h  
GLINTREGISTERPAIR   VTGPolarity     , InterruptLine , 3060h
GLINTREGISTERPAIR   VTGHighFrameAddr, DDCData       , 3068h
GLINTREGISTERPAIR   VTGLowFrameAddr , LineCount     , 3070h
GLINTREGISTER       VTGSerialClk                    , 3078h
SPACEREGISTERS      4000h-3080h

;
;       External Video Control
;
GLINTREGISTER       ExtVC                       , 4000h
SPACEREGISTERS      63c4h-4008h

GLINTREGISTER       VGAControlReg               , 63c4h
SPACEREGISTERS      8000h-63cch

;
;       Graphics Processor
;
GLINTREGISTER       StartXDom                   , (8000h+8h*00h)
GLINTREGISTER       dXDom                       , (8000h+8h*01h)
GLINTREGISTER       StartXSub                   , (8000h+8h*02h)
GLINTREGISTER       dXSub                       , (8000h+8h*03h)
GLINTREGISTER       StartY                      , (8000h+8h*04h)
GLINTREGISTER       dY                          , (8000h+8h*05h)
GLINTREGISTER       Count                       , (8000h+8h*06h)
GLINTREGISTER       Render                      , (8000h+8h*07h)
GLINTREGISTER       ContinueNewLine             , (8000h+8h*08h)
GLINTREGISTER       ContinueNewDom              , (8000h+8h*09h)
GLINTREGISTER       ContinueNewSub              , (8000h+8h*0Ah)
GLINTREGISTER       Continue                    , (8000h+8h*0Bh)
GLINTREGISTER       FlushSpan                   , (8000h+8h*0Ch)
GLINTREGISTER       BitMaskPattern              , (8000h+8h*0Dh)
GLINTREGISTER       Blank00E                    , (8000h+8h*0Eh)
GLINTREGISTER       Blank00F                    , (8000h+8h*0Fh)

GLINTREGISTER       PointTable0                 , (8000h+8h*10h)
GLINTREGISTER       PointTable1                 , (8000h+8h*11h)
GLINTREGISTER       PointTable2                 , (8000h+8h*12h)
GLINTREGISTER       PointTable3                 , (8000h+8h*13h)
GLINTREGISTER       RasterizerMode              , (8000h+8h*14h)
GLINTREGISTER       YLimits		      		    , (8000h+8h*15h)
GLINTREGISTER       ScanlineOwnership   	    , (8000h+8h*16h)
GLINTREGISTER       WaitForCompletion		    , (8000h+8h*17h)
GLINTREGISTER       PixelSize		       	    , (8000h+8h*18h)
GLINTREGISTER       XLimits		                , (8000h+8h*19h)
GLINTREGISTER       RectangleOrigin             , (8000h+8h*1Ah)
GLINTREGISTER       RectangleSize               , (8000h+8h*1Bh)
GLINTREGISTER       Blank01C                    , (8000h+8h*1Ch)
GLINTREGISTER       Blank01D                    , (8000h+8h*1Dh)
GLINTREGISTER       Blank01E                    , (8000h+8h*1Eh)
GLINTREGISTER       Blank01F                    , (8000h+8h*1Fh)

GLINTREGISTER       CoverageValue               , (8000h+8h*20h)
GLINTREGISTER       PrepareToRender             , (8000h+8h*21h)
GLINTREGISTER       ActiveStepX                 , (8000h+8h*22h)
GLINTREGISTER       PassiveStepX                , (8000h+8h*23h)
GLINTREGISTER       ActiveStepYDomEdge          , (8000h+8h*24h)
GLINTREGISTER       PassiveStepYDomEdge         , (8000h+8h*25h)
GLINTREGISTER       FastBlockLimits             , (8000h+8h*26h)
GLINTREGISTER       FastBlockFill               , (8000h+8h*27h)
GLINTREGISTER       SubPixelCorrection          , (8000h+8h*28h)
GLINTREGISTER       ForceBackgroundColor        , (8000h+8h*29h)
GLINTREGISTER       PackedDataLimits            , (8000h+8h*2ah)
GLINTREGISTER       SpanStepX         		    , (8000h+8h*2bh)
GLINTREGISTER       SpanStepYDomEdge            , (8000h+8h*2ch)
GLINTREGISTER       SpanMask                    , (8000h+8h*2dh)
GLINTREGISTER       SuspendReads                , (8000h+8h*2eh)
GLINTREGISTER       Blank02F                    , (8000h+8h*2Fh)

GLINTREGISTER       ScissorMode                 , (8000h+8h*30h)
GLINTREGISTER       ScissorMinXY                , (8000h+8h*31h)
GLINTREGISTER       ScissorMaxXY                , (8000h+8h*32h)
GLINTREGISTER       ScreenSize                  , (8000h+8h*33h)
GLINTREGISTER       AreaStippleMode             , (8000h+8h*34h)
GLINTREGISTER       LineStippleMode             , (8000h+8h*35h)
GLINTREGISTER       LoadLineStippleCounters     , (8000h+8h*36h)
GLINTREGISTER       UpdateLineStippleCounters   , (8000h+8h*37h)
GLINTREGISTER       SaveLineStippleCounters     , (8000h+8h*38h)
GLINTREGISTER       WindowOrigin                , (8000h+8h*39h)
GLINTREGISTER       Blank03A                    , (8000h+8h*3Ah)
GLINTREGISTER       Blank03B                    , (8000h+8h*3Bh)
GLINTREGISTER       Blank03C                    , (8000h+8h*3Ch)
GLINTREGISTER       Blank03D                    , (8000h+8h*3Dh)
GLINTREGISTER       Blank03E                    , (8000h+8h*3Eh)
GLINTREGISTER       Blank03F                    , (8000h+8h*3Fh)

GLINTREGISTER       AreaStipplePattern0         , (8000h+8h*40h)
GLINTREGISTER       AreaStipplePattern1         , (8000h+8h*41h)
GLINTREGISTER       AreaStipplePattern2         , (8000h+8h*42h)
GLINTREGISTER       AreaStipplePattern3         , (8000h+8h*43h)
GLINTREGISTER       AreaStipplePattern4         , (8000h+8h*44h)
GLINTREGISTER       AreaStipplePattern5         , (8000h+8h*45h)
GLINTREGISTER       AreaStipplePattern6         , (8000h+8h*46h)
GLINTREGISTER       AreaStipplePattern7         , (8000h+8h*47h)
GLINTREGISTER       AreaStipplePattern8         , (8000h+8h*48h)
GLINTREGISTER       AreaStipplePattern9         , (8000h+8h*49h)
GLINTREGISTER       AreaStipplePattern10        , (8000h+8h*4ah)
GLINTREGISTER       AreaStipplePattern11        , (8000h+8h*4bh)
GLINTREGISTER       AreaStipplePattern12        , (8000h+8h*4ch)
GLINTREGISTER       AreaStipplePattern13        , (8000h+8h*4dh)
GLINTREGISTER       AreaStipplePattern14        , (8000h+8h*4eh)
GLINTREGISTER       AreaStipplePattern15        , (8000h+8h*4fh)
GLINTREGISTER       AreaStipplePattern16        , (8000h+8h*50h)
GLINTREGISTER       AreaStipplePattern17        , (8000h+8h*51h)
GLINTREGISTER       AreaStipplePattern18        , (8000h+8h*52h)
GLINTREGISTER       AreaStipplePattern19        , (8000h+8h*53h)
GLINTREGISTER       AreaStipplePattern20        , (8000h+8h*54h)
GLINTREGISTER       AreaStipplePattern21        , (8000h+8h*55h)
GLINTREGISTER       AreaStipplePattern22        , (8000h+8h*56h)
GLINTREGISTER       AreaStipplePattern23        , (8000h+8h*57h)
GLINTREGISTER       AreaStipplePattern24        , (8000h+8h*58h)
GLINTREGISTER       AreaStipplePattern25        , (8000h+8h*59h)
GLINTREGISTER       AreaStipplePattern26        , (8000h+8h*5ah)
GLINTREGISTER       AreaStipplePattern27        , (8000h+8h*5bh)
GLINTREGISTER       AreaStipplePattern28        , (8000h+8h*5ch)
GLINTREGISTER       AreaStipplePattern29        , (8000h+8h*5dh)
GLINTREGISTER       AreaStipplePattern30        , (8000h+8h*5eh)
GLINTREGISTER       AreaStipplePattern31        , (8000h+8h*5fh)

GLINTREGISTER       Blank060                    , (8000h+8h*60h)
GLINTREGISTER       Blank061                    , (8000h+8h*61h)
GLINTREGISTER       Blank062                    , (8000h+8h*62h)
GLINTREGISTER       Blank063                    , (8000h+8h*63h)
GLINTREGISTER       Blank064                    , (8000h+8h*64h)
GLINTREGISTER       Blank065                    , (8000h+8h*65h)
GLINTREGISTER       Blank066                    , (8000h+8h*66h)
GLINTREGISTER       Blank067                    , (8000h+8h*67h)
GLINTREGISTER       Blank068                    , (8000h+8h*68h)
GLINTREGISTER       Blank069                    , (8000h+8h*69h)
GLINTREGISTER       Blank06A                    , (8000h+8h*6Ah)
GLINTREGISTER       Blank06B                    , (8000h+8h*6Bh)
GLINTREGISTER       Blank06C                    , (8000h+8h*6Ch)
GLINTREGISTER       Blank06D                    , (8000h+8h*6Dh)
GLINTREGISTER       Blank06E                    , (8000h+8h*6Eh)
GLINTREGISTER       Blank06F                    , (8000h+8h*6Fh)
 
GLINTREGISTER       TextureAddressMode          , (8000h+8h*70h)
GLINTREGISTER       SStart			            , (8000h+8h*71h)
GLINTREGISTER       dSdx			            , (8000h+8h*72h)
GLINTREGISTER       dSdyDom		                , (8000h+8h*73h)
GLINTREGISTER       TStart			            , (8000h+8h*74h)
GLINTREGISTER       dTdx			            , (8000h+8h*75h)
GLINTREGISTER       dTdyDom		                , (8000h+8h*76h)
GLINTREGISTER       QStart			            , (8000h+8h*77h)
GLINTREGISTER       dQdx			            , (8000h+8h*78h)
GLINTREGISTER       dQdyDom		                , (8000h+8h*79h)
GLINTREGISTER       Blank07A                    , (8000h+8h*7Ah)
GLINTREGISTER       Blank07B                    , (8000h+8h*7Bh)
GLINTREGISTER       Blank07C                    , (8000h+8h*7Ch)
GLINTREGISTER       Blank07D                    , (8000h+8h*7Dh)
GLINTREGISTER       Blank07E                    , (8000h+8h*7Eh)
GLINTREGISTER       Blank07F                    , (8000h+8h*7Fh)

GLINTREGISTER       TextureAddress              , (8000h+8h*80h)
GLINTREGISTER       TexelCoordUV                , (8000h+8h*81h)
GLINTREGISTER       TexelCoordU                 , (8000h+8h*82h)
GLINTREGISTER       TexelCoordV                 , (8000h+8h*83h)
GLINTREGISTER       Blank084                    , (8000h+8h*84h)
GLINTREGISTER       Blank085                    , (8000h+8h*85h)
GLINTREGISTER       Blank086                    , (8000h+8h*86h)
GLINTREGISTER       Blank087                    , (8000h+8h*87h)
GLINTREGISTER       Blank088                    , (8000h+8h*88h)
GLINTREGISTER       Blank089                    , (8000h+8h*89h)
GLINTREGISTER       Blank08A                    , (8000h+8h*8Ah)
GLINTREGISTER       Blank08B                    , (8000h+8h*8Bh)
GLINTREGISTER       Blank08C                    , (8000h+8h*8Ch)
GLINTREGISTER       Blank08D                    , (8000h+8h*8Dh)
GLINTREGISTER       Blank08E                    , (8000h+8h*8Eh)
GLINTREGISTER       Blank08F                    , (8000h+8h*8Fh)
 
GLINTREGISTER       TxTextureReadMode           , (8000h+8h*90h)
GLINTREGISTER       TextureFormat               , (8000h+8h*91h)
GLINTREGISTER       TextureCacheControl         , (8000h+8h*92h)
GLINTREGISTER       TexelData0                  , (8000h+8h*93h)
GLINTREGISTER       TexelData1                  , (8000h+8h*94h)
GLINTREGISTER       BorderColor                 , (8000h+8h*95h)
GLINTREGISTER       LUTData                     , (8000h+8h*96h)
GLINTREGISTER       LUTDataDirect               , (8000h+8h*97h)
GLINTREGISTER       TexelLUTIndex               , (8000h+8h*98h)
GLINTREGISTER       TexelLUTData                , (8000h+8h*99h)
GLINTREGISTER       TexelLUTAddress             , (8000h+8h*9Ah)
GLINTREGISTER       TexelLUTTransfer            , (8000h+8h*9Bh)
GLINTREGISTER       TextureFilterMode           , (8000h+8h*9Ch)
GLINTREGISTER       TextureChromaUpper          , (8000h+8h*9Dh)
GLINTREGISTER       TextureChromaLower          , (8000h+8h*9Eh)
GLINTREGISTER       Blank09F                    , (8000h+8h*9Fh)

GLINTREGISTER       TxBaseAddr				    , (8000h+8h*0a0h)
GLINTREGISTER       TxBaseAddrLR			    , (8000h+8h*0a1h)
GLINTREGISTER       Blank0A2                    , (8000h+8h*0A2h)
GLINTREGISTER       Blank0A3                    , (8000h+8h*0A3h)
GLINTREGISTER       Blank0A4                    , (8000h+8h*0A4h)
GLINTREGISTER       Blank0A5                    , (8000h+8h*0A5h)
GLINTREGISTER       Blank0A6                    , (8000h+8h*0A6h)
GLINTREGISTER       Blank0A7                    , (8000h+8h*0A7h)
GLINTREGISTER       Blank0A8                    , (8000h+8h*0A8h)
GLINTREGISTER       Blank0A9                    , (8000h+8h*0A9h)
GLINTREGISTER       Blank0AA                    , (8000h+8h*0AAh)
GLINTREGISTER       Blank0AB                    , (8000h+8h*0ABh)
GLINTREGISTER       Blank0AC                    , (8000h+8h*0ACh)
GLINTREGISTER       Blank0AD                    , (8000h+8h*0ADh)
GLINTREGISTER       Blank0AE                    , (8000h+8h*0AEh)
GLINTREGISTER       Blank0AF                    , (8000h+8h*0AFh)

GLINTREGISTER       TextureBaseAddress          , (8000h+8h*0b0h)
GLINTREGISTER       TextureMapFormat            , (8000h+8h*0b1h)
GLINTREGISTER       TextureDataFormat           , (8000h+8h*0b2h)
GLINTREGISTER       Blank0b3                    , (8000h+8h*0b3h)
GLINTREGISTER       Blank0b4                    , (8000h+8h*0b4h)
GLINTREGISTER       TextureReadPad              , (8000h+8h*0b5h)
GLINTREGISTER       Blank0b6                    , (8000h+8h*0b6h)
GLINTREGISTER       Blank0b7                    , (8000h+8h*0b7h)
GLINTREGISTER       Blank0b8                    , (8000h+8h*0b8h)
GLINTREGISTER       Blank0b9                    , (8000h+8h*0b9h)
GLINTREGISTER       Blank0bA                    , (8000h+8h*0bAh)
GLINTREGISTER       Blank0bB                    , (8000h+8h*0bBh)
GLINTREGISTER       Blank0bC                    , (8000h+8h*0bCh)
GLINTREGISTER       Blank0bD                    , (8000h+8h*0bDh)
GLINTREGISTER       Blank0bE                    , (8000h+8h*0bEh)
GLINTREGISTER       Blank0bF                    , (8000h+8h*0bFh)

GLINTREGISTER       Texel0                      , (8000h+8h*0c0h)
GLINTREGISTER       Texel1                      , (8000h+8h*0c1h)
GLINTREGISTER       Texel2                      , (8000h+8h*0c2h)
GLINTREGISTER       Texel3                      , (8000h+8h*0c3h)
GLINTREGISTER       Texel4                      , (8000h+8h*0c4h)
GLINTREGISTER       Texel5                      , (8000h+8h*0c5h)
GLINTREGISTER       Texel6                      , (8000h+8h*0c6h)
GLINTREGISTER       Texel7                      , (8000h+8h*0c7h)
GLINTREGISTER       Interp0                     , (8000h+8h*0c8h)
GLINTREGISTER       Interp1                     , (8000h+8h*0c9h)
GLINTREGISTER       Interp2                     , (8000h+8h*0cah)
GLINTREGISTER       Interp3                     , (8000h+8h*0cbh)
GLINTREGISTER       Interp4                     , (8000h+8h*0cch)
GLINTREGISTER       TextureFilter               , (8000h+8h*0cdh)
GLINTREGISTER       PTextureReadMode            , (8000h+8h*0ceh)
GLINTREGISTER       TextureLUTMode              , (8000h+8h*0cfh)

GLINTREGISTER       TextureColorMode            , (8000h+8h*0d0h)
GLINTREGISTER       TextureEnvColor             , (8000h+8h*0d1h)
GLINTREGISTER       FogMode                     , (8000h+8h*0d2h)
GLINTREGISTER       FogColor                    , (8000h+8h*0d3h)
GLINTREGISTER       FStart                      , (8000h+8h*0d4h)
GLINTREGISTER       dFdx                        , (8000h+8h*0d5h)
GLINTREGISTER       dFdyDom                     , (8000h+8h*0d6h)
GLINTREGISTER       TextureKd                   , (8000h+8h*0d7h)
GLINTREGISTER       TextureKs                   , (8000h+8h*0d8h)
GLINTREGISTER       KsStart                     , (8000h+8h*0d9h)
GLINTREGISTER       dKsdx                       , (8000h+8h*0dah)
GLINTREGISTER       dKsdyDom                    , (8000h+8h*0dbh)
GLINTREGISTER       KdStart                     , (8000h+8h*0dch)
GLINTREGISTER       dKddx                       , (8000h+8h*0ddh)
GLINTREGISTER       dKddyDom                    , (8000h+8h*0deh)
GLINTREGISTER       Blank0dF                    , (8000h+8h*0dfh)

GLINTREGISTER       Blank0e0                    , (8000h+8h*0e0h)
GLINTREGISTER       Blank0e1                    , (8000h+8h*0e1h)
GLINTREGISTER       Blank0e2                    , (8000h+8h*0e2h)
GLINTREGISTER       Blank0e3                    , (8000h+8h*0e3h)
GLINTREGISTER       Blank0e4                    , (8000h+8h*0e4h)
GLINTREGISTER       Blank0e5                    , (8000h+8h*0e5h)
GLINTREGISTER       Blank0e6                    , (8000h+8h*0e6h)
GLINTREGISTER       Blank0e7                    , (8000h+8h*0e7h)
GLINTREGISTER       Blank0e8                    , (8000h+8h*0e8h)
GLINTREGISTER       Blank0e9                    , (8000h+8h*0e9h)
GLINTREGISTER       Blank0eA                    , (8000h+8h*0eAh)
GLINTREGISTER       Blank0eB                    , (8000h+8h*0eBh)
GLINTREGISTER       Blank0eC                    , (8000h+8h*0eCh)
GLINTREGISTER       Blank0eD                    , (8000h+8h*0eDh)
GLINTREGISTER       Blank0eE                    , (8000h+8h*0eEh)
GLINTREGISTER       Blank0eF                    , (8000h+8h*0eFh)

GLINTREGISTER       RStart                      , (8000h+8h*0f0h)
GLINTREGISTER       dRdx                        , (8000h+8h*0f1h)
GLINTREGISTER       dRdyDom                     , (8000h+8h*0f2h)
GLINTREGISTER       GStart                      , (8000h+8h*0f3h)
GLINTREGISTER       dGdx                        , (8000h+8h*0f4h)
GLINTREGISTER       dGdyDom                     , (8000h+8h*0f5h)
GLINTREGISTER       BStart                      , (8000h+8h*0f6h)
GLINTREGISTER       dBdx                        , (8000h+8h*0f7h)
GLINTREGISTER       dBdyDom                     , (8000h+8h*0f8h)
GLINTREGISTER       AStart                      , (8000h+8h*0f9h)
GLINTREGISTER       dAdx                        , (8000h+8h*0fah)
GLINTREGISTER       dAdyDom                     , (8000h+8h*0fbh)
GLINTREGISTER       ColorDDAMode                , (8000h+8h*0fch)
GLINTREGISTER       ConstantColor               , (8000h+8h*0fdh)
GLINTREGISTER       Color                       , (8000h+8h*0feh)
GLINTREGISTER       Blank0ff                    , (8000h+8h*0ffh)

GLINTREGISTER       AlphaTestMode               , (8000h+8h*100h)
GLINTREGISTER       AntialiasMode               , (8000h+8h*101h)
GLINTREGISTER       AlphaBlendMode              , (8000h+8h*102h)
GLINTREGISTER       DitherMode                  , (8000h+8h*103h)
GLINTREGISTER       FBSoftwareWriteMask         , (8000h+8h*104h)
GLINTREGISTER       LogicalOpMode               , (8000h+8h*105h)
GLINTREGISTER       FBWriteData                 , (8000h+8h*106h)
GLINTREGISTER       FBCancelWrite               , (8000h+8h*107h)
GLINTREGISTERPAIR   ActiveColorStepX, RouterMode, (8000h+8h*108h)
GLINTREGISTER       ActiveColorStepYDomEdge     , (8000h+8h*109h)
GLINTREGISTER       Blank10A                    , (8000h+8h*10Ah)
GLINTREGISTER       Blank10B                    , (8000h+8h*10Bh)
GLINTREGISTER       Blank10C                    , (8000h+8h*10Ch)
GLINTREGISTER       Blank10D                    , (8000h+8h*10Dh)
GLINTREGISTER       Blank10E                    , (8000h+8h*10Eh)
GLINTREGISTER       Blank10F                    , (8000h+8h*10Fh) 

GLINTREGISTER       LBReadMode                  , (8000h+8h*110h) 
GLINTREGISTER       LBReadFormat                , (8000h+8h*111h) 
GLINTREGISTER       LBSourceOffset              , (8000h+8h*112h) 
GLINTREGISTER       LBData                      , (8000h+8h*113h) 
GLINTREGISTER       LBSourceData                , (8000h+8h*114h) 
GLINTREGISTER       LBStencil                   , (8000h+8h*115h) 
GLINTREGISTER       LBDepth                     , (8000h+8h*116h) 
GLINTREGISTER       LBWindowBase                , (8000h+8h*117h) 
GLINTREGISTER       LBWriteMode                 , (8000h+8h*118h) 
GLINTREGISTER       LBWriteFormat               , (8000h+8h*119h) 
GLINTREGISTER       LBWriteBase                 , (8000h+8h*11ah) 
GLINTREGISTER       LBWriteConfig               , (8000h+8h*11bh) 
GLINTREGISTER       LBReadPad                   , (8000h+8h*11ch) 
GLINTREGISTER       TextureData                 , (8000h+8h*11dh) 
GLINTREGISTER       TextureDownloadOffset       , (8000h+8h*11eh) 
GLINTREGISTER       Blank11F                    , (8000h+8h*11Fh) 

GLINTREGISTER       Blank120                    , (8000h+8h*120h)
GLINTREGISTER       Blank121                    , (8000h+8h*121h)
GLINTREGISTER       Blank122                    , (8000h+8h*122h)
GLINTREGISTER       Blank123                    , (8000h+8h*123h)
GLINTREGISTER       Blank124                    , (8000h+8h*124h)
GLINTREGISTER       Blank125                    , (8000h+8h*125h)
GLINTREGISTER       Blank126                    , (8000h+8h*126h)
GLINTREGISTER       Blank127                    , (8000h+8h*127h)
GLINTREGISTER       Blank128                    , (8000h+8h*128h)
GLINTREGISTER       Blank129                    , (8000h+8h*129h)
GLINTREGISTER       Blank12A                    , (8000h+8h*12Ah)
GLINTREGISTER       Blank12B                    , (8000h+8h*12Bh)
GLINTREGISTER       Blank12C                    , (8000h+8h*12Ch)
GLINTREGISTER       Blank12D                    , (8000h+8h*12Dh)
GLINTREGISTER       Blank12E                    , (8000h+8h*12Eh)
GLINTREGISTER       Blank12F                    , (8000h+8h*12Fh) 

GLINTREGISTER       Window                      , (8000h+8h*130h)
GLINTREGISTER       StencilMode                 , (8000h+8h*131h)
GLINTREGISTER       StencilData                 , (8000h+8h*132h)
GLINTREGISTER       Stencil                     , (8000h+8h*133h)
GLINTREGISTER       DepthMode                   , (8000h+8h*134h)
GLINTREGISTER       Depth                       , (8000h+8h*135h)
GLINTREGISTER       ZStartU                     , (8000h+8h*136h)
GLINTREGISTER       ZStartL                     , (8000h+8h*137h)
GLINTREGISTER       dZdxU                       , (8000h+8h*138h)
GLINTREGISTER       dZdxL                       , (8000h+8h*139h)
GLINTREGISTER       dZdyDomU                    , (8000h+8h*13ah)
GLINTREGISTER       dZdyDomL                    , (8000h+8h*13bh)
GLINTREGISTER       FastClearDepth              , (8000h+8h*13ch)
GLINTREGISTER       LBCancelWrite               , (8000h+8h*13dh)
GLINTREGISTER       LBWriteData                 , (8000h+8h*13eh)
GLINTREGISTER       Blank13F                    , (8000h+8h*13Fh) 

GLINTREGISTER       Blank140                    , (8000h+8h*140h)
GLINTREGISTER       Blank141                    , (8000h+8h*141h)
GLINTREGISTER       Blank142                    , (8000h+8h*142h)
GLINTREGISTER       Blank143                    , (8000h+8h*143h)
GLINTREGISTER       Blank144                    , (8000h+8h*144h)
GLINTREGISTER       Blank145                    , (8000h+8h*145h)
GLINTREGISTER       Blank146                    , (8000h+8h*146h)
GLINTREGISTER       Blank147                    , (8000h+8h*147h)
GLINTREGISTER       Blank148                    , (8000h+8h*148h)
GLINTREGISTER       Blank149                    , (8000h+8h*149h)
GLINTREGISTER       Blank14A                    , (8000h+8h*14Ah)
GLINTREGISTER       Blank14B                    , (8000h+8h*14Bh)
GLINTREGISTER       Blank14C                    , (8000h+8h*14Ch)
GLINTREGISTER       Blank14D                    , (8000h+8h*14Dh)
GLINTREGISTER       Blank14E                    , (8000h+8h*14Eh)
GLINTREGISTER       Blank14F                    , (8000h+8h*14Fh) 

GLINTREGISTER       FBReadMode                  , (8000h+8h*150h)
GLINTREGISTER       FBSourceOffset              , (8000h+8h*151h)
GLINTREGISTER       FBPixelOffset               , (8000h+8h*152h)
GLINTREGISTER       FBColor                     , (8000h+8h*153h)
GLINTREGISTER       FBData                      , (8000h+8h*154h)
GLINTREGISTER       FBSourceData                , (8000h+8h*155h)
GLINTREGISTER       FBWindowBase                , (8000h+8h*156h)
GLINTREGISTER       FBWriteMode                 , (8000h+8h*157h)
GLINTREGISTER       FBHardwareWriteMask         , (8000h+8h*158h)
GLINTREGISTER       FBBlockColor                , (8000h+8h*159h)
GLINTREGISTER       FBReadPixel                 , (8000h+8h*15ah)
GLINTREGISTER       FBWritePixel                , (8000h+8h*15bh)
GLINTREGISTER       FBWriteBase                 , (8000h+8h*15ch)
GLINTREGISTER       FBWriteConfig               , (8000h+8h*15dh)
GLINTREGISTER       FBReadPad                   , (8000h+8h*15eh)
GLINTREGISTER       PatternRAMMode              , (8000h+8h*15fh)

GLINTREGISTER       PatternRAMData0             , (8000h+8h*160h)
GLINTREGISTER       PatternRAMData1             , (8000h+8h*161h)
GLINTREGISTER       PatternRAMData2             , (8000h+8h*162h)
GLINTREGISTER       PatternRAMData3             , (8000h+8h*163h)
GLINTREGISTER       PatternRAMData4             , (8000h+8h*164h)
GLINTREGISTER       PatternRAMData5             , (8000h+8h*165h)
GLINTREGISTER       PatternRAMData6             , (8000h+8h*166h)
GLINTREGISTER       PatternRAMData7             , (8000h+8h*167h)
GLINTREGISTER       PatternRAMData8             , (8000h+8h*168h)
GLINTREGISTER       PatternRAMData9             , (8000h+8h*169h)
GLINTREGISTER       PatternRAMData10            , (8000h+8h*16ah)
GLINTREGISTER       PatternRAMData11            , (8000h+8h*16bh)
GLINTREGISTER       PatternRAMData12            , (8000h+8h*16ch)
GLINTREGISTER       PatternRAMData13            , (8000h+8h*16dh)
GLINTREGISTER       PatternRAMData14            , (8000h+8h*16eh)
GLINTREGISTER       PatternRAMData15            , (8000h+8h*16fh)
GLINTREGISTER       PatternRAMData16            , (8000h+8h*170h)
GLINTREGISTER       PatternRAMData17            , (8000h+8h*171h)
GLINTREGISTER       PatternRAMData18            , (8000h+8h*172h)
GLINTREGISTER       PatternRAMData19            , (8000h+8h*173h)
GLINTREGISTER       PatternRAMData20            , (8000h+8h*174h)
GLINTREGISTER       PatternRAMData21            , (8000h+8h*175h)
GLINTREGISTER       PatternRAMData22            , (8000h+8h*176h)
GLINTREGISTER       PatternRAMData23            , (8000h+8h*177h)
GLINTREGISTER       PatternRAMData24            , (8000h+8h*178h)
GLINTREGISTER       PatternRAMData25            , (8000h+8h*179h)
GLINTREGISTER       PatternRAMData26            , (8000h+8h*17ah)
GLINTREGISTER       PatternRAMData27            , (8000h+8h*17bh)
GLINTREGISTER       PatternRAMData28            , (8000h+8h*17ch)
GLINTREGISTER       PatternRAMData29            , (8000h+8h*17dh)
GLINTREGISTER       PatternRAMData30            , (8000h+8h*17eh)
GLINTREGISTER       PatternRAMData31            , (8000h+8h*17fh)
GLINTREGISTER       FilterMode                  , (8000h+8h*180h)
GLINTREGISTER       StatisticMode               , (8000h+8h*181h)
GLINTREGISTER       MinRegion                   , (8000h+8h*182h)
GLINTREGISTER       MaxRegion                   , (8000h+8h*183h)
GLINTREGISTER       ResetPickResult             , (8000h+8h*184h)
GLINTREGISTER       MinHitRegion                , (8000h+8h*185h)
GLINTREGISTER       MaxHitRegion                , (8000h+8h*186h)
GLINTREGISTER       PickResult                  , (8000h+8h*187h)
GLINTREGISTER       Sync                        , (8000h+8h*188h)
GLINTREGISTER       Blank189                    , (8000h+8h*189h)
GLINTREGISTER       Blank18A                    , (8000h+8h*18Ah)
GLINTREGISTER       Blank18B                    , (8000h+8h*18Bh)
GLINTREGISTER       Blank18C                    , (8000h+8h*18Ch)
GLINTREGISTER       FBBlockColorUpper           , (8000h+8h*18dh)
GLINTREGISTER       FBBlockColorLower           , (8000h+8h*18eh)
GLINTREGISTER       SuspendUntilFrameBlank      , (8000h+8h*18fh)

GLINTREGISTER       Blank190                    , (8000h+8h*190h)
GLINTREGISTER       Blank191                    , (8000h+8h*191h)
GLINTREGISTER       Blank192                    , (8000h+8h*192h)
GLINTREGISTER       Blank193                    , (8000h+8h*193h)
GLINTREGISTER       Blank194                    , (8000h+8h*194h)
GLINTREGISTER       Blank195                    , (8000h+8h*195h)
GLINTREGISTER       Blank196                    , (8000h+8h*196h)
GLINTREGISTER       Blank197                    , (8000h+8h*197h)
GLINTREGISTER       Blank198                    , (8000h+8h*198h)
GLINTREGISTER       Blank199                    , (8000h+8h*199h)
GLINTREGISTER       Blank19A                    , (8000h+8h*19Ah)
GLINTREGISTER       Blank19B                    , (8000h+8h*19Bh)
GLINTREGISTER       Blank19C                    , (8000h+8h*19Ch)
GLINTREGISTER       Blank19D                    , (8000h+8h*19Dh)
GLINTREGISTER       Blank19E                    , (8000h+8h*19Eh)
GLINTREGISTER       Blank19F                    , (8000h+8h*19Fh) 

GLINTREGISTER       Blank1a0                    , (8000h+8h*1a0h)
GLINTREGISTER       Blank1a1                    , (8000h+8h*1a1h)
GLINTREGISTER       Blank1a2                    , (8000h+8h*1a2h)
GLINTREGISTER       Blank1a3                    , (8000h+8h*1a3h)
GLINTREGISTER       Blank1a4                    , (8000h+8h*1a4h)
GLINTREGISTER       Blank1a5                    , (8000h+8h*1a5h)
GLINTREGISTER       Blank1a6                    , (8000h+8h*1a6h)
GLINTREGISTER       Blank1a7                    , (8000h+8h*1a7h)
GLINTREGISTER       Blank1a8                    , (8000h+8h*1a8h)
GLINTREGISTER       Blank1a9                    , (8000h+8h*1a9h)
GLINTREGISTER       Blank1aA                    , (8000h+8h*1aAh)
GLINTREGISTER       Blank1aB                    , (8000h+8h*1aBh)
GLINTREGISTER       Blank1aC                    , (8000h+8h*1aCh)
GLINTREGISTER       Blank1aD                    , (8000h+8h*1aDh)
GLINTREGISTER       Blank1aE                    , (8000h+8h*1aEh)
GLINTREGISTER       Blank1aF                    , (8000h+8h*1aFh) 

GLINTREGISTER       Blank1b0                    , (8000h+8h*1b0h)
GLINTREGISTER       Blank1b1                    , (8000h+8h*1b1h)
GLINTREGISTER       Blank1b2                    , (8000h+8h*1b2h)
GLINTREGISTER       Blank1b3                    , (8000h+8h*1b3h)
GLINTREGISTER       Blank1b4                    , (8000h+8h*1b4h)
GLINTREGISTER       Blank1b5                    , (8000h+8h*1b5h)
GLINTREGISTER       Blank1b6                    , (8000h+8h*1b6h)
GLINTREGISTER       Blank1b7                    , (8000h+8h*1b7h)
GLINTREGISTER       Blank1b8                    , (8000h+8h*1b8h)
GLINTREGISTER       Blank1b9                    , (8000h+8h*1b9h)
GLINTREGISTER       Blank1bA                    , (8000h+8h*1bAh)
GLINTREGISTER       Blank1bB                    , (8000h+8h*1bBh)
GLINTREGISTER       Blank1bC                    , (8000h+8h*1bCh)
GLINTREGISTER       Blank1bD                    , (8000h+8h*1bDh)
GLINTREGISTER       Blank1bE                    , (8000h+8h*1bEh)
GLINTREGISTER       Blank1bF                    , (8000h+8h*1bFh) 

GLINTREGISTER       Blank1c0                    , (8000h+8h*1c0h)
GLINTREGISTER       Blank1c1                    , (8000h+8h*1c1h)
GLINTREGISTER       Blank1c2                    , (8000h+8h*1c2h)
GLINTREGISTER       Blank1c3                    , (8000h+8h*1c3h)
GLINTREGISTER       Blank1c4                    , (8000h+8h*1c4h)
GLINTREGISTER       Blank1c5                    , (8000h+8h*1c5h)
GLINTREGISTER       Blank1c6                    , (8000h+8h*1c6h)
GLINTREGISTER       Blank1c7                    , (8000h+8h*1c7h)
GLINTREGISTER       Blank1c8                    , (8000h+8h*1c8h)
GLINTREGISTER       Blank1c9                    , (8000h+8h*1c9h)
GLINTREGISTER       Blank1cA                    , (8000h+8h*1cAh)
GLINTREGISTER       Blank1cB                    , (8000h+8h*1cBh)
GLINTREGISTER       Blank1cC                    , (8000h+8h*1cCh)
GLINTREGISTER       Blank1cD                    , (8000h+8h*1cDh)
GLINTREGISTER       Blank1cE                    , (8000h+8h*1cEh)
GLINTREGISTER       Blank1cF                    , (8000h+8h*1cFh) 

GLINTREGISTER       TexelLUT0                   , (8000h+8h*1d0h)
GLINTREGISTER       TexelLUT1                   , (8000h+8h*1d1h)
GLINTREGISTER       TexelLUT2                   , (8000h+8h*1d2h)
GLINTREGISTER       TexelLUT3                   , (8000h+8h*1d3h)
GLINTREGISTER       TexelLUT4                   , (8000h+8h*1d4h)
GLINTREGISTER       TexelLUT5                   , (8000h+8h*1d5h)
GLINTREGISTER       TexelLUT6                   , (8000h+8h*1d6h)
GLINTREGISTER       TexelLUT7                   , (8000h+8h*1d7h)
GLINTREGISTER       TexelLUT8                   , (8000h+8h*1d8h)
GLINTREGISTER       TexelLUT9                   , (8000h+8h*1d9h)
GLINTREGISTER       TexelLUT10                  , (8000h+8h*1dah)
GLINTREGISTER       TexelLUT11                  , (8000h+8h*1dbh)
GLINTREGISTER       TexelLUT12                  , (8000h+8h*1dch)
GLINTREGISTER       TexelLUT13                  , (8000h+8h*1ddh)
GLINTREGISTER       TexelLUT14                  , (8000h+8h*1deh)
GLINTREGISTER       TexelLUT15                  , (8000h+8h*1dfh)

GLINTREGISTER       YUVMode                     , (8000h+8h*1e0h)
GLINTREGISTER       ChromaUpperBound            , (8000h+8h*1e1h)
GLINTREGISTER       ChromaLowerBound            , (8000h+8h*1e2h)
GLINTREGISTER       Blank1e3                    , (8000h+8h*1e3h)
GLINTREGISTER       Blank1e4                    , (8000h+8h*1e4h)
GLINTREGISTER       Blank1e5                    , (8000h+8h*1e5h)
GLINTREGISTER       Blank1e6                    , (8000h+8h*1e6h)
GLINTREGISTER       Blank1e7                    , (8000h+8h*1e7h)
GLINTREGISTER       Blank1e8                    , (8000h+8h*1e8h)
GLINTREGISTER       Blank1e9                    , (8000h+8h*1e9h)
GLINTREGISTER       Blank1eA                    , (8000h+8h*1eAh)
GLINTREGISTER       Blank1eB                    , (8000h+8h*1eBh)
GLINTREGISTER       Blank1eC                    , (8000h+8h*1eCh)
GLINTREGISTER       Blank1eD                    , (8000h+8h*1eDh)
GLINTREGISTER       Blank1eE                    , (8000h+8h*1eEh)
GLINTREGISTER       Blank1eF                    , (8000h+8h*1eFh) 

GLINTREGISTER       Blank1f0                    , (8000h+8h*1f0h)
GLINTREGISTER       Blank1f1                    , (8000h+8h*1f1h)
GLINTREGISTER       Blank1f2                    , (8000h+8h*1f2h)
GLINTREGISTER       Blank1f3                    , (8000h+8h*1f3h)
GLINTREGISTER       Blank1f4                    , (8000h+8h*1f4h)
GLINTREGISTER       Blank1f5                    , (8000h+8h*1f5h)
GLINTREGISTER       Blank1f6                    , (8000h+8h*1f6h)
GLINTREGISTER       Blank1f7                    , (8000h+8h*1f7h)
GLINTREGISTER       Blank1f8                    , (8000h+8h*1f8h)
GLINTREGISTER       Blank1f9                    , (8000h+8h*1f9h)
GLINTREGISTER       Blank1fA                    , (8000h+8h*1fAh)
GLINTREGISTER       Blank1fB                    , (8000h+8h*1fBh)
GLINTREGISTER       Blank1fC                    , (8000h+8h*1fCh)
GLINTREGISTER       Blank1fD                    , (8000h+8h*1fDh)
GLINTREGISTER       Blank1fE                    , (8000h+8h*1fEh)
GLINTREGISTER       Blank1fF                    , (8000h+8h*1fFh) 

GLINTREGISTER       V0Fixed0                    , (8000h+8h*200h)
GLINTREGISTER       V0Fixed1                    , (8000h+8h*201h)
GLINTREGISTER       V0Fixed2                    , (8000h+8h*202h)
GLINTREGISTER       V0Fixed3                    , (8000h+8h*203h)
GLINTREGISTER       V0Fixed4                    , (8000h+8h*204h)
GLINTREGISTER       V0Fixed5                    , (8000h+8h*205h)
GLINTREGISTER       V0Fixed6                    , (8000h+8h*206h)
GLINTREGISTER       V0Fixed7                    , (8000h+8h*207h)
GLINTREGISTER       V0Fixed8                    , (8000h+8h*208h)
GLINTREGISTER       V0Fixed9                    , (8000h+8h*209h)
GLINTREGISTER       V0FixedA                    , (8000h+8h*20Ah)
GLINTREGISTER       V0FixedB                    , (8000h+8h*20Bh)
GLINTREGISTER       V0FixedC                    , (8000h+8h*20Ch)
GLINTREGISTER       Blank20D                    , (8000h+8h*20Dh)
GLINTREGISTER       Blank20E                    , (8000h+8h*20Eh)
GLINTREGISTER       Blank20F                    , (8000h+8h*20Fh) 

GLINTREGISTER       V1Fixed0                    , (8000h+8h*210h)
GLINTREGISTER       V1Fixed1                    , (8000h+8h*211h)
GLINTREGISTER       V1Fixed2                    , (8000h+8h*212h)
GLINTREGISTER       V1Fixed3                    , (8000h+8h*213h)
GLINTREGISTER       V1Fixed4                    , (8000h+8h*214h)
GLINTREGISTER       V1Fixed5                    , (8000h+8h*215h)
GLINTREGISTER       V1Fixed6                    , (8000h+8h*216h)
GLINTREGISTER       V1Fixed7                    , (8000h+8h*217h)
GLINTREGISTER       V1Fixed8                    , (8000h+8h*218h)
GLINTREGISTER       V1Fixed9                    , (8000h+8h*219h)
GLINTREGISTER       V1FixedA                    , (8000h+8h*21Ah)
GLINTREGISTER       V1FixedB                    , (8000h+8h*21Bh)
GLINTREGISTER       V1FixedC                    , (8000h+8h*21Ch)
GLINTREGISTER       Blank21D                    , (8000h+8h*21Dh)
GLINTREGISTER       Blank21E                    , (8000h+8h*21Eh)
GLINTREGISTER       Blank21F                    , (8000h+8h*21Fh) 

GLINTREGISTER       V2Fixed0                    , (8000h+8h*220h)
GLINTREGISTER       V2Fixed1                    , (8000h+8h*221h)
GLINTREGISTER       V2Fixed2                    , (8000h+8h*222h)
GLINTREGISTER       V2Fixed3                    , (8000h+8h*223h)
GLINTREGISTER       V2Fixed4                    , (8000h+8h*224h)
GLINTREGISTER       V2Fixed5                    , (8000h+8h*225h)
GLINTREGISTER       V2Fixed6                    , (8000h+8h*226h)
GLINTREGISTER       V2Fixed7                    , (8000h+8h*227h)
GLINTREGISTER       V2Fixed8                    , (8000h+8h*228h)
GLINTREGISTER       V2Fixed9                    , (8000h+8h*229h)
GLINTREGISTER       V2FixedA                    , (8000h+8h*22Ah)
GLINTREGISTER       V2FixedB                    , (8000h+8h*22Bh)
GLINTREGISTER       V2FixedC                    , (8000h+8h*22Ch)
GLINTREGISTER       Blank22D                    , (8000h+8h*22Dh)
GLINTREGISTER       Blank22E                    , (8000h+8h*22Eh)
GLINTREGISTER       Blank22F                    , (8000h+8h*22Fh) 

GLINTREGISTER       V0Float0                    , (8000h+8h*230h)
GLINTREGISTER       V0Float1                    , (8000h+8h*231h)
GLINTREGISTER       V0Float2                    , (8000h+8h*232h)
GLINTREGISTER       V0Float3                    , (8000h+8h*233h)
GLINTREGISTER       V0Float4                    , (8000h+8h*234h)
GLINTREGISTER       V0Float5                    , (8000h+8h*235h)
GLINTREGISTER       V0Float6                    , (8000h+8h*236h)
GLINTREGISTER       V0Float7                    , (8000h+8h*237h)
GLINTREGISTER       V0Float8                    , (8000h+8h*238h)
GLINTREGISTER       V0Float9                    , (8000h+8h*239h)
GLINTREGISTER       V0FloatA                    , (8000h+8h*23Ah)
GLINTREGISTER       V0FloatB                    , (8000h+8h*23Bh)
GLINTREGISTER       V0FloatC                    , (8000h+8h*23Ch)
GLINTREGISTER       Blank23D                    , (8000h+8h*23Dh)
GLINTREGISTER       Blank23E                    , (8000h+8h*23Eh)
GLINTREGISTER       Blank23F                    , (8000h+8h*23Fh) 

GLINTREGISTER       V1Float0                    , (8000h+8h*240h)
GLINTREGISTER       V1Float1                    , (8000h+8h*241h)
GLINTREGISTER       V1Float2                    , (8000h+8h*242h)
GLINTREGISTER       V1Float3                    , (8000h+8h*243h)
GLINTREGISTER       V1Float4                    , (8000h+8h*244h)
GLINTREGISTER       V1Float5                    , (8000h+8h*245h)
GLINTREGISTER       V1Float6                    , (8000h+8h*246h)
GLINTREGISTER       V1Float7                    , (8000h+8h*247h)
GLINTREGISTER       V1Float8                    , (8000h+8h*248h)
GLINTREGISTER       V1Float9                    , (8000h+8h*249h)
GLINTREGISTER       V1FloatA                    , (8000h+8h*24Ah)
GLINTREGISTER       V1FloatB                    , (8000h+8h*24Bh)
GLINTREGISTER       V1FloatC                    , (8000h+8h*24Ch)
GLINTREGISTER       Blank24D                    , (8000h+8h*24Dh)
GLINTREGISTER       Blank24E                    , (8000h+8h*24Eh)
GLINTREGISTER       Blank24F                    , (8000h+8h*24Fh) 

GLINTREGISTER       V2Float0                    , (8000h+8h*250h)
GLINTREGISTER       V2Float1                    , (8000h+8h*251h)
GLINTREGISTER       V2Float2                    , (8000h+8h*252h)
GLINTREGISTER       V2Float3                    , (8000h+8h*253h)
GLINTREGISTER       V2Float4                    , (8000h+8h*254h)
GLINTREGISTER       V2Float5                    , (8000h+8h*255h)
GLINTREGISTER       V2Float6                    , (8000h+8h*256h)
GLINTREGISTER       V2Float7                    , (8000h+8h*257h)
GLINTREGISTER       V2Float8                    , (8000h+8h*258h)
GLINTREGISTER       V2Float9                    , (8000h+8h*259h)
GLINTREGISTER       V2FloatA                    , (8000h+8h*25Ah)
GLINTREGISTER       V2FloatB                    , (8000h+8h*25Bh)
GLINTREGISTER       V2FloatC                    , (8000h+8h*25Ch)
GLINTREGISTER       Blank25D                    , (8000h+8h*25Dh)
GLINTREGISTER       Blank25E                    , (8000h+8h*25Eh)
GLINTREGISTER       Blank25F                    , (8000h+8h*25Fh) 

GLINTREGISTER       DeltaMode                   , (8000h+8h*260h)
GLINTREGISTER       DrawTriangle                , (8000h+8h*261h)
GLINTREGISTER       RepeatTriangle              , (8000h+8h*262h)
GLINTREGISTER       DrawLine01                  , (8000h+8h*263h)
GLINTREGISTER       DrawLine10                  , (8000h+8h*264h)
GLINTREGISTER       RepeatLine                  , (8000h+8h*265h)
GLINTREGISTER       Blank266                    , (8000h+8h*266h)
GLINTREGISTER       Blank267                    , (8000h+8h*267h)
GLINTREGISTER       Blank268                    , (8000h+8h*268h)
GLINTREGISTER       Blank269                    , (8000h+8h*269h)
GLINTREGISTER       Blank26A                    , (8000h+8h*26Ah)
GLINTREGISTER       Blank26B                    , (8000h+8h*26Bh)
GLINTREGISTER       Blank26C                    , (8000h+8h*26Ch)
GLINTREGISTER       Blank26D                    , (8000h+8h*26Dh)
GLINTREGISTER       Blank26E                    , (8000h+8h*26Eh)
GLINTREGISTER       BroadcastMask               , (8000h+8h*26Fh)

GLINTREGTAIL






















