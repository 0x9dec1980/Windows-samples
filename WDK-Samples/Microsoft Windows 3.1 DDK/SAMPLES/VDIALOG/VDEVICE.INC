;----------------------------- M A C R O S ------------------------------
Writel	MACRO	addr
	    push	ax
	    push	bx
	    push	cx
	    push	dx

        mov     dx,offset &addr         ;Print
        mov     cx,&addr&l   
        mov     bx,1                    ;stdout
        mov     ah,40h                  ;write
        int     21h

	    pop	    dx
	    pop	    cx
	    pop	    bx
	    pop	    ax	
	    ENDM

;----------------------------- E Q U A T E S -----------------------------

cr      equ     0dh
lf      equ     0ah

vdev_addr   equ 200h

