/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: glintfix.h
 *              Defines fixed point conversion macros for GLINT
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/


/*
** 24bit fixed point representation for color interpolation.
**
** sign     = 1 bit
** integer  = 8 bits
** fraction = 15 bits
**
** +--------+-+-----------+------------+
** + unused +S+ integer   + fraction   +
** +--------+-+-----------+------------+
*/

/*
**
** This is the real mask but I have changed it to
** 0xffffffff which allows the C code work much
** more easily (don't have to worry about shifting
** the sign bit around).  Of course this does not 
** effect the precision of the calculations.
** AJB
**
#define __GLINT_24BIT_COLOR_MASK         0x00ffffff
#define __GLINT_9BIT_TEX_INTERP_MASK     0x000001ff
**
*/
#if CHIP_SX | CHIP_TX
#define __GLINT_POS_Z_FIXED_PT_SCALE	 2147483647.0
#define __GLINT_NEG_Z_FIXED_PT_SCALE	 1073741824.0
#endif

#define __GLINT_24BIT_COLOR_MASK         0xffffffff
#define __GLINT_24BIT_FOG_MASK         	 __GLINT_24BIT_COLOR_MASK
#define __GLINT_9BIT_TEX_INTERP_MASK  	 __GLINT_24BIT_COLOR_MASK

#define __GLINT_24BIT_COLOR_FRACTION_BITS    15
#define __GLINT_24BIT_COLOR_INTEGER_BITS     8
#define __GLINT_24BIT_COLOR_SIGN_BITS        1
#define __GLINT_COLOR_FLOAT_SCALE      	     255.0

#define __GLINT_24BIT_FOG_FRACTION_BITS      22
#define __GLINT_24BIT_FOG_INTEGER_BITS       2

#define __GLINT_9BIT_TEX_INTERP_FRACTION_BITS  	 8
#define __GLINT_9BIT_TEX_INTERP_INTEGER_BITS  	 1
/*-----------------------------------------------------------------*/

#define __GLINT_16BIT_FRACTION_BITS          16
#define __GLINT_32BIT_FRACTION_BITS          32

#define __GLINT_16BIT_X_MASK   0x0000ffff
#define __GLINT_16BIT_Y_MASK   0xffff0000

#define __GLINT_RED_SHIFT   0
#define __GLINT_GREEN_SHIFT 8
#define __GLINT_BLUE_SHIFT  16
#define __GLINT_ALPHA_SHIFT 24

#define __GLINT_RED_MASK   0xFF
#define __GLINT_GREEN_MASK 0xFF00
#define __GLINT_BLUE_MASK  0xFF0000
#define __GLINT_ALPHA_MASK 0xFF000000

#define	__GLINT_COMPONENT_MASK	0xFF

/*
** Combine r, g, b & a color components into a single long.
*/
#define __GLINT_RGBA_TO_LONG(r, g, b, a)	\
  ((unsigned long)(((int)(r) & __GLINT_RED_MASK) | 	\
		(((int)(g) << __GLINT_GREEN_SHIFT) & __GLINT_GREEN_MASK) | \
		(((int)(b) << __GLINT_BLUE_SHIFT) & __GLINT_BLUE_MASK) | \
		(((int)(a) << __GLINT_ALPHA_SHIFT) & __GLINT_ALPHA_MASK)))

/* This version ensures that the bits are MS aligned. */

#define __GLINT_RED_SHIFT_8   	5
#define __GLINT_GREEN_SHIFT_8 	13
#define __GLINT_BLUE_SHIFT_8  	22
#define __GLINT_ALPHA_SHIFT_8 	24

#define __GLINT_RGBA8_TO_LONG(r, g, b, a)	\
  ((unsigned long) (				\
 		(((int)(r) << __GLINT_RED_SHIFT_8)   & __GLINT_RED_MASK) |   \
		(((int)(g) << __GLINT_GREEN_SHIFT_8) & __GLINT_GREEN_MASK) | \
		(((int)(b) << __GLINT_BLUE_SHIFT_8)  & __GLINT_BLUE_MASK) |  \
		(((int)(a) << __GLINT_ALPHA_SHIFT_8) & __GLINT_ALPHA_MASK)))

/*
** Extract components from a single RGBA long.
*/
#define __GLINT_LONG_TO_RGBA(l, r, g, b, a)	\
	r = (l >> __GLINT_RED_SHIFT) &   __GLINT_COMPONENT_MASK;	\
	g = (l >> __GLINT_GREEN_SHIFT) & __GLINT_COMPONENT_MASK;	\
	b = (l >> __GLINT_BLUE_SHIFT) &  __GLINT_COMPONENT_MASK;	\
	a = (l >> __GLINT_ALPHA_SHIFT) & __GLINT_COMPONENT_MASK;

