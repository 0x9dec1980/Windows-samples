;/******************************Module*Header**********************************\
; *
; *                           ***************************************
; *                           * Permedia 2: miniVDD SAMPLE CODE   *
; *                           ***************************************
; *
; * Module Name: glint.inc
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

; Should be defined in newer minivdd.inc
DEVICE_IS_NOT_VGA   equ 800h

NBR_MINI_VDD_FUNCTIONS_40            equ 49
;//
;//  new miniVDD functions that a 4.1 miniVDD should implement
;//
GET_NUM_UNITS                        equ 49
TURN_VGA_OFF                         equ 50
TURN_VGA_ON                          equ 51
SET_ADAPTER_POWER_STATE              equ 52
GET_ADAPTER_POWER_STATE_CAPS         equ 53
SET_MONITOR_POWER_STATE              equ 54
GET_MONITOR_POWER_STATE_CAPS         equ 55
GET_MONITOR_INFO                     equ 56
ifndef WIN98
NBR_MINI_VDD_FUNCTIONS_41            equ 57
endif
GET_ACTIVE_MONITOR                   equ 57
SET_ACTIVE_MONITOR                   equ 58


InitialiseDynamicInit            PROTO NEAR C :DWORD
VesaSupport                      PROTO NEAR C :DWORD,:DWORD
TurnVGAOn                        PROTO NEAR C :DWORD,:DWORD
TurnVGAOff                       PROTO NEAR C :DWORD,:DWORD
EnableVGA                        PROTO NEAR C :DWORD		; 
HandleInterrupt					 PROTO NEAR C :DWORD	;
DisableVGA                       PROTO NEAR C 
DisableHardware                  PROTO NEAR C 
ReinitialiseMode    			 PROTO NEAR C
RegisterDisplayDriver            PROTO NEAR C :DWORD
FindDevNodeForCurrentlyActiveVGA PROTO NEAR C
FindpDevFromDevNodeTable         PROTO NEAR C :DWORD            ; 
FindPrimarypDev                  PROTO NEAR C
vddGetIRQInfo					 PROTO NEAR C :DWORD,:DWORD
vddIsOurIRQ						 PROTO NEAR C :DWORD
vddEnableIRQ					 PROTO NEAR C :DWORD,:DWORD
vddFlipVideoPort				 PROTO NEAR C :DWORD,:DWORD
vddFlipOverlay					 PROTO NEAR C :DWORD,:DWORD
vddGetCurrentAutoflip			 PROTO NEAR C :DWORD,:DWORD,:DWORD
vddGetPreviousAutoflip			 PROTO NEAR C :DWORD,:DWORD,:DWORD
vddLock							 PROTO NEAR C :DWORD,:DWORD,:DWORD
vddSetState						 PROTO NEAR C :DWORD,:DWORD,:DWORD
vddGetFieldPolarity				 PROTO NEAR C :DWORD,:DWORD,:DWORD
vddSkipNextField				 PROTO NEAR C :DWORD,:DWORD

ifdef POWER_STATE_MANAGE
SetAdapterPowerState		PROTO NEAR C :DWORD,:DWORD 
endif

AccessRegistry   				 PROTO NEAR C :DWORD,:DWORD,:DWORD
PNP_NewDevnode   				 PROTO NEAR C :DWORD,:DWORD,:DWORD

DEVICETABLE STRUCT
	pRegisters          dd  0           
    pGLInfo             dd  0
    IRQHandle           dd  0
DEVICETABLE ENDS
