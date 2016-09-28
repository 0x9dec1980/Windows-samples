/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: vesaddc.c
 * This file provides the functions required to support the VESA
 * Display Data Channel protocols (DDC).  These allow information about
 * an attached monitor to be read.  From this the operating system
 * can set up the display hardware to drive the monitor optimally.
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/



#include <basedef.h>
#include "glint.h"
#include <vmm.h>
#include <pbt.h>
#include <dbt.h>
#include <pci.h>

// Vesa VBE return status.
#define VESA_NO_SUPPORT	0x0000	// Function not supported
#define VESA_SUCCESS	0x004F	// Function call successful
#define	VESA_FAILED		0x014F	// Function call failed

#define	EDIDBLOCKSIZE	128		// Number of bytes in an EDID block

// Internal status/error codes. (Not seen outside this file)
enum I2CSTATUS {
    eI2COK = 0,		        // No problems encountered.
    eI2CBUSNOTREADY,		// I2C bus signals not ready for block transfer.
    eI2CSLAVETOOSLOW,		// Slave device took too long to release clock.
    eI2CNOACK			// Slave did not acknowledge a sent byte.
};

// Vesa VBE DDC sub functions supported.
#define	DDC_Capability	0
#define DDC_ReadEDID	1

// Prototypes
enum I2CSTATUS i2CReadBlock(PDEVICETABLE pDev, char *buffer, BYTE blockNumber, int blockSize);
enum I2CSTATUS i2CInputByte(PDEVICETABLE pDev, BYTE *theByte, int acknowledge);
enum I2CSTATUS i2COutputByte(PDEVICETABLE pDev, BYTE theByte);
enum I2CSTATUS i2CWaitForSlave(PDEVICETABLE pDev);
void           i2CWriteLinesDelay(PDEVICETABLE pDev, BYTE sda, BYTE scl);
void           i2CSendStart(PDEVICETABLE pDev);
void           i2CSendStop(PDEVICETABLE pDev);

void *Pointer32(DWORD VirtualMachine, DWORD segment, DWORD offset);
void DelayUs(PDEVICETABLE pDev, DWORD microseconds);

int  i2CReadEDIDBlock(PDEVICETABLE pDev, char *buffer, BYTE blockNumber);
void i2CReadLinesP2(PDEVICETABLE pDev, BYTE *sda, BYTE *scl);	// P2 method of reading DDC lines
void i2CWriteLinesP2(PDEVICETABLE pDev, BYTE sda, BYTE scl);
void i2CReadLinesPAL(PDEVICETABLE pDev, BYTE *sda, BYTE *scl);	// PAL method of reading DDC lines
void i2CWriteLinesPAL(PDEVICETABLE pDev, BYTE sda, BYTE scl);
void i2CReadLinesTVP(PDEVICETABLE pDev, BYTE *sda, BYTE *scl);	// TVP method of reading DDC lines
void i2CWriteLinesTVP(PDEVICETABLE pDev, BYTE sda, BYTE scl);


void (* i2CReadFunc)(PDEVICETABLE pDev, BYTE *sda, BYTE *scl);	// Pointer to function responsible for reading DDC data
void (* i2CWriteFunc)(PDEVICETABLE pDev, BYTE sda, BYTE scl);	// Pointer to function responsible for writing DDC data