/*
** Convert color components in the range 0.0->+/-1.0 into GLINT
** RGBA format.
*/
#define __GLINT_FLOAT_RGBA_TO_SCALED_LONG(r, g, b, a)  \
  ((unsigned long)(((int)((r)*__GLINT_COLOR_FLOAT_SCALE) & __GLINT_RED_MASK) | \
	(((int)((g)*__GLINT_COLOR_FLOAT_SCALE) << __GLINT_GREEN_SHIFT) \
						& __GLINT_GREEN_MASK) | \
	(((int)((b)*__GLINT_COLOR_FLOAT_SCALE) << __GLINT_BLUE_SHIFT) \
						& __GLINT_BLUE_MASK) | \
	(((int)((a)*__GLINT_COLOR_FLOAT_SCALE) << __GLINT_ALPHA_SHIFT) \
						& __GLINT_ALPHA_MASK)))

/*
** Convert a color component in the range 0.0->+/-1.0 into unsigned long
** ready for GLINT RGBA format.
*/
#define __GLINT_COL_TO_SCALED_ULONG(c)  \
  ((unsigned long)((int)((c)*__GLINT_COLOR_FLOAT_SCALE)))

#define __GLINT_FLOAT_R_TO_SCALED_LONG __GLINT_COL_TO_SCALED_ULONG
/*
** Convert red color component in the range 0.0->+/-1.0 into GLINT
** RGBA format.
*/
#define __GLINT_R_TO_LONG(r)  \
  ((unsigned long)(((int)(r) & __GLINT_RED_MASK)))

#define __GLINT_FLOAT_R_TO_LONG	__GLINT_R_TO_LONG

/*
** Convert a color in the range 0.0->+/-1.0 into GLINT 24bit
** fixed point format.
*/
#define __GLINT_FLOAT_TO_SCALED_24BIT_FIXED(c)  \
  ((int)(((c)*__GLINT_COLOR_FLOAT_SCALE) * \
  (1 << __GLINT_24BIT_COLOR_FRACTION_BITS)) & __GLINT_24BIT_COLOR_MASK)

/*
** Convert a color scaled according to the hardware into GLINT 24bit
** fixed point format.
*/
#define __GLINT_INDEX_TO_R_SCALED_24BIT_FIXED(c)  \
  ((int)((c) * \
  (1 << __GLINT_24BIT_COLOR_FRACTION_BITS)) & __GLINT_24BIT_COLOR_MASK)

#define __GLINT_FLOAT_TO_R_SCALED_24BIT_FIXED(c)  \
  ((int)(((c) * cfb->oneOverRedScale * __GLINT_COLOR_FLOAT_SCALE) * \
  (1 << __GLINT_24BIT_COLOR_FRACTION_BITS)) & __GLINT_24BIT_COLOR_MASK)

#define __GLINT_FLOAT_TO_G_SCALED_24BIT_FIXED(c)  \
  ((int)(((c) * cfb->oneOverGreenScale * __GLINT_COLOR_FLOAT_SCALE) * \
  (1 << __GLINT_24BIT_COLOR_FRACTION_BITS)) & __GLINT_24BIT_COLOR_MASK)

#define __GLINT_FLOAT_TO_B_SCALED_24BIT_FIXED(c)  \
  ((int)(((c) * cfb->oneOverBlueScale * __GLINT_COLOR_FLOAT_SCALE) * \
  (1 << __GLINT_24BIT_COLOR_FRACTION_BITS)) & __GLINT_24BIT_COLOR_MASK)

#define __GLINT_FLOAT_TO_A_SCALED_24BIT_FIXED(c)  \
  ((int)(((c) * cfb->oneOverAlphaScale * __GLINT_COLOR_FLOAT_SCALE) * \
  (1 << __GLINT_24BIT_COLOR_FRACTION_BITS)) & __GLINT_24BIT_COLOR_MASK)

/*
** Convert a fog value in the range 0.0->+/-1.0 into GLINT 24bit
** fixed point format.
*/
#define __GLINT_FOG_TO_24BIT_FIXED(f)  \
  ((int)((f) * (1 << __GLINT_24BIT_FOG_FRACTION_BITS)) & __GLINT_24BIT_FOG_MASK)

/*
** Convert a texture interpolation value in the range 0.0->+1.0 into GLINT 9bit
** fixed point format.
*/
#define __GLINT_TEX_INTERP_TO_9BIT_FIXED(ti)  \
  ((int)((ti) * (1 << __GLINT_9BIT_TEX_INTERP_FRACTION_BITS)) & \
	__GLINT_9BIT_TEX_INTERP_MASK)

