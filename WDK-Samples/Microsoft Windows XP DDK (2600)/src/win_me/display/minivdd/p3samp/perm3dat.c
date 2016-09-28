
/******************************Module*Header**********************************\
 *
 *                    ***************************************
 *                    * Permedia 3: miniVDD SAMPLE CODE     *
 *                    ***************************************
 *
 * Module Name: video.c
 *
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/

#include <basedef.h>
#include <vmm.h>
#include <vmmreg.h>
#include <vwin32.h>
#include "minivdd.h"
#include "perm3.h"
#include <regdef.h>


#define NEG        0
#define POS        1


//
// This is our hybrid of DMTF modes and GTF modes which we call the 'VESA' mode
//

TIMING_INFO VESA_LIST[] =
{

//  The first entry in the list must be 640x480x60 as this is the mode NT
//  defaults to when it has somehow been set to an invalid or non-existant mode.

    {{ 640, 480, 60},    { 100,  2, 12,  6, NEG,  525,10, 2, 33, NEG,  251750}},    // DMTF

//  Don't bother for now.
//  {{ 640, 480, 72},    { 104,  3,  5, 16, NEG,  520, 9, 3, 28, NEG,  315000}},    // DMTF

    {{ 640, 480, 75},    { 105,  2,  8, 15, NEG,  500, 1, 3, 16, NEG,  315000}},    // DMTF
    {{ 640, 480, 85},    { 104,  7,  7, 10, NEG,  509, 1, 3, 25, NEG,  360000}},    // DMTF
    {{ 640, 480,100},    { 106,  5,  8, 13, NEG,  509, 1, 3, 25, NEG,  431630}},    // GTF

//           <--320 x 400-->      <--320 x 200-->     <320x400/2>
//  320x200x60 has an fH of 25kHz which is too slow for most monitors
//  {{ 320, 200, 60},    {  48,  0,  4,  4, NEG,  208, 1, 3,  4, POS,   47810}},    // GTF (hybrid of 320 x 400)
    {{ 320, 200, 75},    {  50,  1,  4,  5, NEG,  210, 1, 3,  6, POS,   62700}},    // GTF (hybrid of 320 x 400)
//  {{ 320, 200, 85},    {  52,  2,  4,  6, NEG,  211, 1, 3,  7, POS,   74435}},    // GTF (hybrid of 320 x 400)
//  {{ 320, 200,100},    {  52,  2,  4,  6, NEG,  213, 1, 3,  9, POS,   88190}},    // GTF (hybrid of 320 x 400)

//  {{ 320, 240, 60},    {  50,  1,  4,  5, NEG,  249, 1, 3,  5, POS,   59640}},    // GTF (hybrid of 320 x 480)
    {{ 320, 240, 75},    {  52,  2,  4,  6, NEG,  251, 1, 3,  7, POS,   78310}},    // GTF (hybrid of 320 x 480)
//  {{ 320, 240, 85},    {  52,  2,  4,  6, NEG,  253, 1, 3,  9, POS,   89285}},    // GTF (hybrid of 320 x 480)
//  {{ 320, 240,100},    {  52,  2,  4,  6, NEG,  255, 1, 3, 11, POS,  105870}},    // GTF (hybrid of 320 x 480)

//  DX does not work as these require a screen stride.
//  {{ 400, 300, 60},    {  64,  2,  5,  7, NEG,  311, 1, 3,  7, POS,   86580}},    // GTF (hybrid of 400 x 600)
//  {{ 400, 300, 75},    {  66,  3,  5,  8, NEG,  314, 1, 3, 10, POS,  113040}},    // GTF (hybrid of 400 x 600)
//  {{ 400, 300, 85},    {  66,  3,  5,  8, NEG,  316, 1, 3, 12, POS,  133230}},    // GTF (hybrid of 400 x 600)
//  {{ 400, 300,100},    {  66,  3,  5,  8, NEG,  319, 1, 3, 15, POS,  158220}},    // GTF (hybrid of 400 x 600)

//  512x384x60 has an fH of 24kHz which is too slow for most monitors
//  {{ 512, 384, 60},    {  78,  1,  6,  7, NEG,  398, 1, 3, 10, POS,  149010}},    // GTF
    {{ 512, 384, 75},    {  80,  2,  6,  8, NEG,  402, 1, 3, 14, POS,  192960}},    // GTF
//  {{ 512, 384, 85},    {  82,  2,  7,  9, NEG,  404, 1, 3, 16, POS,  225270}},    // GTF
//  {{ 512, 384,100},    {  82,  2,  7,  9, NEG,  407, 1, 3, 19, POS,  266990}},    // GTF

//  Don't bother for now.
//  {{ 640, 350, 85},    { 104,  4,  8, 12, POS,  445,32, 3, 60, NEG,  315000}},    // DMTF

//  {{ 640, 400, 60},    {  98,  1,  8,  9, NEG,  415, 1, 3, 11, POS,  195220}},    // GTF
    {{ 640, 400, 75},    { 100,  2,  8, 10, NEG,  418, 1, 3, 14, POS,  250800}},    // GTF
//  {{ 640, 400, 85},    { 104,  4,  8, 12, NEG,  445, 1, 3, 41, POS,  315000}},    // DMTF
//  {{ 640, 400,100},    { 104,  4,  8, 12, NEG,  424, 1, 3, 20, POS,  352770}},    // GTF

//  DX does not work as this requires a screen stride.
//  {{ 720, 400, 85},    { 104,  4,  8, 12, NEG,  446, 1, 3, 42, POS,  355000}},    // DMTF

//  Don't bother for now.
//  {{ 800, 600, 56},    { 128,  3,  9, 16, POS,  625, 1, 2, 22, POS,  360000}},    // DMTF
    {{ 800, 600, 60},    { 132,  5, 16, 11, POS,  628, 1, 4, 23, POS,  400000}},    // DMTF
//  {{ 800, 600, 72},    { 130,  7, 15,  8, POS,  666,37, 6, 23, POS,  500000}},    // DMTF
    {{ 800, 600, 75},    { 132,  2, 10, 20, POS,  625, 1, 3, 21, POS,  495000}},    // DMTF
    {{ 800, 600, 85},    { 131,  4,  8, 19, POS,  631, 1, 3, 27, POS,  562500}},    // DMTF
    {{ 800, 600,100},    { 134,  6, 11, 17, NEG,  636, 1, 3, 32, POS,  681790}},    // GTF

//  DX does not work as these require a screen stride.
//  {{ 856, 480, 60},    { 133,  2, 11, 13, NEG,  497, 1, 3, 13, POS,  317280}},    // GTF
//  {{ 856, 480, 75},    { 137,  4, 11, 15, NEG,  502, 1, 3, 18, POS,  412640}},    // GTF
//  {{ 856, 480, 85},    { 139,  5, 11, 16, NEG,  505, 1, 3, 21, POS,  477330}},    // GTF
//  {{ 856, 480,100},    { 141,  6, 11, 17, NEG,  509, 1, 3, 25, POS,  574150}},    // GTF

    {{1024, 768, 60},    { 168,  3, 17, 20, NEG,  806, 3, 6, 29, NEG,  650000}},    // DMTF
//  Don't bother for now.
//  {{1024, 768, 70},    { 166,  3, 17, 18, NEG,  806, 3, 6, 29, NEG,  750000}},    // DMTF
    {{1024, 768, 75},    { 164,  2, 12, 22, POS,  800, 1, 3, 28, POS,  787500}},    // DMTF
    {{1024, 768, 85},    { 172,  6, 12, 26, POS,  808, 1, 3, 36, POS,  945000}},    // DMTF
    {{1024, 768,100},    { 174,  9, 14, 23, NEG,  814, 1, 3, 42, POS, 1133090}},    // GTF
//    {{1024, 768,120},    { 176, 10, 14, 24, NEG,  823, 1, 3, 51, POS, 1390540}},    // GTF

    {{1152, 864, 60},    { 190,  8, 15, 23, NEG,  895, 1, 3, 27, POS,  816240}},    // GTF
    {{1152, 864, 75},    { 200,  8, 16, 32, POS,  900, 1, 3, 32, POS, 1080000}},    // DMTF
    {{1152, 864, 85},    { 194,  9, 16, 25, NEG,  907, 1, 3, 39, POS, 1196510}},    // GTF
//    {{1152, 864,100},    { 196, 10, 16, 26, NEG,  915, 1, 3, 47, POS, 1434720}},    // GTF
//    {{1152, 864,120},    { 198, 11, 16, 27, NEG,  926, 1, 3, 58, POS, 1760140}},    // GTF

    {{1280, 960, 60},    { 225, 12, 14, 39, POS, 1000, 1, 3, 36, POS, 1080000}},    // DMTF
    {{1280, 960, 75},    { 216, 11, 17, 28, NEG, 1002, 1, 3, 38, POS, 1298590}},    // GTF
    {{1280, 960, 85},    { 216,  8, 20, 28, POS, 1011, 1, 3, 47, POS, 1485000}},    // DMTF
//    {{1280, 960,100},    { 220, 12, 18, 30, NEG, 1017, 1, 3, 53, POS, 1789920}},    // GTF
//    {{1280, 960,120},    { 220, 12, 18, 30, NEG, 1029, 1, 3, 65, POS, 2173250}},    // GTF

    {{1280,1024, 60},    { 211,  6, 14, 31, POS, 1066, 1, 3, 38, POS, 1080000}},    // DMTF
    {{1280,1024, 75},    { 211,  2, 18, 31, POS, 1066, 1, 3, 38, POS, 1350000}},    // DMTF
    {{1280,1024, 85},    { 216,  8, 20, 28, POS, 1072, 1, 3, 44, POS, 1575000}},    // DMTF
//    {{1280,1024,100},    { 220, 12, 18, 30, NEG, 1085, 1, 3, 57, POS, 1909600}},    // GTF
//    {{1280,1024,120},    { 222, 13, 18, 31, NEG, 1097, 1, 3, 69, POS, 2337930}},    // GTF

    {{1600,1200, 60},    { 270,  8, 24, 38, POS, 1250, 1, 3, 46, POS, 1620000}},    // DMTF
//  Don't bother for now.
//  {{1600,1200, 65},    { 270,  8, 24, 38, POS, 1250, 1, 3, 46, POS, 1755000}},    // DMTF
//  {{1600,1200, 70},    { 270,  8, 24, 38, POS, 1250, 1, 3, 46, POS, 1890000}},    // DMTF
    {{1600,1200, 75},    { 270,  8, 24, 38, POS, 1250, 1, 3, 46, POS, 2025000}},    // DMTF
    {{1600,1200, 85},    { 270,  8, 24, 38, POS, 1250, 1, 3, 46, POS, 2295000}},    // DMTF
//    {{1600,1200,100},    { 276, 16, 22, 38, NEG, 1271, 1, 3, 67, POS, 2806370}},    // GTF

    {{1920,1080, 60},    { 322, 15, 26, 41, NEG, 1118, 1, 3, 34, POS, 1727980}},    // GTF
    {{1920,1080, 75},    { 326, 17, 26, 43, NEG, 1128, 1, 3, 44, POS, 2206370}},    // GTF
    {{1920,1080, 85},    { 328, 18, 26, 44, NEG, 1134, 1, 3, 50, POS, 2529270}},    // GTF
//    {{1920,1080,100},    { 330, 19, 26, 45, NEG, 1144, 1, 3, 60, POS, 3020160}},    // GTF

    {{1920,1200, 60},    { 324, 16, 26, 42, NEG, 1242, 1, 3, 38, POS, 1931560}},    // GTF
    {{1920,1200, 75},    { 328, 18, 26, 44, NEG, 1253, 1, 3, 49, POS, 2465900}},    // GTF
    {{1920,1200, 85},    { 330, 19, 26, 45, NEG, 1260, 1, 3, 56, POS, 2827440}},    // GTF
//    {{1920,1200,100},    { 332, 19, 27, 46, NEG, 1271, 1, 3, 67, POS, 3375780}},    // GTF
};

#undef NEG
#undef POS


#define VESA_DMTF_COUNT (sizeof(VESA_DMTF_List) / sizeof(VESA_DMTF_List[0]))
#define VESA_COUNT      (sizeof(VESA_LIST) / sizeof(VESA_LIST[0]))


/*******************************************************************************\
*
*    GetVideoTiming ()
*
*    Given a width, height and frequency this function will return a 
*    VESA timing information. 
*
*    The information is extracted from the timing definitions in the 
*    registry, if there aren't any in the registry then it looks up
*    the values in the VESA_LIST.
*
\*******************************************************************************/