//--------------------------------------------------------------
// VESA VBE/DDC Sub function dispatcher
//
DWORD 
Vesa_DDC(PDEVICETABLE   pDev, 
         CLIENT_STRUCT *Regs, 
         DWORD          VirtualMachine)
{
    // DDC is only supported by the driver for PERMEDIA family boards
    if (pDev->FamilyID != PERMEDIA_ID)
        return 0;

    // Decode the requested DDC sub-function.
    switch(Regs->CBRS.Client_BL) 
    {
	case DDC_Capability:
	//(VESA VBE/DDC Sub-function 00h)
	// Return supported function information
	// Entry: CX = Controller unit number (00 = Primary controller)
	//
	// Exit:  AX = Status code
	//        BH = Approx seconds required to transfer 128 byte EDID block.
	//        BL = DDC level supported
	//             Bit 0 = DDC1 supported/not supported
	//             Bit 1 = DDC2 supported/not supported
	//             Bit 2 = Screen blanked during data transfer
	    Regs->CWRS.Client_BX = 0x0102;	// 1 second transfer, DDC2 supported.
	    Regs->CWRS.Client_AX = VESA_SUCCESS;
	break;

	case DDC_ReadEDID:
	//(VESA VBE/DDC Sub-function 01h)
	// Read EDID data from monitor into user buffer.
	//
	// NOTE: There are three methods of supporting DDC on various PERMEDIA reference cards.  They are
	// listed here in order of historical implementation:
	//
	// Method 1 (The PAL method) This applies to those reference boards fitted with a PAL which enables
	// the DDC lines to be controlled over the PERMEDIA's auxilliary bus.
	//
	// Method 2 (The TVP method) This method is only available on boards using the TI TVP3026 RAMDAC.
	// The RAMDAC's general purpose IO ports are wired to the DDC lines.
	//
	// Method 3 (The P2 method)  This method makes use of the PERMEDIA 2's built in DDC ports.
	//
	// The algorithm for selecting the correct method is as follows.  Note that apart from the P2 method
	// it is not possible to detect which method to use without trying it first.
	//
	// If Chip is a P2 then
	//		Use P2 method
	// else {
	//		Try PAL method
	//		If PAL method fails then
	//			Try TVP method
	// }
	//
	// Entry: CX = Controller unit number (00 = Primary controller)
	//        DX = EDID block number (Zero for ver 1.0)
	//        ES:DI = Address of 128 byte buffer to fill with EDID block
	//
	// Exit:  AX = Status code
	{
	    char *pEDIDBuffer = 0;
	    WORD result;
	    BYTE blockNumber = (BYTE)(Regs->CWRS.Client_DX);

	    pEDIDBuffer = Pointer32(	// convert ES:DI to valid C pointer.
			            VirtualMachine,
			            Regs->CRS.Client_ES,
			            Regs->CWRS.Client_DI);
	    
	    i2CReadFunc  = &i2CReadLinesP2;
	    i2CWriteFunc = &i2CWriteLinesP2;
	    result = i2CReadEDIDBlock(pDev, pEDIDBuffer, blockNumber);
	    Regs->CWRS.Client_AX = result;
	}
        break;

        default:
        // Unsupported sub function requested
            Regs->CWRS.Client_AX = VESA_NO_SUPPORT;
        break;
    }
    return 1;
}


//--------------------------------------------------------------
int 
i2CReadEDIDBlock(PDEVICETABLE   pDev, 
                 char           *pEDIDBuffer, 
                 BYTE           blockNumber)
{
    int     repeatCount;
    BYTE    checkSum;
    char    *pCheckByte;
    int     checkCount;
    WORD    result = VESA_FAILED;	// Assume the worst until proven otherwise.

    // Select DDC2 Mode.
    // DDC2 displays power up in DDC1 mode.  The display switches to DDC2 as
    // soon as it sees a high to low transition on the clock line (SCL).
    // Best way to do this is to send a STOP signal.  This will stop any current
    // transfer in progress and prepare it for the next.

    i2CWriteLinesDelay(pDev, 0,0);	// Start DDC2 mode by toggling SCL
    i2CWriteLinesDelay(pDev, 0,1);
    i2CWriteLinesDelay(pDev, 0,0);
    i2CWriteLinesDelay(pDev, 0,1);
    i2CWriteLinesDelay(pDev, 1,1);	// Send STOP signal

    DelayUs(pDev, 100);				// Give monitor time to switch to DDC2

    repeatCount = 8;			// If at first you don't succeed...
    while (repeatCount--) 
    {
	if (i2CReadBlock(pDev, pEDIDBuffer, blockNumber, EDIDBLOCKSIZE) == eI2COK) 
        {
	    // Calculate checksum and compare it to the read value.
	    checkSum = 0;
	    pCheckByte = pEDIDBuffer;
	    checkCount = EDIDBLOCKSIZE;

	    while (checkCount--)
		checkSum += *pCheckByte++;

	    if ((checkSum & 0xFF) == 0) 
            {
		result = VESA_SUCCESS;
		break;
	    }
	}
    }
    return result;
}



