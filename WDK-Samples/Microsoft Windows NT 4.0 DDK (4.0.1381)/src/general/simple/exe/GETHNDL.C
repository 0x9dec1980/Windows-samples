
#include "windows.h"
#include "stdio.h"

int __cdecl main(int argc, char **argv) {


    HANDLE hTest;


    if ((hTest = CreateFile(
                     "\\\\.\\LOADTEST",
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) != ((HANDLE)-1)) {

        printf("Wow - it really worked!!!\n");

        //
        // Point proven.  Be a nice program and close up shop.
        //

        CloseHandle(hTest);

    } else {

        printf("Can't get a handle to LOADTEST\n");

    }

    return 1;

}
