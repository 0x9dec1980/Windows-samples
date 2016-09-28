/******************************Module*Header**********************************\
 *
 *                           ***************************************
 *                           * Permedia 2: miniVDD SAMPLE CODE   *
 *                           ***************************************
 *
 * Module Name: i2c.c
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include <basedef.h>
#include <vmm.h>
#include <vmmreg.h>
#include <vwin32.h>
#include "glint.h"
#include <regdef.h>

#define DELAY Delay(pDev, 30)
#define READ_I2C ReadI2C(pDev);
#define WRITE_I2C WriteI2C(pDev);
#define CHECK_DATA(a) CheckData(pDev, a);
#define CHECK_CLOCK(a) CheckClock(pDev, a);

#define MODE_WRITE 0
#define MODE_READ 1

BOOL SendSlaveAddress(PDEVICETABLE pDev, WORD wAddress, int Access)							
{																
    BYTE MajorAddress;											
    BYTE MinorAddress;											
    MajorAddress = (BYTE)((wAddress & 0xFF00) >> 8);			
    if (MajorAddress != 0)										
    {															
	if (Access == MODE_READ)								
	{														
	    MajorAddress |= 1;									
	}														
	else													
	{														
	    MajorAddress &= 0xFE;								
	}														
	MinorAddress = (BYTE)(wAddress & 0xFF);					
	if (!I2CSendDataByte(pDev, MajorAddress)) 
            return FALSE;	
	if (!I2CSendDataByte(pDev, MinorAddress)) 
            return FALSE;	
    } 
    else
    {													
	MinorAddress = (BYTE)(wAddress & 0xFF);					
	if (Access == MODE_READ)								
	{														
	    MinorAddress |= 1;									
	}														
	else													
	{														
	    MinorAddress &= 0xFE;								
	}														
	if (!I2CSendDataByte(pDev, MinorAddress)) 
            return FALSE;
    }															
    return TRUE;
}

// For timing out
DWORD LastClock;
DWORD LastData;
DWORD dwStartTime;
DWORD dwLastTime;
#define TIMEOUT 100000
#define START_TIMEOUT READ_GLINT_CTRL_REG(MClkCount, dwStartTime);
#define TIMEOUT_CHECK						\
READ_GLINT_CTRL_REG(MClkCount, dwLastTime);		\
if ((dwLastTime - dwStartTime) > TIMEOUT)	\
{											\
	DISPDBG(("GLINTMVD: ERROR: Timed out!"));		\
	break;									\
}

#if BIG_ENDIAN == 1
typedef struct tagI2CReg
{
	DWORD Reserved		: 28;
	DWORD Clock			: 1;
	DWORD Data			: 1;
	DWORD ClkIn			: 1;
	DWORD DataIn		: 1;
} __I2C;
#else
typedef struct tagI2CReg
{
	DWORD DataIn		: 1;
	DWORD ClkIn			: 1;
	DWORD Data			: 1;
	DWORD Clock			: 1;
	DWORD Reserved		: 28;
} __I2C;
#endif

__I2C I2C;

static void Delay(PDEVICETABLE pDev, int microseconds)
{
    DWORD dwCurrentValue;
    DWORD dwStartValue;
    DWORD TimeOut = (DWORD)(microseconds * 200);

    // The MClk never stops.... If it is dead, so is the chip.
    READ_GLINT_CTRL_REG(MClkCount, dwStartValue);
    do
    {
	READ_GLINT_CTRL_REG(MClkCount, dwCurrentValue);
	if ((dwCurrentValue - dwStartValue) > TimeOut) 
            break;
    } while(1);
}

// Read and write to the bus
void ReadI2C(PDEVICETABLE pDev)
{
    READ_GLINT_CTRL_REG(VSSerialBusControl, (*((DWORD*)&I2C)));
    DISPDBG(("GLINTMVD: READ:  Clock: %d, Data: %d", (int)I2C.ClkIn, (int)I2C.DataIn));
}

void WriteI2C(PDEVICETABLE pDev)
{
    LOAD_GLINT_CTRL_REG(VSSerialBusControl, (*((DWORD*)&I2C)));
    DISPDBG(("GLINTMVD: WRITE: Clock: %d, Data: %d", (int)I2C.Clock, (int)I2C.Data));
    DELAY;
}

// Validate the signals
BOOL CheckClock(PDEVICETABLE pDev, BOOL bOne)
{
    START_TIMEOUT;
    do
    {
	READ_I2C;
	TIMEOUT_CHECK;
    }
    while ((DWORD)I2C.ClkIn != (DWORD)bOne);

    if ((DWORD)I2C.ClkIn != (DWORD)bOne)
    {
	DISPDBG(("GLINTMVD: ERROR: CheckClock failed waiting for %d", (int)bOne));
	return FALSE;
    }

    return TRUE;
}

BOOL CheckData(PDEVICETABLE pDev, BOOL bOne)
{
    START_TIMEOUT;
    do
    {
	READ_I2C;
	TIMEOUT_CHECK;
    }
    while ((DWORD)I2C.DataIn != (DWORD)bOne);

    if ((DWORD)I2C.DataIn != (DWORD)bOne)
    {
	DISPDBG(("GLINTMVD: ERROR: CheckData failed waiting for %d", (int)bOne));
	return FALSE;
    }

    return TRUE;
}

void I2CStop(PDEVICETABLE pDev)
{
    DISPDBG(("GLINTMVD:  - STOP"));

    WAIT_GLINT_FIFO(4);

    I2C.Data = 0;
    WRITE_I2C;
    CHECK_DATA(0);

    I2C.Clock = 1;
    WRITE_I2C;
    CHECK_CLOCK(1);

    I2C.Data = 1;
    WRITE_I2C;
    CHECK_DATA(1);
}

void I2CInitBus(PDEVICETABLE pDev)
{
    WAIT_GLINT_FIFO(4);

    I2C.Data = 0;
    WRITE_I2C;
    CHECK_DATA(0); 

    I2C.Clock = 1;
    WRITE_I2C;
    CHECK_DATA(1);

    I2CStop(pDev);
    I2CStop(pDev);
    I2CStop(pDev);
}


void I2CStart(PDEVICETABLE pDev)
{
    DISPDBG(("GLINTMVD:  - START"));

    WAIT_GLINT_FIFO(4);

    I2C.Clock = 1;
    WRITE_I2C;
    CHECK_CLOCK(1); 

    I2C.Data = 1;
    WRITE_I2C;
    CHECK_DATA(1);

    I2C.Data = 0;
    WRITE_I2C;
    CHECK_DATA(0);

    I2C.Clock = 0;
    WRITE_I2C;
    CHECK_CLOCK(0);

    I2C.Data = 1;
    WRITE_I2C;
    CHECK_DATA(1);
}

void I2CSendBit(PDEVICETABLE pDev, BOOL bOne)
{
    DISPDBG(("GLINTMVD:  - SENDBIT"));

    WAIT_GLINT_FIFO(4);

    I2C.Data = bOne;
    WRITE_I2C;
    CHECK_DATA(bOne);

    I2C.Clock = 1;
    WRITE_I2C;
    CHECK_CLOCK(1);

    I2C.Clock = 0;
    WRITE_I2C;
    CHECK_CLOCK(0);
}

void I2CGetBit(PDEVICETABLE pDev, BOOL* pBit)
{
    DISPDBG(("GLINTMVD:  - GETBIT"));

    WAIT_GLINT_FIFO(4);

    I2C.Clock = 1;
    WRITE_I2C;
    CHECK_CLOCK(1);

    READ_I2C;

    // Get the bit
    *pBit = (BOOL)I2C.DataIn;

    I2C.Clock = 0;
    WRITE_I2C;
    CHECK_CLOCK(0);
}

BOOL I2CGetAck(PDEVICETABLE pDev)
{
    BOOL bAck = FALSE;

    DISPDBG(("GLINTMVD:  - GETACK"));

    WAIT_GLINT_FIFO(4);

    // First set the data line high
    // The Device on the bus may already have set the ACK, so 
    // don't check that the data value has been written
    I2C.Data = 1;
    WRITE_I2C;
    //CHECK_DATA(1);

    I2C.Clock = 1;
    WRITE_I2C;
    CHECK_CLOCK(1);

    DELAY;
    READ_I2C;
    bAck = CHECK_DATA(0);
    if (!bAck) 
    {
        DISPDBG(("GLINTMVD: No Ack"));
    }

    I2C.Clock = 0;
    WRITE_I2C;
    CHECK_CLOCK(0);

    // The software should do a stop if the ack fails, but
    // not here as that would cause 2 stop conditions in a row.
    if (!bAck)
    {
        return FALSE;
    }

    return TRUE;
}

// Higher level routines
void I2CSendByte(PDEVICETABLE pDev, BYTE data)
{
    int i;

    DISPDBG(("GLINTMVD:  - SENDBYTE"));

    for(i=7; i>=0; i--)
    {
        if(data & (1 << i))
            I2CSendBit(pDev, TRUE);
        else
            I2CSendBit(pDev, FALSE);
    }
}

void I2CGetByte(PDEVICETABLE pDev, BYTE* data)
{
    int 	i;
    BOOL	b;

    *data = 0x00;
    for (i = 7;  i >= 0;  i--)
    {
        I2CGetBit(pDev, &b);
        if(b)
	    *data |= (1 << i);
    }	
}


void I2CGetDataByte(PDEVICETABLE pDev, BYTE* data)
{
    DISPDBG(("GLINTMVD:  - SENDDATABYTE"));

    I2CGetByte(pDev, data);

    // Send the ACK
    I2CSendBit(pDev, TRUE);
}

BOOL I2CSendDataByte(PDEVICETABLE pDev, BYTE data)
{
    DISPDBG(("GLINTMVD:  - SENDDATABYTE"));

    I2CSendByte(pDev, data);
    if (!I2CGetAck(pDev)) 
    {
	DISPDBG(("GLINTMVD: ERROR: Acknowledge failed in SendDataByte!"));
	return FALSE;
    }
    else
    {
	DISPDBG(("GLINTMVD: Got Acknowlege!"));
    }
    return TRUE;
}

void I2CSendSeq(PDEVICETABLE pDev, WORD wUnit, BYTE num, BYTE* data)
{
    WORD i;

    I2CStart(pDev);
    if (SendSlaveAddress(pDev, wUnit, MODE_WRITE))
    {
	for(i=0; i < num; i++)
	{
	    if (!I2CSendDataByte(pDev, data[i])) break;
	}
    }
    I2CStop(pDev);                                           	
}

void I2CReceiveSeq(PDEVICETABLE pDev, WORD wUnit, BYTE StartAddress, BYTE num, BYTE* data)
{
    WORD i;

    I2CStart(pDev);
    if (SendSlaveAddress(pDev, wUnit, MODE_WRITE))
    {
	if (I2CSendDataByte(pDev, StartAddress))
	{
	    I2CStart(pDev);
	    if (SendSlaveAddress(pDev, wUnit, MODE_READ))
	    {
		for(i=0; i < num; i++)
		{
		    I2CGetDataByte(pDev, &data[i]);
		}
	    }
	}
    }
    I2CStop(pDev);                                           
}

void I2CGetDevice(PDEVICETABLE pDev, WORD wUnit, BYTE* pData)
{
    BOOL bAck;
    I2CStart(pDev);

    *pData = 0;

    // Send the slave address (might be word)
    if (((wUnit & 0xFF00) >> 8) != 0) 
    {
	bAck = I2CSendDataByte(pDev, (BYTE)((wUnit & 0xFF00) >> 8));
	if (!bAck)
	{
	    I2CStop(pDev);
	    DISPDBG(("GLINTMVD: ERROR: Could not find Device! (device is wide addressed)"));
	    return;
	}
    }

    bAck = I2CSendDataByte(pDev, (BYTE)(wUnit & 0xFF));
    if (!bAck)
    {
	I2CStop(pDev);
	DISPDBG(("GLINTMVD: ERROR: Could not find Device!"));
	return;
    }

    DISPDBG(("GLINTMVD: Device 0x%x Present", (DWORD)wUnit));

    // We got an Ack from the device
    *pData = 1;

    I2CStop(pDev);
}

/*
void DoI2C(PDEVICETABLE pDev)
{
	BYTE i;
	BYTE Ret;
	BYTE Data[5];
	DISPDBG(("GLINTMVD: ** Starting Test ****************************************************"));

	for (i = 0; i < 0xFF; i++)
	{
		Ret = 0;
		I2CGetDevice(pDev, (WORD)i, &Ret);
		if (Ret != 0)
		{
			DISPDBG(("GLINTMVD: !! FOUND DEVICE - 0x%x !!"));
		}
	}

	I2CReceiveSeq(pDev, 0x9D, 0, 1, &Data[0]);

	DISPDBG(("GLINTMVD: Read 0x%x, 0x%x", Data[0], Data[1]));

	DISPDBG(("GLINTMVD: ** Done Test     ****************************************************"));
}
*/