//--------------------------------------------------------------
enum I2CSTATUS	
i2CReadBlock(PDEVICETABLE   pDev, 
             char           *buffer, 
             BYTE           blockNumber, 
             int            blockSize)
{
    BYTE sda, scl;
    BYTE aByte;
    enum I2CSTATUS status;


    // Test bus is ready
    i2CWriteLinesDelay(pDev, 1,1);  // Float both lines
    (* i2CReadFunc)(pDev, &sda, &scl);

    if (sda != 1 && scl != 1) 
    {
	return eI2CBUSNOTREADY;	    // I2C Bus not ready.
    }

    // Command display to send block.
    i2CSendStart(pDev);             // Send START signal

    status = i2COutputByte(pDev, 0xA0); // Address the display for writing
    if (status == eI2COK) 
    {
	status = i2COutputByte(pDev, blockNumber);	// Set block number
	if (status == eI2COK) 
        {
	    // Get block
	    i2CSendStart(pDev);						// Send repeated START signal
	    status = i2COutputByte(pDev, 0xA1);		// Address the display for reading
	    if (status == eI2COK) 
            {
		while (blockSize--) 
                {
		    // Read a block's worth of bytes from the device.
		    // Acknowledge every byte except the last.
		    status = i2CInputByte(pDev, &aByte, blockSize > 0);
		    if (status != eI2COK)
			break;
		    *buffer++ = aByte;
		}
	    }
	}
    }
    i2CSendStop(pDev);
    return status;
}


//--------------------------------------------------------------
// Read a byte from the I2C bus.
//
// Returns:	eI2COK on success
//
enum I2CSTATUS 
i2CInputByte(PDEVICETABLE   pDev, 
             BYTE           *theByte, 
             int            acknowledge)
{
    int	    bitCount = 8;
    BYTE    sda, scl;
    BYTE    data = 0;
    enum    I2CSTATUS status;

    // Read the byte from the slave

    while (bitCount--) 
    {
	i2CWriteLinesDelay(pDev, 1,1);
	status = i2CWaitForSlave(pDev);
	if (status != eI2COK)
	    return status;
	(* i2CReadFunc)(pDev, &sda, &scl);
	data = data << 1;
	data |= sda;
	i2CWriteLinesDelay(pDev, 1, 0);
    }
    *theByte = data;

    // Send an Acknowledge or not Acknowledge to the slave.
    // Not acknowledging a byte is a signal to the slave that
    // the master has finished reading.  The slave will then
    // release the data line.

    if (acknowledge)
    {
	i2CWriteLinesDelay(pDev, 0, 0);
	i2CWriteLinesDelay(pDev, 0, 1);
	status = i2CWaitForSlave(pDev);
	if (status != eI2COK)
	    return status;
	i2CWriteLinesDelay(pDev, 0, 0);
    } else 
    {
	i2CWriteLinesDelay(pDev, 1, 0);
	i2CWriteLinesDelay(pDev, 1, 1);
	status = i2CWaitForSlave(pDev);
	if (status != eI2COK)
	    return status;
    }
    i2CWriteLinesDelay(pDev, 1, 0);		// Release data bus

    return eI2COK;					// Successful byte read
}