BOOLEAN 
GetVideoTiming( 
    PDEVICETABLE            pDev,
    ULONG                   xRes, 
    ULONG                   yRes, 
    ULONG                   Freq, 
    ULONG                   Depth,
    VESA_TIMING_STANDARD   *VESATimings) 
{
    TIMING_INFO            *list = VESA_LIST;
    ULONG                   count = VESA_COUNT;        
    ULONG                   i;
    BOOLEAN                 retVal = FALSE;

    DISPDBG((
        "GetVideoTiming: xres %d, yres %d, freq %d, Depth %d\n", 
        xRes, yRes, Freq, Depth));

    //
    // Loop through the table looking for a match
    //

    for( i = 0; i < count; i++ ) {

        //
        // Comparewidth, height and frequency
        //

        if( list[i].basic.width == xRes  &&
            list[i].basic.height == yRes &&
            list[i].basic.refresh == Freq ) {

            //
            // We got a match
            //

            *VESATimings = list[i].vesa;
            retVal = TRUE;
            break;
        }
    }

    //
    // Fix up pixel clock, just in case it hasn't been set
    //

    if (retVal && VESATimings->pClk == 0)
    {

        DISPDBG(("GetVideoTiming: Pixel clock is zero - recalculating!"));

        VESATimings->pClk = 
            (8 * VESATimings->HTot * VESATimings->VTot * Freq) / 100;
    }

    return (retVal);
}

