#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "smpi.h"

PCHAR string = "\\\\.\\Scsi0:";
PCHAR DbgString = "This string is in the data buffer area\n";
PCHAR UniqueString = "MyDrvr";


VOID __cdecl main(VOID)
{
    BOOLEAN     bRc;
    DWORD       cbReturnedData;
    HANDLE      hDevice;
    SHORT       DbgStringLength = strlen(DbgString);
    SHORT       UniqueStringLength = strlen(UniqueString);
    SRB_BUFFER  smpi;

//*************************************************************
//
// obtain a handle to the SCSI miniport driver
//
//*************************************************************

    printf("Attempting to get a handle for %s\n",string);

    hDevice = CreateFile(string,
       GENERIC_READ,
       FILE_SHARE_READ,
       NULL,
       OPEN_EXISTING,
       0,
       NULL);

    if (hDevice == (HANDLE)-1)
       {
       printf("Attempt failed! Error number = %d\n",GetLastError());
       return;
       }

    printf("Attempt on %s succeeded\n\n",string);

//*************************************************************
//
// fill in the SRB_IO_CONTROL structure
//
//*************************************************************

    smpi.sic.HeaderLength = sizeof(SRB_IO_CONTROL);
    smpi.sic.Length = 0;

    _memccpy(&smpi.sic.Signature,UniqueString,0,UniqueStringLength);

    smpi.sic.Timeout = 10000;
    smpi.sic.ControlCode = SMP_RETURN_3F;

//*************************************************************
//
// call the driver
//
//*************************************************************

    bRc = DeviceIoControl(hDevice,                  // device handle
                IOCTL_SCSI_MINIPORT,                // io control code
                &smpi,                              // input buffer
                sizeof(SRB_IO_CONTROL),             // input buffer length
                &smpi,                              // output buffer
                sizeof(SRB_IO_CONTROL),             // output buffer length
                &cbReturnedData,                    // # bytes returned
                NULL);                              // synchronous call

//*************************************************************
//
// check for errors
//
//*************************************************************

    if (!bRc)
       printf("DeviceIoControl request for SMP_RETURN_3F failed : %d\n",
          GetLastError());

//*************************************************************
//
// verify the driver's access to the SRB_IO_CONTROL structure
//
//*************************************************************
    
    else {
       printf("The SMP_RETURN_3F Return Code is 0x%08Xh\n",
          smpi.sic.ReturnCode);
       printf("The number of bytes returned by the driver is %d\n\n",
          cbReturnedData);
       }

//*************************************************************
//
// fill in the SRB_IO_CONTROL structure
//
//*************************************************************

    smpi.sic.HeaderLength = sizeof(SRB_IO_CONTROL);
    smpi.sic.Length = 100;                       // random large number

    _memccpy(&smpi.sic.Signature,UniqueString,0,UniqueStringLength);

    smpi.sic.Timeout = 0;
    smpi.sic.ControlCode = SMP_PRINT_STRING;

    strcpy(smpi.ucDataBuffer,DbgString);

//*************************************************************
//
// call the driver
//
//*************************************************************

    bRc = DeviceIoControl(hDevice,                  // device handle
                IOCTL_SCSI_MINIPORT,                // io control code
                &smpi,                              // input buffer
                sizeof(SRB_IO_CONTROL) +            // input buffer length
                DbgStringLength + 1,
                &smpi,                              // output buffer
                sizeof(SRB_IO_CONTROL) +            // output buffer length
                100,                                // random large number
                &cbReturnedData,                    // # bytes returned
                NULL);                              // synchronous call

//*************************************************************
//
// check for errors
//
//*************************************************************

    if (!bRc)
       printf("DeviceIoControl request for SMP_PRINT_STRING failed : %d\n",
          GetLastError());

//*************************************************************
//
// verify the driver's access to the SRB_IO_CONTROL structure
//
//*************************************************************
    
    else {
       printf("%s",smpi.ucDataBuffer);
       printf("The number of bytes returned by the driver is %d\n\n",
          cbReturnedData);
       }

//*************************************************************
//
// close the handle to the SCSI miniport driver
//
//*************************************************************

    CloseHandle(hDevice);
}