//--------------------------------------------------------------
// Write a byte to the I2C bus.
//
enum I2CSTATUS 
i2COutputByte(PDEVICETABLE  pDev, 
              BYTE          theByte)
{
    BYTE sda, scl;
    int bitCount = 8;
    enum I2CSTATUS status;

    while (bitCount--) 
    {
	sda = (theByte >> 7) & 1;		// Load MSB
	theByte = theByte << 1;			// Ready next bit.
	i2CWriteLinesDelay(pDev, sda, 0);
	i2CWriteLinesDelay(pDev, sda, 1);
	status = i2CWaitForSlave(pDev);
	if (status != eI2COK)
		return status;
	i2CWriteLinesDelay(pDev, sda, 0);
    }

    // Check that slave acknowledges the byte just sent.

    i2CWriteLinesDelay(pDev, 1, 0);			// Release data line.
    i2CWriteLinesDelay(pDev, 1, 1);			// Start acknowledge clock
    status = i2CWaitForSlave(pDev);
    if (status != eI2COK)
	return status;

    (* i2CReadFunc)(pDev, &sda, &scl);
    i2CWriteLinesDelay(pDev, 1, 0);			// Finish acknowledge clock

    if (sda == 0)
	status = eI2COK;
    else
	status = eI2CNOACK;				// Slave did not acknowledge byte

    return status;
}



//--------------------------------------------------------------
void 
i2CWriteLinesDelay(PDEVICETABLE pDev, 
                   BYTE         sda, 
                   BYTE         scl)
{
    (* i2CWriteFunc)(pDev, sda, scl);
    DelayUs(pDev, 17);			// Wait 17 uS approx
}



//--------------------------------------------------------------
// Check SCL line to see if slave device is holding it low.
// This mechanism allows slower slave devices to be connected
// to the I2C bus.
//
// Assumes SCL line has been released (high) by master.
//
enum I2CSTATUS 
i2CWaitForSlave(PDEVICETABLE pDev)
{
    WORD timeout = 32000;
    BYTE sda, scl;

    while (timeout--) 
    {
	(* i2CReadFunc)(pDev, &sda, &scl);
	if (scl)
	    return eI2COK;		// Successful stall
	DelayUs(pDev, 1);				// 1 uS wait Approx
    }
    return eI2CSLAVETOOSLOW;	// Slave took too long
}


//--------------------------------------------------------------
void 
i2CSendStart(PDEVICETABLE pDev)
{
    // The start signal is defined to be a high to low transition
    // on the data line while the clock signal is high.
    i2CWriteLinesDelay(pDev, 1,1);
    i2CWriteLinesDelay(pDev, 0,1);
    i2CWriteLinesDelay(pDev, 0,0);
}


//--------------------------------------------------------------
void 
i2CSendStop(PDEVICETABLE pDev)
{
	// The stop signal is defined to be a low to high transition
	// on the data line while the clock signal is high.
	i2CWriteLinesDelay(pDev, 0,0);
	i2CWriteLinesDelay(pDev, 0,1);
	i2CWriteLinesDelay(pDev, 1,1);
}



//==============================================================
// NON-PORTABLE CODE SECTION.
// The following code is Windows95 Driver specific.
//==============================================================

//--------------------------------------------------------------
void 
DelayUs(PDEVICETABLE    pDev, 
        DWORD           microseconds)
{
    // Delay for requested number of microseconds
    // On a 33Mhz PCI bus a Permedia register read takes approx 0.5 microseconds.
    // The first access may take longer due to having to flush pending writes.
    // However we don't need atomic clock accuracy here.
    volatile DWORD ulvalue;

    while (microseconds--) 
    {
	READ_GLINT_CTRL_REG(InFIFOSpace, ulvalue);
	READ_GLINT_CTRL_REG(InFIFOSpace, ulvalue);
    }
}



//--------------------------------------------------------------
// Read and return the SDA and SCL lines via the TVP3026 RAMDAC
//
// Returns: *scl = Serial Clock Line (SCL)
//          *sda = Serial data line (SDA)
void
i2CReadLinesTVP(PDEVICETABLE pDev, 
                BYTE        *sda, 
                BYTE        *scl)
{
    TVP3026_DECL;
    ULONG value;

    TVP3026_WRITE_INDEX_REG(__TVP3026_GP_CONTROL, 0);	// Set all port bits to read mode.
    TVP3026_READ_INDEX_REG(__TVP3026_GP_DATA, value);	// Read port bits.
    
    *sda = (BYTE)(value & 1);
    *scl = (BYTE)((value >> 1) & 1);
}


