;--------------------------------------------------------------------------
; GLINTESC.INC
;
;--------------------------------------------------------------------------

GlintEscape struc
    wEscapeCode     dw      ?
    union EscParam
        lpEscapeParam   dd      ?
        ReadRegs        GlintReadRegs
        DmaReq          GlintDMARequest
    ends
GlintEscape ends

GLINT_ESCAPE            =       3d3dh
GLESC_NEWCONTEXT        =       0001h
GLESC_DELETECONTEXT     =       0002h
GLESC_GETDMABUFFERS     =       0003h
GLESC_FREEDMABUFFERS    =       0004h
GLESC_STARTDMA          =       0005h
GLESC_WAITDMA           =       0006h
GLESC_GETDMASEQUENCE    =       0007h
GLESC_SWITCHCONTEXT     =       0008h

