/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: glintreg.h
 *
 *              GLINT register window definition
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


/*
 * GLINT register window definition
 */

#define DWFILL unsigned long :32
#define WFILL unsigned short :16

typedef unsigned long DWORD;

typedef struct GlintReg {
    // 0h 
    volatile DWORD      ResetStatus                 ; DWFILL;	// 0x0000
    volatile DWORD      IntEnable                   ; DWFILL;	// 0x0008
    volatile DWORD      IntFlags                    ; DWFILL;   // 0x0010
    volatile DWORD      InFIFOSpace                 ; DWFILL;   // 0x0018
    volatile DWORD      OutFIFOWords                ; DWFILL;   // 0x0020
    volatile DWORD      DMAAddress                  ; DWFILL;   // 0x0028
    volatile DWORD      DMACount                    ; DWFILL;   // 0x0030
    volatile DWORD      ErrorFlags                  ; DWFILL;   // 0x0038
    volatile DWORD      VClkCtl                     ; DWFILL;   // 0x0040
    volatile DWORD      TestRegister                ; DWFILL;   // 0x0048
    volatile DWORD      ApertureOne                 ; DWFILL;	// 0x0050 Permedia control register
    volatile DWORD      ApertureTwo                 ; DWFILL;	// 0x0058 Permedia control register
    volatile DWORD      DMAControl                  ; DWFILL;	// 0x0060
    volatile DWORD      FIFODiscon                  ; DWFILL;   // 0x0068 SX Rev 2, TX and Permedia
    volatile DWORD      ChipConfig                  ; DWFILL;	// 0x0070 Permedia
    DWFILL; 
    DWFILL;							// 0x0078 Not currently used
    volatile DWORD      OutDMAAddress		    ; DWFILL;	// 0x0080 Output DMA controller address
    volatile DWORD      OutDMACount		    ; DWFILL;	// 0x0088 Output DMA controller count
    volatile DWORD      AGPTexBaseAddress	    ; DWFILL;	// 0x0090 AGP Texture base address

    // Delta Registers       
    volatile DWORD      Fill0[0x800/4 - 19*2];
    volatile DWORD      DeltaReset                  ; DWFILL;
    volatile DWORD      DeltaIntEnable              ; DWFILL;
    volatile DWORD      DeltaIntFlags               ; DWFILL;
    volatile DWORD      DeltaInFIFOSpace            ; DWFILL;
    volatile DWORD      DeltaOutFIFOWords           ; DWFILL;
    volatile DWORD      DeltaDMAAddress             ; DWFILL;
    volatile DWORD      DeltaDMACount               ; DWFILL;
    volatile DWORD      DeltaErrorFlags             ; DWFILL;
    volatile DWORD      DeltaVClkCtl                ; DWFILL;
    volatile DWORD      DeltaTestRegister           ; DWFILL;
    volatile DWORD      DeltaAperture0              ; DWFILL;
    volatile DWORD      DeltaAperture1              ; DWFILL;
    volatile DWORD      DeltaDMAControl             ; DWFILL;
    volatile DWORD      DeltaDisconnectControl      ; DWFILL;
   
    // Localbuffer Registers 
    volatile DWORD      Fill1[0x1000/4 - 0x870/4];

    // 1000h 
    volatile DWORD      Reboot		            ; DWFILL;   // Permedia SGRAM control register
    volatile DWORD      LBMemoryEDO                 ; DWFILL;	// 0x1008
    DWORD               dwFill10[12];				// 0x1010 - 0x1038
    volatile DWORD      MemControl                  ; DWFILL;	// 0x1040 Permedia
    DWORD               dwFill11[14];			        // 0x1048 - 0x1078
    volatile DWORD      BootAddress                 ; DWFILL;   // 0x1080 Permedia
    DWORD               dwFill12[14];			        // 0x1088 - 0x10b8
    volatile DWORD      MemConfig                   ; DWFILL;	// 0x10c0 Permedia
    DWORD               dwFill13[14];			        // 0x10c8 - 0x10f8
    volatile DWORD      BypassWriteMask             ; DWFILL;	// 0x1100 Permedia
    DWORD               dwFill14[14];				// 0x1108 - 0x1138
    volatile DWORD      FramebufferWriteMask        ; DWFILL;	// 0x1140 Permedia
    DWORD               dwFill15[14];				// 0x1148 - 0x1178
    volatile DWORD      MClkCount	            ; DWFILL;   // 0x1180 Permedia

    // Framebuffer Registers 
    volatile DWORD      Fill2[0x1800/4 - 0x1188/4];
	
    // 1800h 
    volatile DWORD      FBMemoryCtl                 ; DWFILL;
    volatile DWORD      FBModeSel                   ; DWFILL;
    volatile DWORD      FBGPWrMask                  ; DWFILL;
    volatile DWORD      FBGPColorMask               ; DWFILL;

    // GP Fifo Interface    
    volatile DWORD      Fill3[0x2000/4 - 0x1820/4];

    // 2000h 
    volatile DWORD      GPFifo[1024];

    // Internal Video Registers
    // 3000h 
    volatile DWORD      ScreenBase                  ; DWFILL;   // 0x3000 Permedia VTG Register
    volatile DWORD      ScreenStride                ; DWFILL;	// 0x3008 Permedia VTG Register
    volatile DWORD      HTotal		            ; DWFILL;	// 0x3010 Permedia VTG Register
    volatile DWORD      HgEnd		            ; DWFILL;	// 0x3018 Permedia VTG Register
    volatile DWORD      HbEnd		            ; DWFILL;	// 0x3020 Permedia VTG Register
    volatile DWORD      HsStart	                    ; DWFILL;	// 0x3028 Permedia VTG Register
    volatile DWORD      HsEnd		            ; DWFILL;   // 0x3030 Permedia VTG Register
    volatile DWORD      VTotal		            ; DWFILL;	// 0x3038 Permedia VTG Register
    volatile DWORD      VbEnd		            ; DWFILL;   // 0x3040 Permedia VTG Register
    volatile DWORD      VsStart	                    ; DWFILL;   // 0x3048 Permedia VTG Register
    volatile DWORD      VsEnd		            ; DWFILL;	// 0x3050 Permedia VTG Register
    volatile DWORD      VideoControl                ; DWFILL;   // 0x3058 Permedia VTG Register
    volatile DWORD      InterruptLine               ; DWFILL;   // 0x3060 Permedia VTG Register
    volatile DWORD      DisplayData		    ; DWFILL;   // 0x3068 P2 VTG Register
    volatile DWORD      LineCount	            ; DWFILL;   // 0x3070 Permedia VTG Register
    volatile DWORD      FifoControl                 ; DWFILL;   // 0x3078 P2 VTG Register
    volatile DWORD      ScreenBaseRight             ; DWFILL; 	// 0x3080 P2 VTG Register
    
    // External Video Control 
    volatile DWORD      Fill4[0x4000/4 - 0x3088/4]  ;
	
    // 4000h
    volatile DWORD      ExtVCReg                    ; DWFILL;   // Dac registers
    volatile DWORD      Fill5[0x5000/4 - 0x4008/4]  ;
    volatile DWORD      P2ExtVCReg	            ; DWFILL;   // 0x5000 P2 External Dac
    volatile DWORD      Fill5a[0x5800/4 - 0x5008/4] ;

    // 5800h (Video Streams Interface) 
    volatile DWORD      VSConfiguration		    ; DWFILL;	// 0x5800 PM VidStream Register
    volatile DWORD	VSStatus		    ; DWFILL;	// 0x5808 PM VidStream Register
    volatile DWORD	VSSerialBusControl	    ; DWFILL;	// 0x5810 PM I2C Register

    volatile DWORD      Fill6[0x5900/4 - 0x5818/4];

    // Video Stream A Video Data
    volatile DWORD	VSAControl		    ; DWFILL;	// 0x5900 PM VidStream A Register
    volatile DWORD	VSAInterruptLine	    ; DWFILL;	// 0x5908 PM VidStream A Register
    volatile DWORD	VSACurrentLine		    ; DWFILL;	// 0x5910 PM VidStream A Register
    volatile DWORD	VSAVideoAddressHost	    ; DWFILL;	// 0x5918 PM VidStream A Register
    volatile DWORD	VSAVideoAddressIndex	    ; DWFILL;	// 0x5920 PM VidStream A Register
    volatile DWORD	VSAVideoAddress0	    ; DWFILL;	// 0x5928 PM VidStream A Register
    volatile DWORD	VSAVideoAddress1	    ; DWFILL;	// 0x5930 PM VidStream A Register
    volatile DWORD	VSAVideoAddress2	    ; DWFILL;	// 0x5938 PM VidStream A Register
    volatile DWORD	VSAVideoStride		    ; DWFILL;	// 0x5940 PM VidStream A Register
    volatile DWORD	VSAVideoStartLine	    ; DWFILL;	// 0x5948 PM VidStream A Register
    volatile DWORD	VSAVideoEndLine		    ; DWFILL;	// 0x5950 PM VidStream A Register
    volatile DWORD	VSAVideoStartData	    ; DWFILL;	// 0x5958 PM VidStream A Register
    volatile DWORD	VSAVideoEndData		    ; DWFILL;	// 0x5960 PM VidStream A Register

    // Video Stream A VBI Data
    volatile DWORD	VSAVBIAddressHost           ; DWFILL;	// 0x5968 PM VidStream A Register
    volatile DWORD	VSAVBIAddressIndex	    ; DWFILL;	// 0x5970 PM VidStream A Register
    volatile DWORD	VSAVBIAddress0		    ; DWFILL;	// 0x5978 PM VidStream A Register
    volatile DWORD	VSAVBIAddress1		    ; DWFILL;	// 0x5980 PM VidStream A Register
    volatile DWORD	VSAVBIAddress2		    ; DWFILL;	// 0x5988 PM VidStream A Register
    volatile DWORD	VSAVBIStride		    ; DWFILL;	// 0x5990 PM VidStream A Register
    volatile DWORD	VSAVBIStartLine		    ; DWFILL;	// 0x5998 PM VidStream A Register
    volatile DWORD	VSAVBIEndLine		    ; DWFILL;	// 0x59A0 PM VidStream A Register
    volatile DWORD	VSAVBIStartData		    ; DWFILL;	// 0x59A8 PM VidStream A Register
    volatile DWORD	VSAVBIEndData		    ; DWFILL;	// 0x59B0 PM VidStream A Register
    volatile DWORD	VSAFIFOControl		    ; DWFILL;	// 0x59B8 PM VidStream A Register

    volatile DWORD      Fill7[0x5A00/4 - 0x59C0/4];

    // Video Stream B Video Data
    volatile DWORD      VSBControl		    ; DWFILL;	// 0x5A00 PM VidStream B Register
    volatile DWORD	VSBInterruptLine	    ; DWFILL;	// 0x5A08 PM VidStream B Register
    volatile DWORD	VSBCurrentLine		    ; DWFILL;	// 0x5A10 PM VidStream B Register
    volatile DWORD	VSBVideoAddressHost	    ; DWFILL;	// 0x5A18 PM VidStream B Register
    volatile DWORD	VSBVideoAddressIndex	    ; DWFILL;	// 0x5A20 PM VidStream B Register
    volatile DWORD	VSBVideoAddress0	    ; DWFILL;	// 0x5A28 PM VidStream B Register
    volatile DWORD	VSBVideoAddress1	    ; DWFILL;	// 0x5A30 PM VidStream B Register
    volatile DWORD	VSBVideoAddress2	    ; DWFILL;	// 0x5A38 PM VidStream B Register
    volatile DWORD	VSBVideoStride		    ; DWFILL;	// 0x5A40 PM VidStream B Register
    volatile DWORD	VSBVideoStartLine	    ; DWFILL;	// 0x5A48 PM VidStream B Register
    volatile DWORD	VSBVideoEndLine		    ; DWFILL;	// 0x5A50 PM VidStream B Register
    volatile DWORD	VSBVideoStartData	    ; DWFILL;	// 0x5A58 PM VidStream B Register
    volatile DWORD	VSBVideoEndData		    ; DWFILL;	// 0x5A60 PM VidStream B Register
    volatile DWORD	VSBVBIAddressHost	    ; DWFILL;	// 0x5A68 PM VidStream B Register
    volatile DWORD	VSBVBIAddressIndex	    ; DWFILL;	// 0x5A70 PM VidStream B Register
    volatile DWORD	VSBVBIAddress0		    ; DWFILL;	// 0x5A78 PM VidStream B Register
    volatile DWORD	VSBVBIAddress1		    ; DWFILL;	// 0x5A80 PM VidStream B Register
    volatile DWORD	VSBVBIAddress2		    ; DWFILL;	// 0x5A88 PM VidStream B Register
    volatile DWORD	VSBVBIStride		    ; DWFILL;	// 0x5A90 PM VidStream B Register
    volatile DWORD	VSBVBIStartLine		    ; DWFILL;	// 0x5A98 PM VidStream B Register
    volatile DWORD	VSBVBIEndLine		    ; DWFILL;	// 0x5AA0 PM VidStream B Register
    volatile DWORD	VSBVBIStartData		    ; DWFILL;	// 0x5AA8 PM VidStream B Register
    volatile DWORD      VSBVBIEndData		    ; DWFILL;	// 0x5AB0 PM VidStream B Register
    volatile DWORD      VSBFIFOControl		    ; DWFILL;	// 0x5AB8 PM VidStream B Register

    volatile DWORD      Fill8[0x6000/4 - 0x5AC0/4];

    // 6000h 
    volatile DWORD      ExtBrdReg                   ; DWFILL;
    volatile DWORD      VRAMBankSwitch		    ; DWFILL;
    volatile DWORD      Fill9[0x63C0/4 - 0x6010/4]  ; WFILL;    // 0x63c0 
    volatile BYTE       WriteMiscOutputReg	    ;		// 0x63c2
    volatile BYTE       Fill9a			    ;	        // 0x63c3
    volatile WORD       VGAControlReg               ; WFILL;    // 0x63c4
    DWFILL;	                                                // 0x63c8 
    volatile BYTE       ReadMiscOutputReg	    ;	        // 0x63cc 
    volatile BYTE       Fill9b			    ; WFILL;    // 0x63cd 
    volatile DWORD      Fill10[0x7000/4 - 0x63D0/4] ;
    volatile DWORD      RacerProUBufB               ; DWFILL;   // 0x7000 Racer Pro config
    volatile DWORD      Fill10a[0x8000/4 - 0x7008/4];

    // Graphics Processor
    // 8000h
    volatile DWORD      StartXDom                   ; DWFILL;   /*           0 */
    volatile DWORD      dXDom                       ; DWFILL;   /*           1 */
    volatile DWORD      StartXSub                   ; DWFILL;   /*           2 */
    volatile DWORD      dXSub                       ; DWFILL;   /*           3 */
    volatile DWORD      StartY                      ; DWFILL;   /*           4 */
    volatile DWORD      dY                          ; DWFILL;   /*           5 */
    volatile DWORD      Count                       ; DWFILL;   /*           6 */
    volatile DWORD      Render                      ; DWFILL;   /*           7 */
    volatile DWORD      ContinueNewLine             ; DWFILL;   /*           8 */
    volatile DWORD      ContinueNewDom              ; DWFILL;   /*           9 */
    volatile DWORD      ContinueNewSub              ; DWFILL;   /*           A */
    volatile DWORD      Continue                    ; DWFILL;   /*           B */
    volatile DWORD      FlushSpan                   ; DWFILL;   /*           C */
    volatile DWORD      BitMaskPattern              ; DWFILL;   /*           D */
    DWFILL; 
    DWFILL;                                                     /*           E  */
    DWFILL; 
    DWFILL;			                                /*           F */
    
    // 8000h+16*8
    volatile DWORD      PointTable0                 ; DWFILL;   /*           10 */
    volatile DWORD      PointTable1                 ; DWFILL;   /*           11 */
    volatile DWORD      PointTable2                 ; DWFILL;   /*           12 */
    volatile DWORD      PointTable3                 ; DWFILL;   /*           13 */
    volatile DWORD      RasterizerMode              ; DWFILL;   /*           14 */
    volatile DWORD      YLimits		            ; DWFILL;   /*           15 */
    volatile DWORD      ScanlineOwnership           ; DWFILL;   /*           16 */
    volatile DWORD      WaitForCompletion	    ; DWFILL;   /*           17 */
    volatile DWORD      PixelSize		    ; DWFILL;   /*           18 */
    volatile DWORD      XLimits		            ; DWFILL;   /*           19 */
    volatile DWORD      RectangleOrigin             ; DWFILL;   /*           1a */
    volatile DWORD      RectangleSize	            ; DWFILL;   /*           1b */
    volatile DWORD      Fill11[4*2];    		        /*        1c-1f */
    
    // 8000h+32*8
    volatile DWORD      CoverageValue               ; DWFILL;   /*           20 */
    volatile DWORD      PrepareToRender             ; DWFILL;   /*           21 */
    volatile DWORD      ActiveStepX                 ; DWFILL;   /*           22 */
    volatile DWORD      PassiveStepX                ; DWFILL;   /*           23 */
    volatile DWORD      ActiveStepYDomEdge          ; DWFILL;   /*           24 */
    volatile DWORD      PassiveStepYDomEdge         ; DWFILL;   /*           25 */
    volatile DWORD      FastBlockLimits             ; DWFILL;   /*           26 */
    volatile DWORD      FastBlockFill               ; DWFILL;   /*           27 */
    volatile DWORD      SubPixelCorrection          ; DWFILL;   /*           28 */
    volatile DWORD      ForceBackgroundColor        ; DWFILL;   /*           29 */
    volatile DWORD      PackedDataLimits            ; DWFILL;   /*           2a */
    volatile DWORD      SpanStepX		    ; DWFILL;   /*           2b */
    volatile DWORD      SpanStepYDomEdge            ; DWFILL;   /*           2c */
    volatile DWORD      SpanMask	            ; DWFILL;   /*           2d */
    volatile DWORD      SuspendReads                ; DWFILL;   /*           2e */
    volatile DWORD      Fill12[1*2];    	                /*           2f */
    
    // 8000h+48*8
    volatile DWORD      ScissorMode                 ; DWFILL;   /*           30 */
    volatile DWORD      ScissorMinXY                ; DWFILL;   /*           31 */
    volatile DWORD      ScissorMaxXY                ; DWFILL;   /*           32 */
    volatile DWORD      ScreenSize                  ; DWFILL;   /*           33 */
    volatile DWORD      AreaStippleMode             ; DWFILL;   /*           34 */
    volatile DWORD      LineStippleMode             ; DWFILL;   /*           35 */
    volatile DWORD      LoadLineStippleCounters     ; DWFILL;   /*           36 */
    volatile DWORD      UpdateLineStippleCounters   ; DWFILL;   /*           37 */
    volatile DWORD      SaveLineStippleCounters     ; DWFILL;   /*           38 */
    volatile DWORD      WindowOrigin                ; DWFILL;   /*           39 */
    volatile DWORD      Fill13[6*2];    		        /*        3a-3f */
    
    // 8000h+64*8
    volatile DWORD      AreaStipplePattern0         ; DWFILL;   /*           40 */
    volatile DWORD      AreaStipplePattern1         ; DWFILL;   /*           41 */
    volatile DWORD      AreaStipplePattern2         ; DWFILL;   /*           42 */
    volatile DWORD      AreaStipplePattern3         ; DWFILL;   /*           43 */
    volatile DWORD      AreaStipplePattern4         ; DWFILL;   /*           44 */
    volatile DWORD      AreaStipplePattern5         ; DWFILL;   /*           45 */
    volatile DWORD      AreaStipplePattern6         ; DWFILL;   /*           46 */
    volatile DWORD      AreaStipplePattern7         ; DWFILL;   /*           47 */
    volatile DWORD      AreaStipplePattern8         ; DWFILL;   /*           48 */
    volatile DWORD      AreaStipplePattern9         ; DWFILL;   /*           49 */
    volatile DWORD      AreaStipplePattern10        ; DWFILL;   /*           4a */
    volatile DWORD      AreaStipplePattern11        ; DWFILL;   /*           4b */
    volatile DWORD      AreaStipplePattern12        ; DWFILL;   /*           4c */
    volatile DWORD      AreaStipplePattern13        ; DWFILL;   /*           4d */
    volatile DWORD      AreaStipplePattern14        ; DWFILL;   /*           4e */
    volatile DWORD      AreaStipplePattern15        ; DWFILL;   /*           4f */
    volatile DWORD      AreaStipplePattern16        ; DWFILL;   /*           50 */
    volatile DWORD      AreaStipplePattern17        ; DWFILL;   /*           51 */
    volatile DWORD      AreaStipplePattern18        ; DWFILL;   /*           52 */
    volatile DWORD      AreaStipplePattern19        ; DWFILL;   /*           53 */
    volatile DWORD      AreaStipplePattern20        ; DWFILL;   /*           54 */
    volatile DWORD      AreaStipplePattern21        ; DWFILL;   /*           55 */
    volatile DWORD      AreaStipplePattern22        ; DWFILL;   /*           56 */
    volatile DWORD      AreaStipplePattern23        ; DWFILL;   /*           57 */
    volatile DWORD      AreaStipplePattern24        ; DWFILL;   /*           58 */
    volatile DWORD      AreaStipplePattern25        ; DWFILL;   /*           59 */
    volatile DWORD      AreaStipplePattern26        ; DWFILL;   /*           5a */
    volatile DWORD      AreaStipplePattern27        ; DWFILL;   /*           5b */
    volatile DWORD      AreaStipplePattern28        ; DWFILL;   /*           5c */
    volatile DWORD      AreaStipplePattern29        ; DWFILL;   /*           5d */
    volatile DWORD      AreaStipplePattern30        ; DWFILL;   /*           5e */
    volatile DWORD      AreaStipplePattern31        ; DWFILL;   /*           5f */

    volatile DWORD      Fill14[16*2];                           /*        60-6f */
    
    volatile DWORD      TextureAddressMode          ; DWFILL;   /*           70 */
    volatile DWORD      SStart			    ; DWFILL;   /*           71 */
    volatile DWORD      dSdx			    ; DWFILL;   /*           72 */
    volatile DWORD      dSdyDom		            ; DWFILL;   /*           73 */
    volatile DWORD      TStart			    ; DWFILL;   /*           74 */
    volatile DWORD      dTdx			    ; DWFILL;   /*           75 */
    volatile DWORD      dTdyDom		            ; DWFILL;   /*           76 */
    volatile DWORD      QStart			    ; DWFILL;   /*           77 */
    volatile DWORD      dQdx			    ; DWFILL;   /*           78 */
    volatile DWORD      dQdyDom		            ; DWFILL;   /*           79 */
    volatile DWORD      LOD			    ; DWFILL;   /*		7a */
    volatile DWORD      dSdy                        ; DWFILL;   /*		7b */
    volatile DWORD      dTdy                        ; DWFILL;   /*	        7c */
    volatile DWORD      dQdy                        ; DWFILL;   /*		7d */

    volatile DWORD      Fill15[2*2];			        /*        7e-7f */

    volatile DWORD      TextureAddress              ; DWFILL;   /*           80 */
    volatile DWORD      TexelCoordUV                ; DWFILL;   /*           81 */
    volatile DWORD      TexelCoordU                 ; DWFILL;   /*           82 */
    volatile DWORD      TexelCoordV                 ; DWFILL;   /*           83 */
    volatile DWORD      Fill16[12*2];		                /*        84-8f */

    volatile DWORD      TxTextureReadMode           ; DWFILL;   /*           90 */
    volatile DWORD      TextureFormat               ; DWFILL;   /*           91 */
    volatile DWORD      TextureCacheControl         ; DWFILL;   /*           92 */
    volatile DWORD      TexelData0                  ; DWFILL;   /*           93 */
    volatile DWORD      TexelData1                  ; DWFILL;   /*           94 */
    volatile DWORD      BorderColor                 ; DWFILL;   /*           95 */
    volatile DWORD      LUTData                     ; DWFILL;   /*		96 */
    volatile DWORD      LUTDataDirect               ; DWFILL;   /*		97 */
    volatile DWORD      TexelLUTIndex         	    ; DWFILL;   /*		98 */
    volatile DWORD      TexelLUTData          	    ; DWFILL;   /*		99 */
    volatile DWORD      TexelLUTAddress       	    ; DWFILL;   /*		9a */
    volatile DWORD      TexelLUTTransfer      	    ; DWFILL;   /*		9b */
    volatile DWORD      TextureFilterMode	    ; DWFILL;   /*		9c */
    volatile DWORD      TextureChromaUpper	    ; DWFILL;   /*		9d */
    volatile DWORD      TextureChromaLower	    ; DWFILL;   /*		9e */

    DWFILL; 
    DWFILL;					                /*           9f */

    volatile DWORD      TxBaseAddr0		    ; DWFILL;   /*           a0 */
    volatile DWORD      TxBaseAddr1		    ; DWFILL;   /*           a1 */
    volatile DWORD      TxBaseAddr2		    ; DWFILL;   /*           a2 */
    volatile DWORD      TxBaseAddr3		    ; DWFILL;   /*           a3 */
    volatile DWORD      TxBaseAddr4		    ; DWFILL;   /*           a4 */
    volatile DWORD      TxBaseAddr5		    ; DWFILL;   /*           a5 */
    volatile DWORD      TxBaseAddr6		    ; DWFILL;   /*           a6 */
    volatile DWORD      TxBaseAddr7		    ; DWFILL;   /*           a7 */
    volatile DWORD      TxBaseAddr8		    ; DWFILL;   /*           a8 */
    volatile DWORD      TxBaseAddr9		    ; DWFILL;   /*           a9 */
    volatile DWORD      TxBaseAddr10		    ; DWFILL;   /*           aa */
    volatile DWORD      TxBaseAddr11		    ; DWFILL;   /*           ab */

    volatile DWORD      Fill17[4*2];		                /*        ac-af */

    volatile DWORD      TextureBaseAddress          ; DWFILL;   /*           b0 Permedia */
    volatile DWORD      TextureMapFormat            ; DWFILL;   /*           b1 Permedia */
    volatile DWORD      TextureDataFormat           ; DWFILL;   /*           b2 Permedia */
    volatile DWORD      Fill18[2*2];		                /*        b3-b4 Permedia */
    volatile DWORD      TextureReadPad              ; DWFILL;   /*           b5 Permedia */
    volatile DWORD      Fill19[10*2];			        /*        b6-bf Permedia */
    
    // 8000+192*8
    volatile DWORD      Texel0                      ; DWFILL;   /*           c0 */
    volatile DWORD      Texel1                      ; DWFILL;   /*           c1 */
    volatile DWORD      Texel2                      ; DWFILL;   /*           c2 */
    volatile DWORD      Texel3                      ; DWFILL;   /*           c3 */
    volatile DWORD      Texel4                      ; DWFILL;   /*           c4 */
    volatile DWORD      Texel5                      ; DWFILL;   /*           c5 */
    volatile DWORD      Texel6                      ; DWFILL;   /*           c6 */
    volatile DWORD      Texel7                      ; DWFILL;   /*           c7 */
    volatile DWORD      Interp0                     ; DWFILL;   /*           c8 */
    volatile DWORD      Interp1                     ; DWFILL;   /*           c9 */
    volatile DWORD      Interp2                     ; DWFILL;   /*           ca */
    volatile DWORD      Interp3                     ; DWFILL;   /*           cb */
    volatile DWORD      Interp4                     ; DWFILL;   /*           cc */
    volatile DWORD      TextureFilter               ; DWFILL;   /*           cd */
    volatile DWORD      FxTextureReadMode           ; DWFILL;   /*           ce */
    volatile DWORD      TextureLUTMode              ; DWFILL;   /*           cf */
    
    // 8000h+208*8
    volatile DWORD      TextureColorMode            ; DWFILL;   /*           d0 */
    volatile DWORD      TextureEnvColor             ; DWFILL;   /*           d1 */
    volatile DWORD      FogMode                     ; DWFILL;   /*           d2 */
    volatile DWORD      FogColor                    ; DWFILL;   /*           d3 */
    volatile DWORD      FStart                      ; DWFILL;   /*           d4 */
    volatile DWORD      dFdx                        ; DWFILL;   /*           d5 */
    volatile DWORD      dFdyDom                     ; DWFILL;   /*           d6 */
    volatile DWORD      TextureKd                   ; DWFILL;   /*           d7 */
    volatile DWORD      TextureKs                   ; DWFILL;   /*           d8 */
    volatile DWORD      KsStart                     ; DWFILL;   /*           d9 */
    volatile DWORD      dKsdx                       ; DWFILL;   /*           da */
    volatile DWORD      dKsdyDom                    ; DWFILL;   /*           db */
    volatile DWORD      KdStart                     ; DWFILL;   /*           dc */
    volatile DWORD      dKddx                       ; DWFILL;   /*           dd */
    volatile DWORD      dKddyDom                    ; DWFILL;   /*           de */
    DWFILL; 
    DWFILL;               			                /*           df */

    volatile DWORD      Fill20[16*2];    		        /*        e0-ef */

    // 8000h+240*8
    volatile DWORD      RStart                      ; DWFILL;   /*           f0 */
    volatile DWORD      dRdx                        ; DWFILL;   /*           f1 */
    volatile DWORD      dRdyDom                     ; DWFILL;   /*           f2 */
    volatile DWORD      GStart                      ; DWFILL;   /*           f3 */
    volatile DWORD      dGdx                        ; DWFILL;   /*           f4 */
    volatile DWORD      dGdyDom                     ; DWFILL;   /*           f5 */
    volatile DWORD      BStart                      ; DWFILL;   /*           f6 */
    volatile DWORD      dBdx                        ; DWFILL;   /*           f7 */
    volatile DWORD      dBdyDom                     ; DWFILL;   /*           f8 */
    volatile DWORD      AStart                      ; DWFILL;   /*           f9 */
    volatile DWORD      dAdx                        ; DWFILL;   /*           fa */
    volatile DWORD      dAdyDom                     ; DWFILL;   /*           fb */
    volatile DWORD      ColorDDAMode                ; DWFILL;   /*           fc */
    volatile DWORD      ConstantColor               ; DWFILL;   /*           fd */
    volatile DWORD      Color                       ; DWFILL;   /*           fe */
    DWFILL; 
    DWFILL;					                /*           ff */
    
    //8000h+256*8 
    volatile DWORD      AlphaTestMode               ; DWFILL;   /*           100 */
    volatile DWORD      AntialiasMode               ; DWFILL;   /*           101 */
    volatile DWORD      AlphaBlendMode              ; DWFILL;   /*           102 */
    volatile DWORD      DitherMode                  ; DWFILL;   /*           103 */
    volatile DWORD      FBSoftwareWriteMask         ; DWFILL;   /*           104 */
    volatile DWORD      LogicalOpMode               ; DWFILL;   /*           105 */
    volatile DWORD      FBWriteData                 ; DWFILL;   /*           105 */
    volatile DWORD      FBCancelWrite               ; DWFILL;   /*           107 */
    union 
    {
        volatile DWORD  ActiveColorStepX            ;           /*           108 */
        volatile DWORD  RouterMode                  ;           /*           108 */
    };                                                DWFILL;
    volatile DWORD      ActiveColorStepYDomEdge     ; DWFILL;   /*           109 */
    volatile DWORD      Fill21[6*2];	    	                /*       10a-10f */
    
    //8000h+272*8 
    volatile DWORD      LBReadMode                  ; DWFILL;   /*           110 */
    volatile DWORD      LBReadFormat                ; DWFILL;   /*           111 */
    volatile DWORD      LBSourceOffset              ; DWFILL;   /*           112 */
    volatile DWORD      LBData                      ; DWFILL;   /*           113 */
    volatile DWORD      LBSourceData                ; DWFILL;   /*           114 */
    volatile DWORD      LBStencil                   ; DWFILL;   /*           115 */
    volatile DWORD      LBDepth                     ; DWFILL;   /*           116 */
    volatile DWORD      LBWindowBase                ; DWFILL;   /*           117 */
    volatile DWORD      LBWriteMode                 ; DWFILL;   /*           118 */
    volatile DWORD      LBWriteFormat               ; DWFILL;   /*           119 */
    volatile DWORD      LBWriteBase                 ; DWFILL;   /*           11a */
    volatile DWORD      LBWriteConfig               ; DWFILL;   /*           11b */
    volatile DWORD      LBReadPad                   ; DWFILL;   /*           11c */
    volatile DWORD      TextureData                 ; DWFILL;   /*           11d */
    volatile DWORD      TextureDownloadOffset       ; DWFILL;   /*           11e */
    volatile DWORD      LBWindowOffset		    ; DWFILL;   /*		11f */
    
    volatile DWORD      Fill22[16*2];    		        /*       120-12f */

    // 8000h+304*8
    volatile DWORD      Window                      ; DWFILL;   /*           130 */
    volatile DWORD      StencilMode                 ; DWFILL;   /*           131 */
    volatile DWORD      StencilData                 ; DWFILL;   /*           132 */
    volatile DWORD      Stencil                     ; DWFILL;   /*           133 */
    volatile DWORD      DepthMode                   ; DWFILL;   /*           134 */
    volatile DWORD      Depth                       ; DWFILL;   /*           135 */
    volatile DWORD      ZStartU                     ; DWFILL;   /*           136 */
    volatile DWORD      ZStartL                     ; DWFILL;   /*           137 */
    volatile DWORD      dZdxU                       ; DWFILL;   /*           138 */
    volatile DWORD      dZdxL                       ; DWFILL;   /*           139 */
    volatile DWORD      dZdyDomU                    ; DWFILL;   /*           13a */
    volatile DWORD      dZdyDomL                    ; DWFILL;   /*           13b */
    volatile DWORD      FastClearDepth              ; DWFILL;   /*           13c */
    volatile DWORD      LBCancelWrite               ; DWFILL;   /*           13d */
    volatile DWORD      LBWriteData                 ; DWFILL;   /*           13e */
    DWFILL; 
    DWFILL;					                /*           13f */
    
    volatile DWORD      Fill23[16*2];    		        /* 140-14f */

    // 8000h+336*8
    volatile DWORD      FBReadMode                  ; DWFILL;   /* 150 */
    volatile DWORD      FBSourceOffset              ; DWFILL;   /* 151 */
    volatile DWORD      FBPixelOffset               ; DWFILL;   /* 152 */
    volatile DWORD      FBColor                     ; DWFILL;   /* 153 */
    volatile DWORD      FBData                      ; DWFILL;   /* 154 */
    volatile DWORD      FBSourceData                ; DWFILL;   /* 155 */
    volatile DWORD      FBWindowBase                ; DWFILL;   /* 156 */
    volatile DWORD      FBWriteMode                 ; DWFILL;   /* 157 */
    volatile DWORD      FBHardwareWriteMask         ; DWFILL;   /* 158 */
    volatile DWORD      FBBlockColor                ; DWFILL;   /* 159 */
    volatile DWORD      FBReadPixel                 ; DWFILL;   /* 15a */
    volatile DWORD      FBWritePixel                ; DWFILL;   /* 15b */
    volatile DWORD      FBWriteBase                 ; DWFILL;   /* 15c */
    volatile DWORD      FBWriteConfig               ; DWFILL;   /* 15d */
    volatile DWORD      FBReadPad                   ; DWFILL;   /* 15e */
    volatile DWORD      PatternRAMMode              ; DWFILL;   /* 15f */

    volatile DWORD      PatternRamData0             ; DWFILL;   /* 160 */
    volatile DWORD      PatternRamData1             ; DWFILL;   /* 161 */
    volatile DWORD      PatternRamData2             ; DWFILL;   /* 162 */
    volatile DWORD      PatternRamData3             ; DWFILL;   /* 163 */
    volatile DWORD      PatternRamData4             ; DWFILL;   /* 164 */
    volatile DWORD      PatternRamData5             ; DWFILL;   /* 165 */
    volatile DWORD      PatternRamData6             ; DWFILL;   /* 166 */
    volatile DWORD      PatternRamData7             ; DWFILL;   /* 167 */
    volatile DWORD      PatternRamData8             ; DWFILL;   /* 168 */
    volatile DWORD      PatternRamData9             ; DWFILL;   /* 169 */
    volatile DWORD      PatternRamData10            ; DWFILL;   /* 16a */
    volatile DWORD      PatternRamData11            ; DWFILL;   /* 16b */
    volatile DWORD      PatternRamData12            ; DWFILL;   /* 16c */
    volatile DWORD      PatternRamData13            ; DWFILL;   /* 16d */
    volatile DWORD      PatternRamData14            ; DWFILL;   /* 16e */
    volatile DWORD      PatternRamData15            ; DWFILL;   /* 16f */
    volatile DWORD      PatternRamData16            ; DWFILL;   /* 170 */
    volatile DWORD      PatternRamData17            ; DWFILL;   /* 171 */
    volatile DWORD      PatternRamData18            ; DWFILL;   /* 172 */
    volatile DWORD      PatternRamData19            ; DWFILL;   /* 173 */
    volatile DWORD      PatternRamData20            ; DWFILL;   /* 174 */
    volatile DWORD      PatternRamData21            ; DWFILL;   /* 175 */
    volatile DWORD      PatternRamData22            ; DWFILL;   /* 176 */
    volatile DWORD      PatternRamData23            ; DWFILL;   /* 177 */
    volatile DWORD      PatternRamData24            ; DWFILL;   /* 178 */
    volatile DWORD      PatternRamData25            ; DWFILL;   /* 179 */
    volatile DWORD      PatternRamData26            ; DWFILL;   /* 17a */
    volatile DWORD      PatternRamData27            ; DWFILL;   /* 17b */
    volatile DWORD      PatternRamData28            ; DWFILL;   /* 17c */
    volatile DWORD      PatternRamData29            ; DWFILL;   /* 17d */
    volatile DWORD      PatternRamData30            ; DWFILL;   /* 17e */
    volatile DWORD      PatternRamData31            ; DWFILL;   /* 17f */
    
    // 8000h+384*8
    volatile DWORD      FilterMode                  ; DWFILL;   /* 180 */
    volatile DWORD      StatisticMode               ; DWFILL;   /* 181 */
    volatile DWORD      MinRegion                   ; DWFILL;   /* 182 */
    volatile DWORD      MaxRegion                   ; DWFILL;   /* 183 */
    volatile DWORD      ResetPickResult             ; DWFILL;   /* 184 */
    volatile DWORD      MinHitRegion                ; DWFILL;   /* 185 */
    volatile DWORD      MaxHitRegion                ; DWFILL;   /* 186 */
    volatile DWORD      PickResult                  ; DWFILL;   /* 187 */
    volatile DWORD      Sync                        ; DWFILL;   /* 188 */
    DWFILL; 
    DWFILL;					                /* 189 */
    DWFILL; 
    DWFILL;				                        /* 18a */
    DWFILL; 
    DWFILL;					                /* 18b */
    DWFILL; 
    DWFILL;					                /* 18c */
    volatile DWORD      FBBlockColorUpper           ; DWFILL;   /* 18d */
    volatile DWORD      FBBlockColorLower           ; DWFILL;   /* 18e */
    volatile DWORD      SuspendUntilFrameBlank      ; DWFILL;   /* 18f */

    volatile DWORD      KsRStart		    ; DWFILL;   /* 190 */
    volatile DWORD      dKsRdx			    ; DWFILL;   /* 191 */
    volatile DWORD      dKsRdyDom		    ; DWFILL;   /* 192 */
    volatile DWORD      KsGStart		    ; DWFILL;   /* 193 */
    volatile DWORD      dKsGdx			    ; DWFILL;   /* 194 */
    volatile DWORD      dKsGdyDom		    ; DWFILL;   /* 195 */
    volatile DWORD      KsBStart		    ; DWFILL;   /* 196 */
    volatile DWORD      dKsBdx			    ; DWFILL;   /* 197 */
    volatile DWORD      dKsBdyDom		    ; DWFILL;   /* 198 */

    volatile DWORD      Fill24[7*2];		               /* 199-19f */

    volatile DWORD      KdRStart		    ; DWFILL;  /* 1a0 */
    volatile DWORD      dKdRdx			    ; DWFILL;  /* 1a1 */
    volatile DWORD      dKdRdyDom		    ; DWFILL;  /* 1a2 */
    volatile DWORD      KdGStart		    ; DWFILL;  /* 1a3 */
    volatile DWORD      dKdGdx			    ; DWFILL;  /* 1a4 */
    volatile DWORD      dKdGdyDom		    ; DWFILL;  /* 1a5 */
    volatile DWORD      KdBStart	 	    ; DWFILL;  /* 1a6 */
    volatile DWORD      dKdBdx			    ; DWFILL;  /* 1a7 */
    volatile DWORD      dKdBdyDom                   ; DWFILL;  /* 1a8 */

    volatile DWORD      Fill25[39*2];    		       /* 1a9-1af */

    volatile DWORD      TexelLUT0                   ; DWFILL;  /* 1d0 */
    volatile DWORD      TexelLUT1                   ; DWFILL;  /* 1d1 */
    volatile DWORD      TexelLUT2                   ; DWFILL;  /* 1d2 */
    volatile DWORD      TexelLUT3                   ; DWFILL;  /* 1d3 */
    volatile DWORD      TexelLUT4                   ; DWFILL;  /* 1d4 */
    volatile DWORD      TexelLUT5                   ; DWFILL;  /* 1d5 */
    volatile DWORD      TexelLUT6                   ; DWFILL;  /* 1d6 */
    volatile DWORD      TexelLUT7                   ; DWFILL;  /* 1d7 */
    volatile DWORD      TexelLUT8                   ; DWFILL;  /* 1d8 */
    volatile DWORD      TexelLUT9                   ; DWFILL;  /* 1d9 */
    volatile DWORD      TexelLUT10                  ; DWFILL;  /* 1da */
    volatile DWORD      TexelLUT11                  ; DWFILL;  /* 1db */
    volatile DWORD      TexelLUT12                  ; DWFILL;  /* 1dc */
    volatile DWORD      TexelLUT13                  ; DWFILL;  /* 1dd */
    volatile DWORD      TexelLUT14                  ; DWFILL;  /* 1de */
    volatile DWORD      TexelLUT15                  ; DWFILL;  /* 1df */

    volatile DWORD      YUVMode                     ; DWFILL;  /* 1e0 */
    volatile DWORD      ChromaUpperBound            ; DWFILL;  /* 1e1 */
    volatile DWORD      ChromaLowerBound            ; DWFILL;  /* 1e2 */
    volatile DWORD      ChromaTestMode              ; DWFILL;  /* 1e3 */
    volatile DWORD      Fill26[28*2];                         /* 1e4-1ff */

    // 8000h+512*8 DELTA specific

    volatile DWORD      V0Fixed0                    ; DWFILL;  /* 0x200 */
    volatile DWORD      V0Fixed1                    ; DWFILL;  /* 0x201 */
    volatile DWORD      V0Fixed2                    ; DWFILL;  /* 0x202 */
    volatile DWORD      V0Fixed3                    ; DWFILL;  /* 0x203 */
    volatile DWORD      V0Fixed4                    ; DWFILL;  /* 0x204 */
    volatile DWORD      V0Fixed5                    ; DWFILL;  /* 0x205 */
    volatile DWORD      V0Fixed6                    ; DWFILL;  /* 0x206 */
    volatile DWORD      V0Fixed7                    ; DWFILL;  /* 0x207 */
    volatile DWORD      V0Fixed8                    ; DWFILL;  /* 0x208 */
    volatile DWORD      V0Fixed9                    ; DWFILL;  /* 0x209 */
    volatile DWORD      V0FixedA                    ; DWFILL;  /* 0x20A */
    volatile DWORD      V0FixedB                    ; DWFILL;  /* 0x20B */
    volatile DWORD      V0FixedC                    ; DWFILL;  /* 0x20C */
    volatile DWORD      Fill27[3*2];

    volatile DWORD      V1Fixed0                    ; DWFILL;  /* 0x210 */
    volatile DWORD      V1Fixed1                    ; DWFILL;  /* 0x211 */
    volatile DWORD      V1Fixed2                    ; DWFILL;  /* 0x212 */
    volatile DWORD      V1Fixed3                    ; DWFILL;  /* 0x213 */
    volatile DWORD      V1Fixed4                    ; DWFILL;  /* 0x214 */
    volatile DWORD      V1Fixed5                    ; DWFILL;  /* 0x215 */
    volatile DWORD      V1Fixed6                    ; DWFILL;  /* 0x216 */
    volatile DWORD      V1Fixed7                    ; DWFILL;  /* 0x217 */
    volatile DWORD      V1Fixed8                    ; DWFILL;  /* 0x218 */
    volatile DWORD      V1Fixed9                    ; DWFILL;  /* 0x219 */
    volatile DWORD      V1FixedA                    ; DWFILL;  /* 0x21A */
    volatile DWORD      V1FixedB                    ; DWFILL;  /* 0x21B */
    volatile DWORD      V1FixedC                    ; DWFILL;  /* 0x21C */
    volatile DWORD      Fill28[3*2];

    volatile DWORD      V2Fixed0                    ; DWFILL;  /* 0x220 */
    volatile DWORD      V2Fixed1                    ; DWFILL;  /* 0x221 */
    volatile DWORD      V2Fixed2                    ; DWFILL;  /* 0x222 */
    volatile DWORD      V2Fixed3                    ; DWFILL;  /* 0x223 */
    volatile DWORD      V2Fixed4                    ; DWFILL;  /* 0x224 */
    volatile DWORD      V2Fixed5                    ; DWFILL;  /* 0x225 */
    volatile DWORD      V2Fixed6                    ; DWFILL;  /* 0x226 */
    volatile DWORD      V2Fixed7                    ; DWFILL;  /* 0x227 */
    volatile DWORD      V2Fixed8                    ; DWFILL;  /* 0x228 */
    volatile DWORD      V2Fixed9                    ; DWFILL;  /* 0x229 */
    volatile DWORD      V2FixedA                    ; DWFILL;  /* 0x22A */
    volatile DWORD      V2FixedB                    ; DWFILL;  /* 0x22B */
    volatile DWORD      V2FixedC                    ; DWFILL;  /* 0x22C */
    volatile DWORD      Fill29[3*2];

    volatile DWORD      V0Float0                    ; DWFILL;  /* 0x230 */
    volatile DWORD      V0Float1                    ; DWFILL;  /* 0x231 */
    volatile DWORD      V0Float2                    ; DWFILL;  /* 0x232 */
    volatile DWORD      V0Float3                    ; DWFILL;  /* 0x233 */
    volatile DWORD      V0Float4                    ; DWFILL;  /* 0x234 */
    volatile DWORD      V0Float5                    ; DWFILL;  /* 0x235 */
    volatile DWORD      V0Float6                    ; DWFILL;  /* 0x236 */
    volatile DWORD      V0Float7                    ; DWFILL;  /* 0x237 */
    volatile DWORD      V0Float8                    ; DWFILL;  /* 0x238 */
    volatile DWORD      V0Float9                    ; DWFILL;  /* 0x239 */
    volatile DWORD      V0FloatA                    ; DWFILL;  /* 0x23A */
    volatile DWORD      V0FloatB                    ; DWFILL;  /* 0x23B */
    volatile DWORD      V0FloatC                    ; DWFILL;  /* 0x23C */
    volatile DWORD      Fill30[3*2];

    volatile DWORD      V1Float0                    ; DWFILL;  /* 0x240 */
    volatile DWORD      V1Float1                    ; DWFILL;  /* 0x241 */
    volatile DWORD      V1Float2                    ; DWFILL;  /* 0x242 */
    volatile DWORD      V1Float3                    ; DWFILL;  /* 0x243 */
    volatile DWORD      V1Float4                    ; DWFILL;  /* 0x244 */
    volatile DWORD      V1Float5                    ; DWFILL;  /* 0x245 */
    volatile DWORD      V1Float6                    ; DWFILL;  /* 0x246 */
    volatile DWORD      V1Float7                    ; DWFILL;  /* 0x247 */
    volatile DWORD      V1Float8                    ; DWFILL;  /* 0x248 */
    volatile DWORD      V1Float9                    ; DWFILL;  /* 0x249 */
    volatile DWORD      V1FloatA                    ; DWFILL;  /* 0x24A */
    volatile DWORD      V1FloatB                    ; DWFILL;  /* 0x24B */
    volatile DWORD      V1FloatC                    ; DWFILL;  /* 0x24C */
    volatile DWORD      Fill31[3*2];

    volatile DWORD      V2Float0                    ; DWFILL;  /* 0x250 */
    volatile DWORD      V2Float1                    ; DWFILL;  /* 0x251 */
    volatile DWORD      V2Float2                    ; DWFILL;  /* 0x252 */
    volatile DWORD      V2Float3                    ; DWFILL;  /* 0x253 */
    volatile DWORD      V2Float4                    ; DWFILL;  /* 0x254 */
    volatile DWORD      V2Float5                    ; DWFILL;  /* 0x255 */
    volatile DWORD      V2Float6                    ; DWFILL;  /* 0x256 */
    volatile DWORD      V2Float7                    ; DWFILL;  /* 0x257 */
    volatile DWORD      V2Float8                    ; DWFILL;  /* 0x258 */
    volatile DWORD      V2Float9                    ; DWFILL;  /* 0x259 */
    volatile DWORD      V2FloatA                    ; DWFILL;  /* 0x25A */
    volatile DWORD      V2FloatB                    ; DWFILL;  /* 0x25B */
    volatile DWORD      V2FloatC                    ; DWFILL;  /* 0x25C */
    volatile DWORD      Fill32[3*2];

    volatile DWORD      DeltaMode                   ; DWFILL;  /* 0x260 */
    volatile DWORD      DrawTriangle                ; DWFILL;  /* 0x261 */
    volatile DWORD      RepeatTriangle              ; DWFILL;  /* 0x262 */
    volatile DWORD      DrawLine01                  ; DWFILL;  /* 0x263 */
    volatile DWORD      DrawLine10                  ; DWFILL;  /* 0x264 */
    volatile DWORD      RepeatLine                  ; DWFILL;  /* 0x265 */
    volatile DWORD      Fill33[9*2];
    volatile DWORD      BroadcastMask               ; DWFILL;  /* 0x26F */
} GLREG, *PGLREG, far *FPGLREG, FAR *FPPERMEDIA2REG, PERMEDIA2REG;