//--------------------------------------------------------------
// Write new values to the SDA and SCL lines via the TVP3026 RAMDAC
//
void 
i2CWriteLinesTVP(PDEVICETABLE   pDev, 
                 BYTE           sda, 
                 BYTE           scl)
{
    TVP3026_DECL;
    BYTE value;
    BYTE control;

    // The TVP3026's general purpose io port bits can individually
    // be set to read or write.  For the purpose of emulating an I2C
    // bus the code sets the port to write a zero but read a one.

    control = (scl ? 0 : 2) | (sda ? 0 : 1);
    value = ((scl & 1) << 1) | (sda & 1);
    TVP3026_WRITE_INDEX_REG(__TVP3026_GP_CONTROL, control);
    TVP3026_WRITE_INDEX_REG(__TVP3026_GP_DATA, value);
}


//--------------------------------------------------------------
// Read and return the SDA and SCL lines
//
// Returns: *scl = Serial Clock Line (SCL)
//          *sda = Serial data line (SDA)
 
void 
i2CReadLinesPAL(PDEVICETABLE pDev, 
                BYTE        *sda, 
                BYTE        *scl)
{
    BYTE	value;

    value = *((BYTE *)pDev->pRegisters + (0x5000 + 2));		// Read SDA
    *sda = value & 1;

    value = *((BYTE *)pDev->pRegisters + (0x5000 + 1));		// Read SCL
    *scl = value & 1;
}


//--------------------------------------------------------------
// Write new values to the SDA and SCL lines.
//
void 
i2CWriteLinesPAL(PDEVICETABLE   pDev, 
                 BYTE           sda, 
                 BYTE           scl)
{
    BYTE value;

    value = ((sda & 1) << 1) | (scl & 1);
    *((BYTE *)pDev->pRegisters + (0x5000 + 0)) = value;
}



//--------------------------------------------------------------
// Read and return the SDA and SCL lines via the P2 Chip
//
// Returns: *scl = Serial Clock Line (SCL)
//          *sda = Serial data line (SDA)
void 
i2CReadLinesP2(PDEVICETABLE pDev, 
               BYTE         *sda, 
               BYTE         *scl)
{
    ULONG ulDisplayData;

    READ_GLINT_CTRL_REG(DisplayData, ulDisplayData);// Read the current inputs

    *sda = (BYTE)(ulDisplayData & 1);
    *scl = (BYTE)((ulDisplayData >> 1) & 1);
}


//--------------------------------------------------------------
// Write new values to the SDA and SCL lines via the P2 Chip
//
void 
i2CWriteLinesP2(PDEVICETABLE    pDev, 
                BYTE            sda, 
                BYTE            scl)
{
    ULONG ulDisplayData = 0x0000E000;

    ulDisplayData |= (ULONG)( ((sda & 1) << 2) | ((scl & 1) << 3) );
    LOAD_GLINT_CTRL_REG(DisplayData, ulDisplayData);
}


//--------------------------------------------------------------
// Convert a real mode Segment/Offset address into 32bit protected address.
//
void *
Pointer32(DWORD VirtualMachine, 
          DWORD segment, 
          DWORD offset)
{
    DWORD   HWord, LWord;
    DWORD   Base;
    VOID   *result = 0;

    // First get the Virtual Memory Manager (VMM) to get the segment description
    // information from the given segment selector.

    __asm {
        pushad
        xor     eax, eax
        push    eax
        push    VirtualMachine
        push    segment
        VMMCall(_GetDescriptor)
        add     esp, 0xc
        mov     HWord, edx
        mov     LWord, eax
        popad
    }

    if (HWord == 0  && LWord == 0) 
    {
	// The segment is not a valid selector.  Resort to HACKERY!
	// ABANDON ALL HOPE YE WHO ENTER HERE.
	result = (void *)(*((DWORD *)VirtualMachine + 1) + segment*16 + offset);
    } else 
    {
	// Extract the base address of the selector.
	Base = (LWord >> 16) | ((HWord & 0xff) << 16) | (HWord & 0xff000000);
	result = (void *)(Base + offset);
    }
    return result;
}