/*-----------------------------------------------------------------*/

#define __GLINT_INT_TO_24BIT_FIXED(c) \
  (((c) * (1 << __GLINT_24BIT_COLOR_FRACTION_BITS)) & __GLINT_24BIT_COLOR_MASK)

#define __GLINT_24BIT_FIXED_TO_BYTE(c) \
  (((c) >> __GLINT_24BIT_COLOR_FRACTION_BITS) & 0xff)

#define __GLINT_24BIT_SCALED_FIXED_TO_BYTE(c) \
  (((c) >> __GLINT_24BIT_COLOR_FRACTION_BITS) & 0xff)

#define __GLINT_FLOAT_TO_16BIT_FIXED(f)  \
  ((int)((f) * (1 << __GLINT_16BIT_FRACTION_BITS)))

#define __GLINT_INT_TO_16BIT_FIXED(i)  \
  (((i) * (1 << __GLINT_16BIT_FRACTION_BITS)))

#define __GLINT_16BIT_FIXED_TO_INT(i) \
  ((i) >>__GLINT_16BIT_FRACTION_BITS)

#define	__GLINT_16BIT_FIXED_TO_FLOAT(a)	\
		((__glFloat)(a)/(__glFloat)(1 << __GLINT_16BIT_FRACTION_BITS))

#define __GLINT_XPOS_SHIFT 16
#define __GLINT_YPOS_SHIFT 0

#define __GLINT_TRUNC_16BIT_FIXED(f) \
  ((f) & ~0xffff)

#define __GLINT_ADD(a,b,c) \
 (a) = (b) + (c)
 
#define __GLINT_SUB(a,b,c) \
 (a) = (b) - (c)

#define __GLINT_FIX_ONE (0x00010000)

#define __GLINT_FLOAT_TO_XFIXED(x) ((int)((x) *__GLINT_XSUBPIXSCALE))
#define __GLINT_FLOAT_TO_YFIXED(y) ((int)((y) *__GLINT_YSUBPIXSCALE))


/* The following macro converts between float or double to fixed point,
    arranged as 1 bit sign, 32 bits int, 15 bits fract. The parameter
    types should be:
	double or float f;
	long zi;
	unsigned long zf;
*/
#if CHIP_SX | CHIP_TX
#define __GLINT_FLOAT_Z_TO_FIXED(f, zi, zf)  			\
    {								\
	double gf;						\
								\
        gf = (f) * 0.5;  /* effectively shift down */		\
        (zi) = gf;						\
        if ((f) < 0)						\
        {							\
    	    long zfneg;						\
								\
            zfneg = __GLINT_NEG_Z_FIXED_PT_SCALE *		\
				(gf - (double) (zi));		\
            (zi) += (zfneg >> 31);				\
            (zf) = zfneg << 1;   /* lose overflow bit of fraction */	\
        }							\
        else							\
            (zf) = __GLINT_POS_Z_FIXED_PT_SCALE * (gf - (double) (zi)); \
    }
#endif

/* This macro is used when the Z buffer in question is a 16 bit one. */
/* We need to scale the numbers up some more to left adjust them in  */
/* the appropriate GLINT format. */


#define __GLINT_FLOAT_Z_TO_FIXED_16(f, zi, zf)                  \
    {                                                           \
        (zi) = (((long) f) << 15);                              \
        (zf) = 0;                                               \
    }

/* Test to see if we are using a 16 bit Z buffer. */

#define	__GLINT_USING_16_BIT_ZBUFFER	\
	(gc->depthBuffer.buf.elementSize == sizeof(short))


/* The following macro converts from fixed point to float or double,
    arranged as 1 bit sign, 32 bits int, 15 bits fract. The parameter
    types should be:
	double f;
	long zi;
	unsigned long zf;
*/

#define __GLINT_FIXED_Z_TO_FLOAT(f, zi, zf)  \
   (f) = ((double)(zi) * 2.0) + ((double)(zf) / 2147483647.0L);
#if CHIP_SX || CHIP_TX
#define Z_SCALE_GLINT(i) \
	((double)i * (1 << 16))
#endif

/* The following macro converts bit position to nybble position for
   GID, FrameCount, Stencil and Depth. */

#define __GLINT_BIT_TO_NYBBLE_POSITION(b)	\
   (((b) - 16) >> 2) 

/* The following macros convert bit and byte widths to nybble widths for
   GID, FrameCount, Stencil and Depth. */

#define __GLINT_BIT_TO_NYBBLE_WIDTH(w)	\
   ((w) >> 2) 

