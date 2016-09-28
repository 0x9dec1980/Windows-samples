;/******************************Module*Header**********************************\
; *
; *                           ***************************
; *                           * Permedia 2: SAMPLE CODE *
; *                           ***************************
; *
; * Module Name: TEXT32.ASM
; *
; *
; * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
; * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
; *
;\*****************************************************************************/

	.286c
	.xlist
	DOS5 = 1			;so we don't get INC BP in <cBegin>
incDrawMode = 1
incFont = 1
CODE32BPP = 1

include glint.inc
include	gdidefs.inc

GLYPHINFO STRUCT
    CacheOffset     dd  0       ; Mem: Offset of font within the font cache
                                ; OST: Texel address of start of char in memory
    CacheSize       dd  0       ; Mem: Number of words of data in font
                                ; OST: Format of character
    CachedOffsets   dd  0       ; yOffset << 16 | xOffset
    CachedWidth     dd  0       ; Mem: 16.16 format width of char
                                ; OST: Height << 16 | Width
GLYPHINFO ENDS
GLYPHINFOSHIFT = 4              ; Size = 16, so shift of 4 avoids mul

FONTCACHE STRUCT
    LastAccess      dd  0       ; Access ID of last access to this font (for font expiry)
    FontID          dd  0       ; Unique ID of the font
    FontType        dd  0       ; Type of this font. 0=Unused, 1=New, 2=Old
    nGlyphs         dd  0       ; Number of Glyphs in array
    Glyphs          dd  0       ; Offset of Glyph array in FontCache segment
    OSTChunkIndex   dd  0       ; Chunk index of first memory block for OST fonts
    OSTChunkFree    dd  0       ; Byte Address of next free space in chunk
    OSTChunkEnd     dd  0       ; Byte Address just beyond free space
FONTCACHE ENDS
FONTCACHESHIFT = 5              ; Size = 32, so shift of 5 avoids mul

N_CACHED_FONTS = 64

UNUSED_FONT = 0
NEW_FONT    = 1
OLD_FONT    = 2

; Some defines for offscreen chunks.
MAX_CHUNK_INDEX     = 256           ; Includes marker indexes
CHUNK_UNUSED        = 0             ; Marker for unused chunk
CHUNK_FIRST_INDEX   = 1             ; First usable chunk index
CHUNK_LAST_INDEX    = 254           ; Marker for unused chunk
CHUNK_END           = 255           ; Marker for end of chain of chunks
MAX_USABLE_CHUNKS   = CHUNK_LAST_INDEX - CHUNK_FIRST_INDEX + 1

;GLYPH_INDEX     = 0			;the first 4 of these are not cached - since their values
;GLYPH_XDOM      = 4			;vary with char position, except GLYPH_INDEX which is
;GLYPH_XSUB      = 8			;a true constant - so it's not cached either.
;GLYPH_Y         = 12
GLYPH_COUNT     = 0
GLYPH_RENDER    = 4
GLYPH_HOLD      = 8

GLYPH_COMMAND_HEADER = 12
GLYPH_DMA_LIMIT = 4

	.list
;----------------------------------------------------------------------------
; D A T A
;----------------------------------------------------------------------------
_TEXT segment
	assumes cs,_TEXT
_TEXT ends

.486

_INIT segment 

    globalD TextFn, ETOBlockFill
    globalD OpaqueBltCmd, 48h
	globalD	MemTextCmd, 0848h
	globalD	OSTTextCmd, 0848h
	globalD dropDoubles, 0

    globalD FreeHeapOffset, FONT_CACHE_OFFSET
    globalD CurrentAccess, 0
    globalD LastFontID, 0ffffffffh 
    globalD LastFontpFC, 0

    ; OS Chunk management. ChunkOffset + ChunkIndex * Size = byte offset of chunk start
    ; Index 0 is free space, so ChunkOffset is one chunk before the actual space.
    globalD ChunkOffset, 0
    globalD ChunkSize, 0
    globalD ChunkShift, 0
    globalD ChunkCount, 0
    globalD LastAllocatedChunk, CHUNK_FIRST_INDEX-1

    globalD dwTextureMapFormat, 0ffffffffh
    globalD dwRectangleSize, 0ffffffffh

BytesToTextureMapFormat dd  0
                        dd  P_8BIT_TEXELS
                        dd  P_16BIT_TEXELS
                        dd  P_24BIT_TEXELS
                        dd  P_32BIT_TEXELS

Fast7	dd	FastDL0, FastDL1, FastDL2, FastDL3, FastDL4, FastDL5, FastDL6, FastDL7

;   Array of cached fonts
FontCache       FONTCACHE   N_CACHED_FONTS dup (<?>)

	globalB	boldByHand, 0
	globalB	dropThis, 0

_INIT ends


_TEXT32 segment


;------------------------------------------------------------------------------
;
;   InitialiseFontCache
;       Set up any information needed to cache the fonts
;
;------------------------------------------------------------------------------
	BEST_ALIGN
PLABEL InitialiseFontCacheFromC
; Extend return address to DWORD:DWORD as C compiler pushes WORD:WORD
    xor     eax, eax
    pop     ax
    xor     ebx, ebx
    pop     bx
    push    ebx
    push    eax
InitialiseFontCache PROC FAR PASCAL PUBLIC USES ds, CacheMode:DWORD

; Fix up base pointer and stack pointer as we were called from a 16 bit segment
    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    eax, RemappedDataSegment
    and     ebp, 0ffffh
    mov     ds, eax

	assumes ds,_INIT
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs,nothing

    mov     eax, GLInfo.dwFontCache16
    test    eax, eax                            ; Have we got a font cache?
    je      InitialiseFontCacheDone

    ; Initialise all the font cache pointers to be invalid
    xor     eax, eax
    mov     ecx, N_CACHED_FONTS
    lea     ebx, FontCache
@@:
    mov     [ebx].Glyphs, eax 
    mov     [ebx].FontID, eax 
    mov     [ebx].nGlyphs, eax 
    mov     [ebx].LastAccess, eax 
    mov     [ebx].OSTChunkIndex, eax 
    mov     [ebx].OSTChunkFree , eax 
    mov     [ebx].OSTChunkEnd  , eax 
    mov     [ebx].FontType, UNUSED_FONT
    add     ebx, SIZE FONTCACHE                 ; Move onto next font
    dec     ecx
    jne     @b

    mov     CurrentAccess, eax
    mov     LastFontpFC,eax
    mov     LastFontID, 0ffffffffh
    mov     ChunkCount, eax                     ; Used to determine of OST is enabled. 

	cmp		CacheMode, TEXT_CACHED_IN_OFFSCREEN
	je		IFC_OffscreenInit
	cmp		CacheMode, TEXT_CACHED_IN_MEMORY
	je		IFC_MemoryInit
ifdef DEBUG
    int     1
endif
	jmp		InitialiseFontCacheDone

IFC_MemoryInit:
    ; Initialise command registers
    mov     OpaqueBltCmd, 48h
	mov    	MemTextCmd, 0848h

	; Set up the cache records
    mov     edi, FONT_CACHE_OFFSET 
    mov     ebx, GLInfo.dwFontCacheSize
    shl     ebx, 12								; Convert size to bytes
    mov     DWORD PTR ds:[edi], ebx			    ; Store size of free block
    mov     DWORD PTR ds:[edi+4], 0ffffffffh	; store pointer to next block
    mov     FreeHeapOffset, edi

	jmp		InitialiseFontCacheDone

IFC_OffscreenInit:

    mov     ebx, GLInfo.ddFBSize
    sub     ebx, GLInfo.dwOSFontCacheStart
    jle     IFC_MemoryInit                      ; If we have no cache, goto memory caching

    ; Offscreen base Font Cache is allocated to a font in chunks. We allow up to 252
    ; chunks to be allocated, and each chunk has a byte identifier which indicates the
    ; next chunk of this particular fonts data. These 252 byte identifiers are located
    ; at the start of the Memory Cache. Following the 252 byte identifiers is the
    ; font information, similar to the Memory Cache method. 


	; Set up the OS Chunk records. All are set to unused (0)
    mov     edi, FONT_CACHE_OFFSET
    mov     ecx, MAX_CHUNK_INDEX
    xor     eax, eax                            ; Unused      
@@:
    mov     [edi], eax
    add     edi, 4
    sub     ecx, 4
    jge     @b

    ; ebx is size of cache. Minimum chunk size of 512. Start at 256 and double at
    ; first loop
    mov     ecx, 8                              ; Start size of 256 -> shift of 10
    shr     ebx, cl                             ; number of chunks at 256 bytes
@@:
    inc     ecx                                 ; double chunk size
    shr     ebx, 1                              ; halve number of chunks
    cmp     ebx, MAX_USABLE_CHUNKS              ; Can we fit into space?
    ja      @b

    mov     eax, 1
    mov     edx, GLInfo.dwOSFontCacheStart
    shl     eax, cl                             ; Work out chunk size
    mov     ChunkShift, ecx
    sub     edx, eax                            ; Subtract size from offset
    mov     ChunkCount, ebx                     ; Actual number of chunks
    mov     ChunkSize, eax                      ; Size of chunks
    mov     ChunkOffset, edx                    ; Offset to add on after Index << Shift

    mov     eax, GLInfo.dwFontCacheSize
    shl     eax, 12                             ; Convert size to bytes
    mov     edi, FONT_CACHE_OFFSET + MAX_CHUNK_INDEX 
    sub     eax, MAX_CHUNK_INDEX                ; Remove MAX_CHUNK_INDEX bytes
    mov     DWORD PTR ds:[edi], eax		        ; Store size of free block
    mov     DWORD PTR ds:[edi+4], 0ffffffffh	; store pointer to next block
    mov     FreeHeapOffset, edi

    ; Initialise command registers
    mov     OpaqueBltCmd, 48h
	mov    	MemTextCmd, 0848h
	mov    	OSTTextCmd, 6020C8h                    ; RECT, +X +Y +Texture +BlockFill

	jmp		InitialiseFontCacheDone

InitialiseFontCacheDone:
    ret
InitialiseFontCache ENDP

ValidateHeap PROC NEAR C USES ds gs eax ebx esi edi
    and ebp, 0ffffh
    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    eax,RemappedDataSegment
	mov	    ds,eax
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs,nothing

    mov     eax, GLInfo.dwFontCache16
    test    eax, eax                        ; Have we got a font cache?
    je      ValidateHeapDone

    mov     ebx, GLInfo.dwFontCacheSize     ; get cache size
    shl     ebx, 12                         ; Convert size to bytes

    mov     esi, FreeHeapOffset             ; Get offset of first free area of the heap
ValidateHeapNext:
    cmp     esi, 0ffffffffh
    je      ValidateHeapDone
    cmp     esi, ebx                        ; esi > size of heap?
    jae     ValidateHeapDie
    mov     eax, ds:[esi]                   ; get size
    mov     edi, ds:[esi+4]                 ; get next
    test    esi, 3                          ; bottom bits of this pointer set?
    jne     ValidateHeapDie
    test    eax, 3                          ; bottom bits of size set?
    jne     ValidateHeapDie
    cmp     esi, edi                        ; Current > than next?
    jae     ValidateHeapDie
    add     eax, esi
    cmp     eax, edi                        ; end of current > next?
    jae     ValidateHeapDie                 
    mov     esi, edi
    jmp     ValidateHeapNext
ValidateHeapDie:
    int     1
ValidateHeapDone:
    ret
ValidateHeap ENDP


;------------------------------------------------------------------------------
;
;   AllocateHeap
;       Allocate the specified number of bytes from the Font Cache heap.
;
;------------------------------------------------------------------------------
	BEST_ALIGN
AllocateHeap PROC NEAR C SizeToAllocate:DWORD
    and ebp, 0ffffh
	assumes ds,_INIT
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs,nothing                      ; Font Cache segment
        
    mov     ebx, 0ffffffffh                 ; Last Offset = 0xffffffff
    mov     eax, FreeHeapOffset             ; Get offset of first free area of the heap
    cmp     eax, ebx                        ; Are we at the end of our heap list?
    mov     ecx, SizeToAllocate             ; Get number of bytes to allocate
    je      AH_EndOfFreeHeap
    
AH_NextFreeHeapArea:
    mov     edx, DWORD PTR ds:[eax]         ; Get the size of the free heap area
    cmp     edx, ecx                        ; Is the size of the heap large enough?
    jb      AH_TryNextHeapArea              ; Try next if not
    je      AH_HeapExactlyCorrectSize       ; If we are exactly the same size, use area

    sub     edx, 8                          ; Subtract 8 (size of a free record)
    cmp     edx, ecx                        ; Have we got enough size to split?
    jae     AH_HeapLargeEnoughToSplit       ; Split if so
    
AH_TryNextHeapArea:
    mov     ebx, eax                        ; remember new last offset
    mov     eax, ds:[eax+4]                 ; Get offset of next area
    cmp     eax, 0ffffffffh                 ; Are we at the end of our heap list?
    jne     AH_NextFreeHeapArea             ; loop if not
AH_EndOfFreeHeap:                           ; eax = 0xffffffff
ifdef DEBUG
    jmp     AH_Exit
else
    ret				                        ; return Offset in eax.
endif

	BEST_ALIGN
AH_HeapExactlyCorrectSize:                  ; remove area from list
    mov     edx, DWORD PTR ds:[eax+4]       ; Get offset of next area
    cmp     ebx, 0ffffffffh                 ; Was the last the FreeHeapOffset?
    jne     @f
    mov     FreeHeapOffset, edx             ; remove element from head of list
ifdef DEBUG
    jmp     AH_Exit
else
    ret				                        ; return Offset in eax.
endif

	BEST_ALIGN
@@:
    mov     DWORD PTR ds:[ebx + 4], edx     ; remove element from the middle of list
ifdef DEBUG
    jmp     AH_Exit
else
    ret				                        ; return Offset in eax.
endif

	BEST_ALIGN
AH_HeapLargeEnoughToSplit:
    add     edx, 8                          ; Add on the 8 we subtracted earlier
    sub     edx, ecx                        ; subtract area to be allocated
    mov     DWORD PTR ds:[eax], edx         ; update the size of the region
    add     eax, edx                        ; return offset of allocated area
AH_Exit:
ifdef DEBUG
    cmp     eax, 0ffffffffh
    je      AH_DebugFailure
    cmp     eax, FONT_CACHE_OFFSET
    jb      @f
    cmp     eax, FONT_CACHE_OFFSET * 2
    jb      AH_DebugInRange
@@:
    int     1 
AH_DebugInRange:
    mov     ecx, SizeToAllocate
    mov     ebx, [eax]
    mov     ebx, [eax + ecx - 4]
AH_DebugFailure:
endif
    ret
AllocateHeap ENDP

;------------------------------------------------------------------------------
;
;   FreeHeap
;       Free the specified area of font cache
;
;------------------------------------------------------------------------------
	BEST_ALIGN
FreeHeap PROC NEAR C USES esi, OffsetToFree:DWORD, SizeToFree:DWORD
    and ebp, 0ffffh
	assumes ds,_INIT
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs,nothing                       
        
ifdef DEBUG
    mov     edx, OffsetToFree               ; Get offset to free
    mov     ecx, SizeToFree                 ; Get number of bytes to free
    cmp     edx, FONT_CACHE_OFFSET
    jb      @f
    cmp     edx, FONT_CACHE_OFFSET * 2
    jb      FH_InRange
@@:
    int     1 
FH_InRange:
    mov     ebx, [edx]
    mov     ebx, [edx + ecx - 4]
endif
    mov     eax, FreeHeapOffset             ; Get offset of first free area of the heap
    mov     ebx, 0ffffffffh                 ; Last Offset
    mov     ecx, SizeToFree                 ; Get number of bytes to free
    mov     edx, OffsetToFree               ; Get offset to free
    cmp     eax, ebx                        ; Are we at the end of our heap list?
    je      FH_EndOfFreeHeap
FH_NextFreeHeapArea:
    cmp     eax, edx
    ja      FH_OffsetAboveOffsetToFree

    mov     esi, DWORD PTR ds:[eax]         ; Get the size of this chunk
    add     esi, eax                        ; work out the final address of the chunk
    cmp     esi, edx                        ; Is final address the same as OffsetToFree?
    je      FH_AddToPreviousArea

    mov     ebx, eax                        ; remember new last offset
    mov     eax, DWORD PTR  ds:[eax+4]      ; Get offset of next area
    cmp     eax, 0ffffffffh                 ; Are we at the end of our heap list?
    jne     FH_NextFreeHeapArea             ; loop if not
FH_EndOfFreeHeap:                           ; Add space to the end of the list
                                            ; eax = 0xffffffff
    mov     DWORD PTR ds:[edx], ecx         ; Write size of area into new record
    mov     DWORD PTR ds:[edx+4], eax       ; Write end of list into new record
    cmp     ebx, eax                        ; Is this the first offset?
    je      @f                              ; Jump if so
    mov     DWORD PTR ds:[ebx+4], edx       ; add record into the list
    jmp     FH_Exit
@@:
    mov     FreeHeapOffset, edx
    jmp     FH_Exit

	BEST_ALIGN
FH_AddToPreviousArea:                       ; Concatenate onto the end of previous record
    mov     ebx, DWORD PTR ds:[eax + 4]     ; Get offset of next area
    sub     esi, eax                        ; Subtract the Offset again. Should be size now.
    add     esi, ecx                        ; Add in size of area free'd
    mov     DWORD PTR ds:[eax], esi         ; Write out new size of area
    add     esi, eax                        ; Work out end of area offset
    cmp     esi, ebx                        ; Have we bridged a gap?
    jne     FH_Exit                         ; Exit if we havnt
    mov     ecx, DWORD PTR ds:[ebx]         ; Get the size of the next region
    sub     esi, eax                        ; Revert offset to size again
    mov     edx, DWORD PTR ds:[ebx+4]       ; Get the offset of the following region
    add     esi, ecx                        ; Work out the size of the new area
    mov     DWORD PTR ds:[eax+4], edx       ; Write out offset to next region
    mov     DWORD PTR ds:[eax], esi         ; Write out new length
    jmp     FH_Exit

	BEST_ALIGN
FH_OffsetAboveOffsetToFree:                 ; Either we have to prepend the free space or add a new record
    cmp     ebx, 0ffffffffh
    jne     @f
    mov     FreeHeapOffset, edx             ; Point first record at our new record
    jmp     FH_NewRecordInserted
	BEST_ALIGN
@@:
    mov     DWORD PTR ds:[ebx+4], edx       ; Insert the new record
FH_NewRecordInserted:
    mov     esi, edx
    add     esi, ecx                        ; esi = OffsetToFree + SizeToFree
    cmp     esi, eax                        ; do we need to prepend?
    je      FH_PrependArea
    mov     DWORD PTR ds:[edx], ecx
    mov     DWORD PTR ds:[edx+4], eax
    jmp     FH_Exit

	BEST_ALIGN
FH_PrependArea:
    mov     ebx, DWORD PTR ds:[eax]         ; Get size of next region
    mov     esi, DWORD PTR ds:[eax+4]       ; Get offset to the next region
    add     ebx, ecx                        ; Add size of this region
    mov     DWORD PTR ds:[edx+4], esi       ; Point this region at next
    mov     DWORD PTR ds:[edx], ebx         ; Update size of this area
FH_Exit:
    ret
FreeHeap ENDP

;------------------------------------------------------------------------------
;
;   RemoveCachedFont
;       Remove a font from the cache and return the address of the freed PFONTCACHE
;
;------------------------------------------------------------------------------
	BEST_ALIGN
RemoveCachedFont PROC NEAR C USES ds es gs esi edi
    and ebp, 0ffffh
    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    eax,RemappedDataSegment
	mov	    ds,eax
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs,nothing                  

    mov     eax, CurrentAccess			; eax = CurrentAccess
    lea     esi, FontCache
    mov     ecx, N_CACHED_FONTS
    mov     edx, eax				    ; edx = Earliest Access
    xor     edi, edi					; edi = pFreeFC

RCF_CheckNextFont:

    cmp     [esi].FontType, UNUSED_FONT ; Is this font cache entry used?
    je      RCF_TryNextFont
    mov     ebx, [esi].LastAccess       ; Get last access ID
    cmp     ebx, eax                    ; compare with current access                    
    jle     @f                          ; if its lower, all is well.
    xor     ebx, ebx                    ; if its higher, we must have wrapped
    mov     [esi].LastAccess, ebx       ; zero the last access
@@:
    cmp     ebx, edx                    ; Compare last access with earliest access
    jg      @f                          ; if its > go try next font, 
    mov     edx, ebx                    ; Update earliest access
    mov     edi, esi                    ; remember pointer
@@:
RCF_TryNextFont:
    add     esi, SIZE FONTCACHE
    dec     cx
    jne     RCF_CheckNextFont

	or		edi, edi					; Did we find something to free?
	je		RCF_Failed					; if we didnt, fail call

RCF_FreeFont:
    push    edi                         ; Save pFreeFC for return
    mov     esi, [edi].Glyphs           ; Get the glyph Offset
    mov     ecx, [edi].nGlyphs          ; get glyph count 
    or      ecx, ecx                    ; Are there any glyphs?
    je      RCF_NoGlyphData
    test    ecx, 080000000h             ; Are there memory glyphs?
    je      RCF_NoGlyphData
    and     ecx, 07fffffffh
    
    xor     ebx, ebx
    push    edi                         ; Save pFreeFC for later
    dec     ebx                         ; ebx = 0ffffffffh
RCF_NextGlyph:
    mov     eax, ds:[esi].CacheOffset
    cmp     ebx, eax
    je      RCF_TryNextGlyph

    mov     edx, ds:[esi].CachedWidth
    test    edx, 080000000h
    je      RCF_TryNextGlyph

    mov     ds:[esi].CacheOffset, ebx   ; Cache Offset = 0xffffffff
    push    esi
    push    ecx
    push    ebx
    mov     edi,ds:[esi].CacheSize		; in words
	shl		edi, 2						; convert to bytes
    invoke  FreeHeap, eax, edi
    pop     ebx
    pop     ecx
    pop     esi
RCF_TryNextGlyph:
    add     esi, SIZE GLYPHINFO
    dec     ecx
    jne     RCF_NextGlyph
    pop     edi                          ; Restore pFreeFC

RCF_NoGlyphData:
    mov     esi, FONT_CACHE_OFFSET
    mov     eax, [edi].OSTChunkIndex
    test    eax, eax
    je      RCFOST_NoChunks              ; Any chunks to free?
; Free all the glyph info - just traverse the chunk list freeing each chunk
RCFOST_NextChunk:
    mov     ecx, eax
    mov     al, ds:[esi+ecx]
ifdef DEBUG
    test    al, al
    jne     @f
    int 1
@@:
endif
    mov     BYTE PTR ds:[esi+ecx], CHUNK_UNUSED
    cmp     al, CHUNK_END
    jne     RCFOST_NextChunk
    mov     [edi].OSTChunkIndex, 0

    ; traverse the glyph array freeing each glyph pointer if we are not
    ; going to free the glyph array in the pFC
    cmp     edi, LastFontpFC
    jne     RCFOST_NoChunks

    xor     eax, eax
	mov		ecx, [edi].nGlyphs			; Get the number of glyphs
    and     ecx, 07fffffffh             ; remove memory glyph flag
    dec     eax                         ; eax = 0ffffffffh
    mov     esi, [edi].Glyphs           ; Get the glyph Offset
    test    ecx, ecx
    je      RCFOST_NoChunks
@@:
    mov     ds:[esi].CacheOffset, eax
    add     esi, SIZE GLYPHINFO
    dec     ecx
    jnz     @b

RCFOST_NoChunks:

    ; We dont want to deallocate the current FC's, despite having deallocated
    ; the cached font info
    cmp     edi, LastFontpFC
    je      @f
    mov     [edi].FontType, UNUSED_FONT

    mov     esi, [edi].Glyphs           ; Get the glyph Offset
	mov		eax, [edi].nGlyphs			; Get the number of glyphs
	shl		eax, GLYPHINFOSHIFT			; work out how much we allocated
    invoke  FreeHeap, esi, eax			; free up the space
@@:
    pop     eax                         ; return pFreeFC
    ret

	BEST_ALIGN
RCF_Failed:
	xor		eax, eax					; return NULL
	ret
RemoveCachedFont ENDP

;------------------------------------------------------------------------------
;
;   AllocateOST
;       Allocate a chunk of OST. If ChainIndex == CHUNK_UNUSED, then this
;       is the first chunk in a block. If not, then it will be appended to the
;       end of the chain specified.
;
;------------------------------------------------------------------------------
	BEST_ALIGN
AllocateOST PROC NEAR C USES esi edi, pFC:DWORD
    and ebp, 0ffffh
	assumes ds,_INIT
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs,nothing                       

    mov     ecx, LastAllocatedChunk
    mov     esi, FONT_CACHE_OFFSET
    mov     edi, pFC

AOST_TryNextIndexAfterLast:
    inc     ecx
    cmp     ecx, ChunkCount
    ja      AOST_WrapChunkIndex
    mov     al, ds:[esi+ecx]
    test    al, al
    jne     AOST_TryNextIndexAfterLast
AOST_FoundChunkIndex:
    mov     BYTE PTR ds:[esi+ecx], CHUNK_END
    mov     LastAllocatedChunk, ecx
    test    edi, edi                        ; Do we need to update a pFC?
    je      AOST_Exit
    mov     eax, ds:[edi].OSTChunkIndex     ; Index will be zero if this is first call
    test    eax, eax                        ; with this font or if we free'd it.
    jne     @f
    mov     ds:[edi].OSTChunkIndex, ecx     ; if we did, update pFC with new chunk
    jmp     AOST_Exit
@@:
    mov     ebx, eax                        ; add new chunk to end of chunk list
    mov     al, ds:[esi + eax]
    cmp     al, CHUNK_END
    jne     @b
    mov     ds:[esi + ebx], cl              ; Add to end of chain
AOST_Exit:
    mov     eax, ecx                        ; return chunk index allocated 
    ret

AOST_WrapChunkIndex:
    mov     ecx, CHUNK_FIRST_INDEX-1
    mov     ebx, LastAllocatedChunk
AOST_TryNextIndexBeforeLast:
    inc     ecx
    cmp     ecx, ebx
    je      AOST_NoFreeChunkIndex
    mov     al, ds:[esi+ecx]
    test    al, al
    jne     AOST_TryNextIndexBeforeLast
    jmp     AOST_FoundChunkIndex

AOST_NoFreeChunkIndex:
    invoke  RemoveCachedFont
    test    eax, eax   
    je      AOST_NoFreeChunkIndex
    mov     ecx, CHUNK_FIRST_INDEX-1        ; Start from scratch
    jmp     AOST_TryNextIndexAfterLast

AllocateOST ENDP

;------------------------------------------------------------------------------
;
;   FindFontCache
;       Return the PFONTCACHE for the required font
;
;------------------------------------------------------------------------------
	BEST_ALIGN

FindFontCache PROC NEAR C, findFontType:DWORD, UniqueID:DWORD, lpFontInfo:DWORD
    and ebp, 0ffffh
	assumes ds,_INIT
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs,nothing

    lea     eax, FontCache
    mov     ecx, N_CACHED_FONTS
    mov     ebx, UniqueID
    mov     edx, findFontType
FFC_CheckNextFontCache:
    cmp     [eax].FontType, edx
    jne     FFC_TryNextFontCache
    cmp     [eax].FontID, ebx
    je      FFC_FoundFontCache
FFC_TryNextFontCache:
    add     eax, SIZE FONTCACHE
    dec     ecx
    jne     FFC_CheckNextFontCache

    lea     eax, FontCache
    mov     ecx, N_CACHED_FONTS
FFC_CheckNextUnusedFontCache:
    cmp     [eax].FontType, UNUSED_FONT
    je      FFC_FoundUnusedFontCache
    add     eax, SIZE FONTCACHE
    dec     ecx
    jne     FFC_CheckNextUnusedFontCache
    ; No unused FontCache. Free one
    invoke  RemoveCachedFont
	test	eax, eax					; Did we remove something?
	je		FFC_Failed					; Nothing to remove, so fail!
	cmp		[eax].FontType, UNUSED_FONT	; Have we been returned an unused font?
	jne		FFC_Failed					; fail if we havnt
    jmp     FFC_FoundUnusedFontCache

	BEST_ALIGN
FFC_FoundFontCache:                     ; eax = pFreeFC, edx = FontType, ebx = ID
    mov     LastFontID, ebx
    mov     LastFontpFC, eax
    ret

FFC_FoundUnusedFontCache:               ; eax = pFreeFC

	; Determine if we can actually cache the font
    les     di, lpFontInfo              ; es:[edi] = font info
    xor     edi, edi                    ; newfontseg data starts at zero!
	xor		ebx, ebx
    mov     bx, es:[edi].nfNumGlyphs    ; Get the number of Glyphs
	mov		[eax].nGlyphs, ebx			; save the number of glyphs	in the pFC
	shl		ebx, GLYPHINFOSHIFT			; work out the number of bytes to allocate
    mov     ecx, GLInfo.dwFontCacheSize	; get the number of pages of font cache
    shl     ecx, 10                     ; Convert size to bytes and quarter it
	cmp		ebx, ecx					; we will fail to cache any font whose glyph data
	ja		FFC_Failed					; takes up > 1/4 of our cache

	push	eax							; save pFC
FFC_AllocateHeap:
	invoke	AllocateHeap, ebx
	cmp		eax, 0ffffffffh				; Did we get the allocation?
	jne		FFC_HeapAllocated			; Get out of loop if we did.

    invoke  RemoveCachedFont            ; Free up some space
	pop		ebx							; Get pFC temporarily
	push	ebx							; put it back again
	mov		ebx, [ebx].nGlyphs			; get the number of glyphs in the pFC
	shl		ebx, GLYPHINFOSHIFT			; work out the number of bytes to allocate
	test	eax, eax					; Did we manage to free something?
	jne		FFC_AllocateHeap			; if we did, try to allocate again
	pop		eax
	jmp		FFC_Failed					; If not, fail

	BEST_ALIGN
FFC_HeapAllocated:
	pop		esi							; restore pFC
	mov		[esi].Glyphs, eax			; Store heap offset
    mov     ecx, UniqueID
    mov     edx, findFontType
    mov     [esi].FontID, ecx
    mov     [esi].FontType, edx

    ; Update last font cache info
    mov     edx, CurrentAccess
    mov     LastFontpFC, esi
    mov     LastFontID, ecx
    mov     [esi].LastAccess, edx

	; initialise the Glyph array
    mov     ecx, [esi].nGlyphs          ; Get nGlyphs again
    mov     ebx, 0ffffffffh
@@:
    mov     ds:[eax].CacheOffset, ebx   ; nulify cache offset
    add     eax, SIZE GLYPHINFO         ; Move to next Glyph
    dec     ecx
    jne     @b                          ; Loop

    mov     ecx, ChunkCount
    test    ecx, ecx
    je      FFC_MemoryFont

; Initialise the OST fields
    invoke  AllocateOST, 0              ; Alloc block without worrying about a pFC
    mov     [esi].OSTChunkIndex, eax
    mov     ecx, ChunkShift
    shl     eax, cl                     ; Work out address of chunk from chunk 0
    mov     ebx, ChunkSize
    add     eax, ChunkOffset            ; Work out byte offset of chunk start
    add     ebx, eax
    mov     [esi].OSTChunkFree, eax
    mov     [esi].OSTChunkEnd, ebx

FFC_MemoryFont:
    mov     ebx, UniqueID
    mov     eax, esi                    ; return pFC
	jmp     FFC_FoundFontCache

	BEST_ALIGN
FFC_Failed:
	xor		eax, eax					; return NULL
	ret
FindFontCache ENDP

;------------------------------------------------------------------------------
;
;   LoadMemChar
;       Load a new format character into the font cache
;
;------------------------------------------------------------------------------
	BEST_ALIGN
LoadMemChar PROC NEAR C USES es pNewFontSeg:DWORD, pFC:DWORD, pGlyph:DWORD, Char:DWORD
    LOCAL   sWidth:DWORD, dWidth:DWORD, sHeight:DWORD, pSrcBitmap:DWORD
	and		ebp, 0ffffh
	assumes ds,_INIT
	assumes	es,nothing
	assumes fs,nothing
	assumes	gs,nothing

    les     di, pNewFontSeg     
    xor     edx, edx                    ; Newfontseg starts at zero
    mov     edi, pGlyph                 ; Get glyph offset in edi
    xor     ecx, ecx                    ; zero top bits of ecx
    mov     ebx, Char

    ; First we need to decide how much data space to allocate from the DMA buffer
    test    es:[edx].nfFormat, NF_LARGE
    je      LMC_SMALLROWGLYPH
    mov     esi, es:[edx].nfGlyphOffset
    mov     esi, es:[esi + ebx*4]
    mov     cx, es:[esi].lrgOrgX
    mov     ax, es:[esi].lrgOrgY
    shl     eax, 16
    or      eax, ecx
    mov     ds:[edi].CachedOffsets, eax
    movsx   ecx, es:[esi].lrgWidth
    mov     eax, ecx
    shl     eax, 16
    or      eax, 080000000h
    mov     ds:[edi].CachedWidth, eax
    movsx   eax, es:[esi].lrgHeight
    add     esi, SIZE LARGEROWGLYPH
    jmp     LMC_GotGlyphInfo

	BEST_ALIGN
LMC_SMALLROWGLYPH:
    mov     ecx, es:[edx].nfGlyphOffset
    xor     esi, esi
    mov     si, es:[ecx + ebx*2]
    movsx   cx, es:[esi].srgOrgX
    movsx   eax, es:[esi].srgOrgY
    shl     eax, 16
    or      eax, ecx
    mov     ds:[edi].CachedOffsets, eax
    movsx   ecx, es:[esi].srgWidth
    mov     eax, ecx
    shl     eax, 16
    or      eax, 080000000h
    mov     ds:[edi].CachedWidth, eax
    movsx   eax, es:[esi].srgHeight
    add     esi, SIZE SMALLROWGLYPH
LMC_GotGlyphInfo:
    add     ecx, 7                      ; turn width into bytes (of source)
    shr     ecx, 3
    mov     sWidth, ecx
    add     ecx, 3                      ; Work out width in bytes of dest
    and     ecx, NOT 3
    mov     dWidth, ecx

; We want to remove any zero scanlines from the bitmap to decrease the number of words to
; store and to download
	test	eax, eax
	je		LMC_ZeroReductionAllDone
	mov		ecx, sWidth
    mov     edx, ds:[edi].CachedOffsets	; edx = Cached Offsets
LMC_ZeroReduceTop:
	xor		bl,bl
@@:
	or		bl, es:[esi+ecx-1]			; Or each byte of destination scanline
	dec		ecx
	ja		@b							; Loop for each byte
	mov		ecx, sWidth					; ecx = sWidth again
	test	bl, bl						; did we zero a scanline?
	jne		LMC_ZeroReduceBottom		; Go try bottom if we didnt
	add		esi, ecx					; Move esi onto next scanline
	sub		edx, 010000h				; Move offset correctly
	dec		eax							; reduce height by one
	jbe		LMC_ZeroReductionAllDone	; Dont go below zero!
	jmp		LMC_ZeroReduceTop			; go try next scanline

	BEST_ALIGN
LMC_ZeroReduceBottom:
	push	esi							; save bitmap start
	push	edx							; save edx for the mul
	mov		ebx, eax					; ebx = remaining sHeight temporarily
	dec		eax							; want to find the address of the last scanline
	mul		ecx							; (sHeight - 1) * sWidth
	add		esi, eax					; esi is now the address of the last scanline
	mov		eax, ebx					; restore eax with sHeight
	pop		edx							; restore edx
LMC_ZeroReduceBottomNext:
	xor		bl,bl
@@:
	or		bl, es:[esi+ecx-1]			; Or each byte of destination scanline
	dec		ecx
	ja		@b							; Loop for each byte
	mov		ecx, sWidth					; ecx = sWidth again
	test	bl, bl						; did we zero a scanline?
	jne		LMC_ZeroReductionBottomDone ; Exit if we couldnt
	sub		esi, ecx					; Move esi onto previous scanline
	dec		eax							; reduce height by one
	jbe		LMC_ZeroReductionBottomDone	; Dont go below zero!
	jmp		LMC_ZeroReduceBottomNext	; go try next scanline

	BEST_ALIGN
LMC_ZeroReductionBottomDone:
	pop		esi							; restore bitmap start
LMC_ZeroReductionAllDone:
    mov     pSrcBitmap, esi
    mov     ds:[edi].CachedOffsets, edx	; save away altered offsets
    mov     sHeight, eax				; save away height
	mov		ecx, dWidth					; Get width and height
    mul     ecx                         ; Work out size to allocate
    add     eax, GLYPH_COMMAND_HEADER+3 ; Add in the header size and round to 32 bits
	and		eax, NOT 3
    mov     ds:[edi].CacheSize, eax     ; Store allocated size in bytes for now
LMC_AllocateHeap:
    push    edi                         ; push Glyph pointer
    invoke  AllocateHeap, eax
    cmp     eax, 0ffffffffh             ; Did the allocation work?
    jne     @f
    invoke  RemoveCachedFont            ; Free up some space - this had better not fail!
    pop     edi
    mov     eax, ds:[edi].CacheSize
    jmp     LMC_AllocateHeap
	BEST_ALIGN
@@:
    mov     ebx, pFC
    pop     edi
    or      [ebx].nGlyphs, 080000000h   ; Indicate font contains memory cached fonts

    ; Now we need to set up the Bitmap data.
    mov     esi, pSrcBitmap             ; esi = pSrcBitmap
    mov     ds:[edi].CacheOffset, eax   ; Store allocated cache offset
;    mov     ds:[eax+GLYPH_INDEX], 00d58000h ; Index tag
    mov     ebx, sHeight                ; bx = No of scanlines
	mov		ecx, MemTextCmd
    mov     ds:[eax+GLYPH_RENDER], ecx	; Render command
    mov     ecx, ds:[edi].CacheSize     ; Get size of allocated buffer in bytes
    shr     ecx, 2                      ; convert to words of cache
    mov     ds:[edi].CacheSize, ecx     ; Update size of allocated buffer in dwords
    mov     ds:[eax+GLYPH_COUNT], ebx
    sub     ecx, GLYPH_COMMAND_HEADER/4 ; convert into words of Bitmap data
    mov     edi, eax                    ; edi = pDstBitmap
	dec		ecx							; Hold count is words-1
    add     edi, GLYPH_COMMAND_HEADER   ; edi now points to start of bitmap data
    shl     ecx, 16                     ; convert to hold count
    or      ecx, 0dh                    ; Or in Bitmask data tag
    mov     ds:[eax+GLYPH_HOLD], ecx    ; store it.
    or      ebx, ebx
    je      LMC_DoneBitmap
LMC_NextScan:
    mov     edx, sWidth
    xor     ecx, ecx
@@:
    mov     al, es:[esi]                ; Get a byte of source
    add     esi, 1
    mov     ds:[edi], al                ; Write to destination
    add     edi, 1
    inc     ecx
    cmp     ecx, edx
    jne     @b

    mov     edx, dWidth                 ; Pad destination to words
    xor     eax, eax
    cmp     ecx, edx
    je      LMC_DonePadding
@@:
    mov     ds:[edi], al                ; zero pad destination
    add     edi, 1
    inc     ecx
    cmp     ecx, edx
    jne     @b
LMC_DonePadding:
    dec     ebx
    jne     LMC_NextScan
LMC_DoneBitmap:
    ret
LoadMemChar    ENDP


;------------------------------------------------------------------------------
;
;   LoadOSTChar
;       Load a OST character into the font cache
;
;------------------------------------------------------------------------------
	BEST_ALIGN
LoadOSTChar PROC NEAR C USES es pNewFontSeg:DWORD, pFC:DWORD, pGlyph:DWORD, Char:DWORD
    LOCAL   sWidth:DWORD, dWidth:DWORD, sHeight:DWORD, pSrcBitmap:DWORD
	and		ebp, 0ffffh
	assumes ds,_INIT
	assumes	es,nothing
	assumes fs,nothing
	assumes	gs,nothing

    les     di, pNewFontSeg     
    xor     edx, edx                    ; Newfontseg starts at zero
    mov     edi, pGlyph                 ; Get glyph offset in edi
    xor     ecx, ecx                    ; zero top bits of ecx
    mov     ebx, Char

    ; First we need to decide how much data space to allocate from the OST Chunk
    test    es:[edx].nfFormat, NF_LARGE
    je      LOC_SMALLROWGLYPH
    mov     esi, es:[edx].nfGlyphOffset
    mov     esi, es:[esi + ebx*4]
    mov     dx, es:[esi].lrgOrgX
    mov     ax, es:[esi].lrgOrgY
    shl     eax, 16
    sub     edx, eax
    movsx   eax, es:[esi].lrgHeight
    movsx   ecx, es:[esi].lrgWidth
    add     esi, SIZE LARGEROWGLYPH
    jmp     LOC_GotGlyphInfo

LOC_TooBigForOST:
; We cannot cache in OST for some reason. Use memory cache for this character
    invoke  LoadMemChar, pNewFontSeg, pFC, pGlyph, Char
    ret

	BEST_ALIGN
LOC_SMALLROWGLYPH:
    mov     ecx, es:[edx].nfGlyphOffset
    xor     esi, esi
    mov     si, es:[ecx + ebx*2]
    movsx   dx, es:[esi].srgOrgX
    movsx   eax, es:[esi].srgOrgY
    shl     eax, 16
    sub     edx, eax
    movsx   eax, es:[esi].srgHeight
    movsx   ecx, es:[esi].srgWidth
    add     esi, SIZE SMALLROWGLYPH
LOC_GotGlyphInfo:
    test    edx, 08000h                 ; Is the X org negative?
    je      @f
    sub     edx, 10000h                 ; Reduce Y offset by 1, anticipating overflow from adding
@@:
    mov     sHeight, eax
    shl     eax, 16
    mov     ds:[edi].CachedOffsets, edx
    or      eax, ecx
    mov     ds:[edi].CachedWidth, eax

    add     ecx, 7                      ; turn width into bytes (of source)
    shr     ecx, 3
    mov     sWidth, ecx
    cmp     ecx, 4                      ; dWidth = sWidth if sWidth < 4
    jbe     LOC_LE4Bytes
    add     ecx, 3                      ; Work out width in bytes of dest
    and     ecx, NOT 3
    mov     eax, P_32BIT_TEXELS
    jmp     LOC_GotTextureMapFormat
LOC_LE4Bytes:
    mov     eax, [BytesToTextureMapFormat + ecx*4]
LOC_GotTextureMapFormat:
    mov     dWidth, ecx
    mov     pSrcBitmap, esi
    mov     ds:[edi].CacheSize, eax     ; Store format of glyph

    mov     eax, sHeight				
	mov		ecx, dWidth					; Get width and height
    mul     ecx                         ; Work out size to allocate
    lea     ebx, [eax + 3]              ; round to 32 bits
	and		ebx, NOT 3

    cmp     ebx, ChunkSize              ; Can we fit text into one chunk?
    jae     LOC_TooBigForOST            ; Use memory caching if not

LOC_AllocateSpace:
    mov     esi, pFC
    mov     eax, ds:[esi].OSTChunkFree
    cmp     dWidth, 3                   ; Need to align start on a 12 byte boundary
    je      LOC_24BitTexels             ; if 24 bit wide char.

    lea     edx, [eax + ebx]            ; eax = start, ebx = size, edx = end, esi = pFC, 
    cmp     edx, ds:[esi].OSTChunkEnd
    jae     LOC_AllocateExtraOSTChunk
    mov     ds:[esi].OSTChunkFree, edx  ; Update allocation
    mov     ecx, 2                      ; work out texel size. Assume 32 bit texels
    cmp     dWidth, 4                      
    jae     @f
    mov     ecx, dWidth
    shr     ecx, 1
@@:
    mov     edx, eax                    
    shr     edx, 2                      ; Texture address in DWORDS
    mov     TextureDownloadOffset, edx  ; Set up the chip to accept data
    shr     eax, cl                     ; Convert bytes to texels
    mov     ds:[edi].CacheOffset, eax   ; Store texel offset in glyph info

    mov     eax, sWidth
    cmp     eax, dWidth
    jne     LOC_DownloadPaddedGlyph

    ; ebx = size
LOC_DownloadGlyph:
    test    ebx, ebx
    je      LOC_Done
    mov     eax, ebx
    mov     esi, pSrcBitmap
    sub     eax, 4                      ; Reduce by one
    lea     edi, GPFifo2                ; Continue from here
    shl     eax, 14                     ; convert to hold count
    or      eax, 11dh                   ; TextureData Tag
    mov     GPFifo1, eax
    push    ebp
    cmp     ebx, 16
    jb      LOC_DownloadLessThan4
LOC_DownloadNext4:
    mov     eax, es:[esi +  0]
    mov     ecx, es:[esi +  4]
    mov     edx, es:[esi +  8]
    mov     ebp, es:[esi + 12]
    mov     ds:[edi +  0], eax
    mov     ds:[edi +  4], ecx
    mov     ds:[edi +  8], edx
    mov     ds:[edi + 12], ebp
    add     esi, 16
    add     edi, 16
    sub     ebx, 16
    and     edi, 0fffff7ffh             ; Ensure we dont wrap off end of GPFifo
    cmp     ebx, 16
    jae     LOC_DownloadNext4
LOC_DownloadLessThan4:
    cmp     ebx, 8
    jb      LOC_DownloadLessThan2
    mov     eax, es:[esi +  0]
    mov     ecx, es:[esi +  4]
    mov     ds:[edi +  0], eax
    mov     ds:[edi +  4], ecx
    add     esi, 8
    sub     ebx, 8
    add     edi, 8
LOC_DownloadLessThan2:    
    cmp     ebx, 4
    jb      LOC_DownloadDone
    mov     eax, es:[esi +  0]
    mov     ds:[edi +  0], eax
LOC_DownloadDone:
    pop     ebp
    mov     WaitForCompletion, 0        ; Make sure its downloaded before its used!
LOC_Done:
    ret

	BEST_ALIGN
LOC_24BitTexels:
    ; eax = OSTChunkFree, ebx = size 
    xor     edx, edx
    mov     esi, eax
    mov     ecx, 12
    div     ecx                         ; edx = remainder
    shl     eax, 2                      ; Convert from 12 byte alignment to texel alignment
    test    edx, edx                    ; At boundary if remainder = 0
    je      @f
    sub     ecx, edx                    ; ecx is number to add to get to boundary
    add     eax, 4                      ; eax is number of texels to address
    lea     esi, [esi + ecx]            ; work out boundary address
@@:
    mov     ecx, pFC
    lea     edx, [esi + ebx]            ; work out end address
    mov     ds:[edi].CacheOffset, eax   ; Store texel offset in glyph info

    cmp     edx, ds:[ecx].OSTChunkEnd
    jae     LOC_AllocateExtraOSTChunk

    mov     ds:[ecx].OSTChunkFree, edx  ; Update allocation
    shr     esi, 2                      ; Convert start to DWORDS
    mov     TextureDownloadOffset, esi  ; Set up the chip to accept data
    jmp     LOC_DownloadGlyph

	BEST_ALIGN
LOC_AllocateExtraOSTChunk:
    push    ebx
    push    edi
LOC_AllocateExtraOSTChunkAgain:
    mov     edi, pFC
    invoke  AllocateOST, edi
    test    eax, eax                    ; Did the allocation work?
    je     @f

    mov     ecx, ChunkShift
    shl     eax, cl                     ; Work out address of chunk from chunk 0
    mov     ebx, ChunkSize
    add     eax, ChunkOffset            ; Work out byte offset of chunk start
    add     ebx, eax
    cmp     [edi].OSTChunkEnd, eax      ; If new chunk starts at end of last chunk
    je      @f                          ; then we can span 2 blocks, saving cache space
    mov     [edi].OSTChunkFree, eax     ; In this case, dont update cache start addr 
@@:
    mov     [edi].OSTChunkEnd, ebx

    pop     edi
    pop     ebx
    jmp     LOC_AllocateSpace
@@:
    invoke  RemoveCachedFont            ; Free up some space - this had better not fail!
    jmp     LOC_AllocateExtraOSTChunkAgain

LOC_DownloadPaddedGlyph:
    ; This is a slower version of the download. Hopefully it wont get used too much.
    ; The source and destination widths are different in this case.
    ; ebx = size
    mov     eax, ebx
    mov     esi, pSrcBitmap
    sub     eax, 4                      ; Reduce by one
    lea     edi, GPFifo2                ; Continue from here
    shl     eax, 14                     ; convert to hold count
    or      eax, 11dh                   ; TextureData Tag
    mov     GPFifo1, eax

    mov     ecx, sWidth
    xor     edx, edx
LOC_DownloadPaddedGlyphNextDWORD:
    mov     eax, es:[esi + edx]
    mov     ds:[edi +  edx], eax
    add     edx, 4
    sub     ecx, 4
    ja      LOC_DownloadPaddedGlyphNextDWORD
    mov     eax, dWidth
    mov     ecx, sWidth
    xor     edx, edx
    add     edi, eax
    add     esi, ecx
    and     edi, 0fffff7ffh             ; Ensure we dont wrap off end of GPFifo
    sub     ebx, eax
    ja      LOC_DownloadPaddedGlyphNextDWORD
    mov     WaitForCompletion, 0        ; Make sure its downloaded before its used!
    ret

LoadOSTChar    ENDP


	BEST_ALIGN
PLABEL  ExtTextOut16

	StackMangle16To32

;	lpDevice:DWORD,		;Destination device
;	x:WORD,			    ;Left origin of string
;	y:WORD,			    ;Top  origin of string
;	lpClipRect:DWORD,	;Clipping rectangle
;	lpString:DWORD,		;The string itself
;	count:WORD,			;Number of characters in the string
;	lpFont:DWORD,		;Font to use
;	lpDrawMode:DWORD,	;Drawmode structure to use
;	lpXform:DWORD,		;Current text transform
;	lpDx:DWORD,			;Widths for the characters
;	lpOpaqueRect:DWORD,	;Opaquing rectangle
;	etoOptions:WORD		;ExtTextOut options


ExtTextOutParms	STRUCT
	_ebp		    DWORD	0
	_retLo		    DWORD	0
	_retHi		    DWORD	0

	etoOptions	    WORD	0
	lpOpaqueRect    DWORD	0
	lpDx    	    DWORD	0
	lpXform		    DWORD	0
	lpDrawMode	    DWORD	0
	lpFont		    DWORD	0
	count		    WORD	0
	lpString	    DWORD	0
	lpClipRect	    DWORD	0
	wDestY     	    WORD	0
	wDestX     	    WORD	0
	lpDevice	    DWORD	0
ExtTextOutParms	ENDS

ExtTextOut32	PROC FAR PASCAL PUBLIC		;remember that USES would override "ret" code...
    LOCAL GlyphBase:DWORD, GlyphIndexed:DWORD, DestXY:DWORD, CharOffset:DWORD,
		  lpStrSeg:DWORD, lpDxSeg:DWORD, CharCount:DWORD, pFC:DWORD,
          BackCol:DWORD, TextCol:DWORD, LimitsMaxX:DWORD, LimitsMinX:DWORD, LimitsMinY:DWORD 

	assumes	ds,nothing
	assumes	es,nothing
	assumes	fs,nothing
	assumes	gs,nothing

	and		ebp,0ffffh

	push	ds
	push	esi			;save 32 bit versions of these
	push	edi			;

    mov     eax,_INIT
    mov     ds, eax
	assumes	ds,_INIT
	mov	    edx, RemappedDataSegment

	mov		eax, DWORD PTR [ebp].etoOptions
    xor     esi, esi

	test	eax, ETO_LEVEL_MODE			    ;Is this an anti aliased background mode?
	jnz	    Jmp_DIB_ETO                     ;Yes, give to dib engine

	test	eax, ETO_BYTE_PACKED            ;Is this a font we support?
   	je	    Jmp_DIB_ETO                     ;if not, give to dib engine

	lds	    si,[ebp].lpDevice        	    ;ds:si-->lpDevice

    mov     ebx, DWORD PTR ds:[esi].deFlags
	mov		ecx, DWORD PTR [ebp].count

	cmp	    ds:[esi].deType,TYPE_DIBENG     ;is this a DIBENGINE surface?
    jne     Jmp_DIB_ETO
    test    ebx,VRAM
    jz      Jmp_DIB_ETO                     ;out if busy
    test    ebx,BUSY
    jnz     Jmp_DIB_ETO                     ;out if busy
	test	DWORD PTR ds:[esi].deDriverReserved,ACCELERATED  	            ;is this accelerated?
	jz		Jmp_DIB_ETO	                    ;no, let the DIB Engine handle it.

	push	edx							    ; save Data segment for ds
	pop		ds							    ; ds = data segment
	assumes ds, _INIT

    mov     ebx, GLInfo.dwFontCache16       ; Get Font Cache segment
    test	ecx, 8000h                      ; count is a 16 bit unsigned value
											; is this an extent call ?
    mov     eax, GLInfo.dwCursorType
	jnz		Jmp_DIB_ETO                     ;let the DIB Engine handle it.
	and		ecx, 0ffffh
	jz		ETO_OpaqueRect				    ;no text to draw - just blank area

	test	ebx, ebx
	jz		Jmp_DIB_ETO

	mov		CharCount, ecx

;add this if you simply want to drop every other string to emulate
;handling bold without double draws
;XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
if 0
	test	dropDoubles,-1
	jz		no_special_spacing
	dec		ecx						;many characters to draw? (we don't accelerate 1 char strings)
	jle		no_special_spacing
	mov		edx, [ebp].lpDx			;is there a special widths table
	mov		ecx, dword ptr [ebp].etoOptions
	test	edx,edx
	jz		no_special_spacing
	test	ecx, ETO_GLYPH_INDEX
	jnz		no_special_spacing
	mov		cl, dropThis
	xor		cl, 1					;Yes, then drop every other string
	mov		dropThis, cl
	jz		ETO_Exit
	mov		boldByHand, 1			;clear this flag on the way out
	;if (dropThis) then it's a bold font to double by hand!!!
  no_special_spacing:
endif
;XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

	les		si, [ebp].lpDrawMode			; es:si = draw mode
    
	test    eax,1          	                ;running with a software cursor?
	jne	    ETO_Excluded                    ;nope, no need to exclude hdw cursor
    call    ETO_CursorExclude
ETO_Excluded:
    mov     ecx, es:[esi].TextColor
    xor     eax, eax
	mov		edx, es:[esi].bkColor
    mov     TextCol, ecx
    mov     BackCol, edx
	jmp     [TextFn]

	BEST_ALIGN
PLABEL Jmp_DIB_ETO
    pop     edi
    pop     esi
    pop     ds
    mov     esp, ebp
    pop     ebp
	StackMangle32To16				        ; convert the 32:32 ret addr back to 16:16

	jmp		DIB_ExtTextOut

	BEST_ALIGN
PLABEL ETO_OpaqueRect
	assumes ds,nothing
	assumes	es,nothing
	assumes	fs,nothing
	assumes ds, _INIT

	cmp	    GLInfo.dwCursorType,0           ;running with a software cursor?
	jne	    OR_Excluded     	            ;nope, no need to exclude hdw cursor
    call    ETO_CursorExclude

	BEST_ALIGN
OR_Excluded:

    DISPLAY_CONTEXT
    ;ENABLE_DISCONNECT ds

    mov     ebx, DD_EXTTEXTOUT_CTXT
    mov     eax, [ebp].lpDevice
	mov		DD_CurrentContext, ebx          ; Last operation was a text operation
	mov		ecx, GLInfo.dwBpp
    cmp     eax, GLInfo.dwCurrentOffscreen
    je      @f
    les     bx, [ebp].lpDevice
    mov     GLInfo.dwCurrentOffscreen, eax
    mov     edx, es:[bx].OSB_PixelOffset
    mov     FBWindowBase, edx
@@:

	les		bx, [ebp].lpDrawMode			; es:bx = draw mode
	mov		edx, es:[ebx].bkColor
    mov     ebx, OFFSET FBBlockColor        ; Offset of colour register to use
	cmp		cl, 16
    ja      OR_ReplicateAbove16
	je		OR_Replicate16
    mov     dh, dl                          ; 8->16
OR_Replicate16:
    mov     ecx, edx
    and     edx, 0ffffh
    shl     ecx, 16
    or      edx, ecx                        ;16->32
    jmp     OR_Replicated
OR_ReplicateAbove16:
	cmp		cl, 32
	je		OR_Replicated
    ; 24 bpp. Select Block fill if grey or single pixels otherwise
    mov     ecx, edx
    cmp     dl, dh                          ; Are lower and middle equal
    jne     OR_24bppNotGrey                 ; If not, do slow fill
    shr     ecx, 8
    cmp     cl, ch                          ; Are middle and upper equal
    jne     OR_24bppNotGrey                 ; If not, do slow fill
    shl     edx, 8                          ; Replicate all bytes
    mov     OpaqueBltCmd, 48h               ; expecting a block fill
    mov     dl, dh
    jmp     OR_Replicated
OR_24bppNotGrey:
    mov     ebx, OFFSET FBWriteData         ; Offset of colour register to use
    mov     OpaqueBltCmd, 40h               ; expecting no block fills
OR_Replicated:
    WaitFifo 6
	mov		[ebx], edx

	mov		ebx, [ebp].lpClipRect      	    ; Get segment of cliprect
	test	ebx, ebx
	je		OR_NoClipRect

    xor     esi, esi
    xor     edi, edi

	les		di, [ebp].lpOpaqueRect          ; es:di = lpOpaque
	lfs		si, [ebp].lpClipRect	        ; ds:si = lpClip
	mov		eax, DWORD PTR es:[edi].top		; Get Top of Opaque
	mov		ebx, DWORD PTR fs:[esi].top	    ; get top of clip
	cmp		bx, ax	  				        ; Do we need to reduce the top? (signed)
	jle		@f
	mov		eax, ebx					    ; Only opaque to top of clip
@@:
	mov		bx, fs:[esi].bottom				; Get bottom of clip (Word!)
    mov     edx, eax
    shl     edx, 16
    and     ebx, 0ffffh
	mov		cx, es:[edi].bottom				; Get Bottom of Opaque (Word!)
	mov		StartY, edx
	cmp		bx, cx                          ; signed
	jge		@f
	mov		ecx, ebx					    ; Only Opaque to bottom of clip
@@:				
    and     eax, 0ffffh
    and     ecx, 0ffffh
	sub		ecx, eax					    ; turn dx into count
	jle		OR_Exit
	mov		ebx, DWORD PTR es:[edi].left    ; Get left of Opaque
	mov		Count, ecx
  	mov		eax, DWORD PTR fs:[esi].left    ; Get left of clip
	cmp		ax, bx					        ; do we need to reduce the left?
	jle		@f
	mov		ebx, eax
@@:
	mov		eax, DWORD PTR es:[edi].right			    ; Get Right
    mov     edx, ebx
    shl     edx, 16
	mov		ecx, DWORD PTR fs:[esi].right			
	mov		StartXDom, edx
	cmp		cx, ax
	jge		@f
	mov		eax, ecx
@@:
	cmp		bx, ax					        ; Are we clipped out? (signed)
	jge		OR_Exit				            ; If so, exit
    shl     eax, 16
	mov		edi, OpaqueBltCmd
	mov		StartXSub, eax
	mov		Render, edi
    jmp     OR_Exit

	BEST_ALIGN
OR_NoClipRect:
    xor     esi, esi
	les		si, [ebp].lpOpaqueRect	        ; ds:si = lpOpaque

	mov		ecx, DWORD PTR es:[esi].top		; Get Top
	mov		dx, es:[esi].bottom				; Get Bottom
	mov		eax, DWORD PTR es:[esi].left	; Get left
	sub		edx, ecx					    ; turn dx into count
	mov		ebx, DWORD PTR es:[esi].right	; Get Right
    and     edx, 0ffffh
	shl		eax, 16
	mov		Count, edx
	shl		ebx, 16
	mov		edi, OpaqueBltCmd
	shl		ecx, 16
	mov		StartXDom, eax
	mov		StartXSub, ebx
	mov		StartY,ecx
	mov		Render, edi

OR_Exit:
	cmp	    GLInfo.dwCursorType,0                  ;running with a software cursor?
	je	    @f                              ;nope, no need to exclude hdw cursor
OR_PostSWCursor:
	pop		edi					
	pop		esi					
	pop		ds
	leave
	mov		eax, 1
	retf	sizeof ExtTextOutParms - 12		;8 bytes of return addr and 4 of ebp

@@:
	les		bx, GLInfo.lpDriverPDevice				
	and	    es:[bx].deFlags,NOT BUSY
   	cmp	    DIBAccessCallFlag,0             ;did we call DIB eng to exclude cursor?
	je	    OR_PostSWCursor	                ;nope, skip the following!
	push	[ebp].lpDevice		            ;
	push	word ptr CURSOREXCLUDE		            ;
	call	DIB_EndAccess		            ;Let DIB Engine unexclude cursor.
	mov	DIBAccessCallFlag,0	                ;clear the flag
    jmp     OR_PostSWCursor

	BEST_ALIGN
PLABEL ETO_CursorExclude
    push    es
    push    esi
    xor     esi, esi
	les		si, GLInfo.lpDriverPDevice				
	or	    es:[esi].deFlags,BUSY

    les     si, [ebp].lpDevice
    xor     eax, eax
    mov     ax, es:[esi].deFlags
    test    eax, OFFSCREEN
    jne     ETO_ReturnFromExclude
    test    eax, VRAM
    je      ETO_ReturnFromExclude

    cmp     [ebp].lpOpaqueRect, 0
    je      @f
    les     si, [ebp].lpOpaqueRect
@@:
    cmp     [ebp].lpClipRect, 0
    je      @f
    les     si, [ebp].lpClipRect
@@:
    xor     eax, eax
    xor     ecx, ecx
    mov     ax, es:[esi].left
    xor     ebx, ebx
    mov     cx, es:[esi].right
    xor     edx, edx
    mov     bx, es:[esi].top
    sub     ecx, eax
    mov     dx, es:[esi].bottom
    sub     edx, ebx
    invoke  CursorExclude, eax, ebx, ecx, edx
ETO_ReturnFromExclude:
    pop     esi
    pop     es
    retn


	BEST_ALIGN
PLABEL ETOBlockFill
	assumes ds,_INIT
	assumes	es,nothing
	assumes	fs,nothing
	assumes gs,nothing                  

    DISPLAY_CONTEXT
    ;ENABLE_DISCONNECT ds
    WaitFifo    16
    DISABLE_FIFO_CHECKING

    mov     ebx, DD_EXTTEXTOUT_CTXT
    mov     ecx, [ebp].lpDevice
	mov		DD_CurrentContext, ebx          ; Last operation was a text operation
    mov     eax, DWORD PTR [ebp].etoOptions	; Get options
    
    cmp     ecx, GLInfo.dwCurrentOffscreen
    je      ETOBF_SameOffscreen
    les     bx, [ebp].lpDevice
    mov     GLInfo.dwCurrentOffscreen, ecx
    mov     edx, es:[ebx].OSB_PixelOffset
    mov     FBWindowBase, edx
ETOBF_SameOffscreen:

    test    eax, ETO_CLIPPED                ; Do we have to merge cliprect and opaque rect
    je      ETOBF_ClipOnly
    ; the cliprect actually comes from the intersection of the ClipRect and the OpaqueRect
    test    eax, ETO_OPAQUE
    je      ETOBF_MergeClipOpaqueNoOpaqueRect

; Replicate the background color
    mov     edx, BackCol
    mov     ecx, GLInfo.dwBpp
    mov     ebx, OFFSET FBBlockColor        ; Offset of colour register to use
	cmp		cl, 16
    ja      ETOBF_Bg1ReplicateAbove16
	je		ETOBF_Bg1Replicate16
    mov     dh, dl                          ; 8->16
ETOBF_Bg1Replicate16:
    mov     ecx, edx
    and     edx, 0ffffh
    shl     ecx, 16
    or      edx, ecx                        ;16->32
    jmp     ETOBF_Bg1Replicated
ETOBF_Bg1ReplicateAbove16:
	cmp		cl, 32
	je		ETOBF_Bg1Replicated
    ; 24 bpp. Select Block fill if grey or single pixels otherwise
    mov     ecx, edx
    cmp     dl, dh                          ; Are lower and middle equal
    jne     ETOBF_Bg1_24bppNotGrey          ; If not, do slow fill
    shr     ecx, 8
    cmp     cl, ch                          ; Are middle and upper equal
    jne     ETOBF_Bg1_24bppNotGrey          ; If not, do slow fill
    shl     edx, 8                          ; Replicate all bytes
    mov     OpaqueBltCmd, 48h               ; expecting a block fill
    mov     dl, dh
    jmp     ETOBF_Bg1Replicated
ETOBF_Bg1_24bppNotGrey:
    mov     ebx, OFFSET FBWriteData         ; Offset of colour register to use
    mov     OpaqueBltCmd, 40h               ; expecting no block fills
ETOBF_Bg1Replicated:
    mov     [ebx], edx                      ; Load the colour

    xor     esi,esi
    xor     edi, edi
    les     si, [ebp].lpClipRect
    lgs     di, [ebp].lpOpaqueRect
    mov     ebx, DWORD PTR es:[esi].top
    mov     ecx, DWORD PTR gs:[edi].top
    cmp     bx, cx                          ; Compare in 16 bits as sign is relevant
    jle     @f
    mov     ecx, ebx
@@:
    ; ecx contains top (rubbish in top 16 bits)
    mov     ebx, DWORD PTR es:[esi].left
    and     ecx, 0ffffh
    mov     edx, DWORD PTR gs:[edi].left
    mov     LimitsMinY, ecx
    cmp     bx, dx                          ; Compare in 16 bits as sign is relevant.
    jle     @f
    mov     edx, ebx
@@:
    ; edx contains the left (rubbish in top 16 bits)
    shl     ecx, 16
    and     edx, 0ffffh
    mov     ebx, edx                  
    or      edx, ecx                        ; edx contains the top<<16 + left
    shl     ebx, 16                         ; Work out XDom
	mov		ScissorMinXY, edx
    shl     edx, 16
    mov     StartXDom, ebx                  ; Load it
    mov     ebx, DWORD PTR es:[esi].right
    mov     StartY, ecx                     ; Load StartY into the chip
    mov     ecx, DWORD PTR gs:[edi].right
    mov     LimitsMinX, edx                  ; Store limits for future use.
    cmp     bx, cx                          ; Compare in 16 bits as sign is relevant
    jge     @f
    mov     ecx, ebx
@@:
    ; ecx contains the right (rubish in top 16 bits)
    mov     bx, es:[esi].bottom				; Cant DWORD load as they might be at end of segment
    and     ecx, 0ffffh
    mov     dx, gs:[edi].bottom
    cmp     bx, dx                          ; Compare in 16 bits as sign is relevant
    jge     @f
    mov     edx, ebx
@@:
    ; edx contains the bottom (rubish in top 16 bits)
; draw the opaque rect
    mov     esi, ecx                  
    and     edx, 0ffffh
    shl     esi, 16                         ; Work out XDom
    mov     ebx, edx
    mov     StartXSub, esi                  ; Load it
    sub     ebx, LimitsMinY                 ; top - preserved from earlier
    mov     edi, OpaqueBltCmd
    mov     Count, ebx                      ; Load StartY into the chip
    shl     edx, 16
    mov     Render, edi                     ; Render the rectangle
    or      edx, ecx                        ; edx contains the bottom<<16 + right
	mov		ScissorMaxXY, edx		
    shl     ecx, 16
    mov     ebx, 3	
    mov     LimitsMaxX, ecx                  ; Store limits for future use.
    jmp     ETOBF_ClipAndOpaqueDone

	BEST_ALIGN
ETOBF_MergeClipOpaqueNoOpaqueRect:
    xor     esi, esi
    xor     edi, edi
    les     si, [ebp].lpClipRect
    lgs     di, [ebp].lpOpaqueRect
    mov     ebx, DWORD PTR es:[esi].left
    mov     edx, DWORD PTR gs:[edi].left
    cmp     bx, dx                          ; Compare in 16 bits as sign is relevant
    jle     @f
    mov     edx, ebx
@@:
    ; edx contains the left (rubish in top 16 bits)
    mov     ebx, DWORD PTR es:[esi].top
    and     edx, 0ffffh
    mov     ecx, DWORD PTR gs:[edi].top
    cmp     bx, cx                          ; Compare in 16 bits as sign is relevant
    jle     @f
    mov     ecx, ebx
@@:
    ; ecx contains the top (rubish in top 16 bits)
    and     ecx, 0ffffh
    mov     LimitsMinY, ecx
    shl     ecx, 16
    or      edx, ecx                        ; edx contains the top<<16 + left
	mov		ScissorMinXY, edx

    mov     ebx, DWORD PTR es:[esi].right
    mov     LimitsMinX, ecx                  ; Store limits for future use.
    mov     edx, DWORD PTR gs:[edi].right
    cmp     bx, dx                          ; Compare in 16 bits as sign is relevant
    jge     @f
    mov     edx, ebx
@@:
    ; edx contains the right (rubbish in top 16 bits)
    mov     bx, es:[esi].bottom			; Cant DWORD load as they might be at end of segment
    and     edx, 0ffffh
    mov     cx, gs:[edi].bottom
    cmp     bx, cx                          ; Compare in 16 bits as sign is relevant
    jge     @f
    mov     ecx, ebx
@@:
    shl     ecx, 16
    or      ecx, edx                        ; edx contains the bottom<<16 + right
    shl     edx, 16
	mov		ScissorMaxXY, ecx		
    mov     LimitsMaxX, edx                  ; Store limits for future use.
    mov     ebx, 3
    jmp     ETOBF_ClipAndOpaqueDone

ETOBF_NoClipOrOpaqueClip:
    mov     ebx, [ebp].lpOpaqueRect
    xor     esi, esi
    mov     LimitsMaxX, 08000000h           ; Store limits for future use.
    mov     LimitsMinY, esi					; 00000000h
    mov     LimitsMinX, esi					; 00000000h
    test    ebx, ebx
    je      ETOBF_ClipAndOpaqueDone         ; Scissor mode of 0 will do!
; Opaque the Opaque Rect
	les		si, [ebp].lpOpaqueRect	        ; es:esi = lpOpaque

; Replicate the background color
    mov     edx, BackCol
    mov     ecx, GLInfo.dwBpp
    mov     ebx, OFFSET FBBlockColor        ; Offset of colour register to use
	cmp		cl, 16
    ja      ETOBF_Bg2ReplicateAbove16
	je		ETOBF_Bg2Replicate16
    mov     dh, dl                          ; 8->16
ETOBF_Bg2Replicate16:
    mov     ecx, edx
    and     edx, 0ffffh
    shl     ecx, 16
    or      edx, ecx                        ;16->32
    jmp     ETOBF_Bg2Replicated
ETOBF_Bg2ReplicateAbove16:
	cmp		cl, 32
	je		ETOBF_Bg2Replicated
    ; 24 bpp. Select Block fill if grey or single pixels otherwise
    mov     ecx, edx
    cmp     dl, dh                          ; Are lower and middle equal
    jne     ETOBF_Bg2_24bppNotGrey          ; If not, do slow fill
    shr     ecx, 8
    cmp     cl, ch                          ; Are middle and upper equal
    jne     ETOBF_Bg2_24bppNotGrey          ; If not, do slow fill
    shl     edx, 8                          ; Replicate all bytes
    mov     OpaqueBltCmd, 48h               ; expecting a block fill
    mov     dl, dh
    jmp     ETOBF_Bg2Replicated
ETOBF_Bg2_24bppNotGrey:
    mov     ebx, OFFSET FBWriteData         ; Offset of colour register to use
    mov     OpaqueBltCmd, 40h               ; expecting no block fills
ETOBF_Bg2Replicated:
    mov     [ebx], edx                      ; Load the colour


	mov		ecx, DWORD PTR es:[esi].top		; Get Top
	mov		edi, OpaqueBltCmd
	mov		dx, es:[esi].bottom			    ; Get Bottom. Must be WORD access!
	mov		eax, DWORD PTR es:[esi].left	; Get left
	sub		edx, ecx					    ; turn dx into count (rubbish in top 16 bits)
	mov		ebx, DWORD PTR es:[esi].right	; Get Right
    and     edx, 0ffffh
	shl		eax, 16
	mov		Count, edx
	shl		ebx, 16
	mov		StartXDom, eax
	shl		ecx, 16
	mov		StartXSub, ebx
	mov		StartY,ecx
	mov		Render, edi
    jmp     ETOBF_ClipAndOpaqueDone

	BEST_ALIGN
ETOBF_ClipOnly:
    ; We may need to opaque the background in this case. (!ETO_CLIPPED)
    mov     ecx, [ebp].lpClipRect
    xor     esi, esi
    test    ecx, ecx                        ; Have we got a cliprect?
    je      ETOBF_NoClipOrOpaqueClip
	lfs		si, [ebp].lpClipRect		    ; fs:esi = lpClipRect
    mov     ebx, [ebp].lpOpaqueRect
    test    ebx, ebx
    je      ETOBF_NoOpaqueRect
    
; Opaque if we have an opaque rect

; Replicate the background color
    mov     edx, BackCol
    mov     ecx, GLInfo.dwBpp
    mov     ebx, OFFSET FBBlockColor        ; Offset of colour register to use
	cmp		cl, 16
    ja      ETOBF_Bg3ReplicateAbove16
	je		ETOBF_Bg3Replicate16
    mov     dh, dl                          ; 8->16
ETOBF_Bg3Replicate16:
    mov     ecx, edx
    and     edx, 0ffffh
    shl     ecx, 16
    or      edx, ecx                        ;16->32
    jmp     ETOBF_Bg3Replicated
ETOBF_Bg3ReplicateAbove16:
	cmp		cl, 32
	je		ETOBF_Bg3Replicated
    ; 24 bpp. Select Block fill if grey or single pixels otherwise
    mov     ecx, edx
    cmp     dl, dh                          ; Are lower and middle equal
    jne     ETOBF_Bg3_24bppNotGrey          ; If not, do slow fill
    shr     ecx, 8
    cmp     cl, ch                          ; Are middle and upper equal
    jne     ETOBF_Bg3_24bppNotGrey          ; If not, do slow fill
    shl     edx, 8                          ; Replicate all bytes
    mov     OpaqueBltCmd, 48h               ; expecting a block fill
    mov     dl, dh
    jmp     ETOBF_Bg3Replicated
ETOBF_Bg3_24bppNotGrey:
    mov     ebx, OFFSET FBWriteData         ; Offset of colour register to use
    mov     OpaqueBltCmd, 40h               ; expecting no block fills
ETOBF_Bg3Replicated:
    xor     edi, edi
	les		di, [ebp].lpOpaqueRect	        ; es:edi = lpOpaque
    mov     [ebx], edx                      ; Load the colour


	mov		ebx, DWORD PTR fs:[esi].top	    ; get top of clip
	mov		edx, DWORD PTR es:[edi].top	    ; Get Top of Opaque
	and		ebx, 0ffffh
	cmp		bx, dx		    				; Do we need to reduce the top? (signed)
	jle		@f
	mov		edx, ebx					    ; Only opaque to top of clip
@@:
    ; edx contains the left
	xor		ecx, ecx
	mov		bx, es:[edi].bottom	            ; Get Bottom of Opaque. 16 bit
    mov     eax, edx                        ; preserve left
    shl     edx, 16
	mov		cx, fs:[esi].bottom	            ; Get bottom of clip 16 bit
	mov		StartY, edx
	cmp		cx, bx							; Compare in 16 bits as sign is relevant
	jge		@f
	mov		ebx, ecx					    ; Only Opaque to bottom of clip
@@:				
	sub		bx, ax  					    ; turn dx into count (signed)
	jle		ETOBF_NoOpaqueRect
	mov		ecx, DWORD PTR es:[edi].left	; Get left of Opaque
	mov		Count, ebx
	mov		edx, DWORD PTR fs:[esi].left	; Get left of clip
	cmp		dx, cx  					    ; do we need to reduce the left? (signed)
	jle		@f
	mov		ecx, edx
@@:
    mov     eax, ecx
    shl     ecx, 16
	mov		edx, DWORD PTR es:[edi].right	; Get Right
	mov		StartXDom, ecx
	mov		ebx, DWORD PTR fs:[esi].right
	and		edx, 0ffffh
	and		ebx, 0ffffh
	cmp		bx, dx
	jge		@f
	mov		edx, ebx
@@:
	cmp		ax, dx					        ; Are we clipped out? (signed)
	jge		ETOBF_NoOpaqueRect				; If so, exit
    shl     edx, 16
	mov		edi, OpaqueBltCmd
	mov		StartXSub, edx
	mov		Render, edi                 ; Render the rectangle
ETOBF_NoOpaqueRect:
         
    mov     edx, fs:[esi]                   ; Get scissor MinXY
    mov     ecx, fs:[esi+4]                 ; Get scissor MaxXY
    mov     ebx, edx
	mov		ScissorMaxXY, ecx
    shr     ebx, 16		
	mov		ScissorMinXY, edx
    shl     ecx, 16
    mov     LimitsMinY, ebx
    shl     edx, 16
    mov     ebx, 3	
    mov     LimitsMaxX, ecx                  ; Store limits for future use.
    mov     LimitsMinX, edx

ETOBF_ClipAndOpaqueDone:                    ; ebx = scissor mode value
	mov		ScissorMode, ebx

; Replicate the background color
    mov     edx, TextCol
    mov     ecx, GLInfo.dwBpp
    mov     ebx, OFFSET FBBlockColor        ; Offset of colour register to use
	cmp		cl, 16
    ja      ETOBF_ReplicateAbove16
	je		ETOBF_Replicate16
    mov     dh, dl                          ; 8->16
ETOBF_Replicate16:
    mov     ecx, edx
    and     edx, 0ffffh
    shl     ecx, 16
    or      edx, ecx                        ;16->32
    jmp     ETOBF_Replicated
ETOBF_ReplicateAbove16:
	cmp		cl, 32
	je		ETOBF_Replicated
    ; 24 bpp. Select Block fill if grey or single pixels otherwise
    mov     ecx, edx
    cmp     dl, dh                          ; Are lower and middle equal
    jne     ETOBF_24bppNotGrey              ; If not, do slow fill
    shr     ecx, 8
    cmp     cl, ch                          ; Are middle and upper equal
    jne     ETOBF_24bppNotGrey              ; If not, do slow fill
    shl     edx, 8                          ; Replicate all bytes
    or      MemTextCmd, 8h                  ; Ensure a block fill
    mov     dl, dh
    jmp     ETOBF_Replicated
ETOBF_24bppNotGrey:
    mov     ebx, OFFSET FBWriteData         ; Offset of colour register to use
    and     MemTextCmd, 0fffffff7h          ; remove Block Fill
ETOBF_Replicated:
    mov     [ebx], edx                      ; Load the colour

    mov     edx, LimitsMinY
    shl     edx, 16
    mov     eax, DWORD PTR [ebp].etoOptions
    dec     edx
    mov     LimitsMinY, edx

	; Now just have the hard task of drawing the text
    test    eax, ETO_BYTE_PACKED
    je      ETOBF_NotBytePackedFont

    ; We have a new format font.
    and     eax, ETO_GLYPH_INDEX
    mov     ebx, LastFontID             ; Get the last font ID
    lgs     di, [ebp].lpFont
	mov		GlyphIndexed, eax			; save flag locally to indicate if we are Glyph Indexed
    xor     edi, edi                    ; Newfontseg starts at 0 offset
    mov     ecx, gs:[edi].nfUniqueID    ; Get the font ID
    mov     esi, LastFontpFC            ; Assume we will be using the same font as last time
    cmp     ecx, ebx
    je      @f                          ; The pFC is correct

    mov     esi, [ebp].lpFont			; esi = lpFont
    push    eax                         ; Preserve etoWOptions    
    invoke  FindFontCache, NEW_FONT, ecx, esi
    mov     esi, eax                    ; save return font cache
    pop     eax

	test	esi, esi					; Did we manage to locate/cache font?
	jne		@f
    mov     ScissorMode, 0              ; reset scissor mode and pass to DIB
    jmp     Jmp_DIB_ETO
@@:
    mov     pFC, esi
    mov     ebx, CurrentAccess
    xor     edi, edi
    inc     ebx
    mov     edx, DWORD PTR [ebp].wDestY ; Get Dest Y
    mov     [esi].LastAccess, ebx
    mov     CurrentAccess, ebx

;----------------------------------------------------------------------------------------
; Byte Packed
;----------------------------------------------------------------------------------------

    mov     ebx, DWORD PTR [ebp].wDestX ; Get Destination X
    xor     ecx, ecx                    ; Initialise char offset to zero
    and     ebx, 0ffffh
    shl     edx, 16                     ; convert to 16.16 format
    mov     CharOffset, ecx             ; Update locals
    or      ebx, edx
    mov     edx, [esi].Glyphs           ; Get GlyphBase Offset
    mov     DestXY, ebx                 ; Update local X + Y << 16
    mov     ecx, [ebp].lpString         ; get lpStr
    mov     eax, [ebp].lpDx             ; get lpDx
	mov		GlyphBase, edx

    test    eax, eax
    je      ETOBFBytePackedNoDX

	mov		ebx, eax					; replicate lpDx
    mov     edx, ecx					; replicate lpStr
    shr     ebx, 16                     ; Get segment of lpDx in bx
    and     eax, 0ffffh                 ; get offset of lpDx in eax
	push	ebx							; push segment of lpDx
    and     ecx, 0ffffh                 ; get offset of lpStr in ecx
    shr     edx, 16                     ; Get segment of lpStr in dx
    mov     lpDxSeg, eax                ; store offset of nfAWTable
    push    edx                         ; put segment lpStr on stack
    mov     lpStrSeg, ecx                  ; store offset of lpStr
    xor     ebx, ebx                    ; Char offset starts at zero
    pop     es	                        ; es = segment of string
	pop		gs							; gs = segment of lpDx
    mov     dwRectangleSize, ebx        ; Clear the size field for OST rendering
    cmp     ChunkCount, ebx
    jne     ETOOST_FirstChar
	jmp		ETOBFBPNoDX_NextChar         

	BEST_ALIGN
PLABEL ETOBFBytePackedNoDX
    mov     ebx, [ebp].lpFont           ; As we have no lpDX information, get the info
                                        ; directly from the new segment font
    mov     edx, ecx					; replicate lpStr
    shr     ebx, 16                     ; Get segment of lpFont in bx
    and     ecx, 0ffffh                 ; get offset of lpStr in ecx
	push	ebx							; push segment of lpFont
	pop		gs							; gs = segment of lpFont (Or nfAWTable)
    xor     ebx, ebx                    ; Char offset starts at zero
    shr     edx, 16                     ; Get segment of lpStr in dx
    mov     eax, gs:[ebx].nfAWTable		; gs:eax is now nfAWTable
    push    edx                         ; put segment lpStr on stack
    mov     lpDxSeg, eax                ; store offset of nfAWTable
    mov     lpStrSeg, ecx               ; store offset of lpStr
    pop     es	                        ; es = segment of string
    mov     dwRectangleSize, ebx        ; Clear the size field for OST rendering
    cmp     ChunkCount, ebx
    jne     ETOOST_FirstChar
	jmp		ETOBFBPNoDX_NextChar         

	BEST_ALIGN
PLABEL ETOBFBPNoDX_LoadChar
    push    ecx
    push    ebx
    push    edx
    push    esi
      
    mov     eax, [ebp].lpFont
    mov     ebx, pFC
    invoke  LoadMemChar, eax, ebx, esi, edx

    pop     esi
    pop     edx
    pop     ebx
    pop     ecx
    mov     edi, ds:[esi].CacheOffset      ; edi = Offset
	jmp		ETOBFBPNoDX_CharLoaded

	BEST_ALIGN

ETOBFBPNoDX_NextChar:
;   ebx = char
;   ecx = Offset of string
	mov		esi, GlyphIndexed			; get glyph indexed flag
	xor		edx, edx
	test	esi, esi					; Are we Glyph Indexed?
	jne		ETOBFBPNoDX_GetGlyphIndexedChar
    mov     dl, es:[ecx + ebx]          ; get this char in dl
	jmp		ETOBFBPNoDX_GotChar
	BEST_ALIGN
ETOBFBPNoDX_GetGlyphIndexedChar:
    mov     dx, es:[ecx + ebx*2]       ; get this Glyph Indexed char in dx
ETOBFBPNoDX_GotChar:
	mov		eax, GlyphBase
    mov     esi, edx                    ; esi = char
	mov		edi, [ebp].lpDx   		    ; Get lpDx
    shl     esi, GLYPHINFOSHIFT         ; esi = offset of glyph from glyphbase
    mov     ecx, lpDxSeg                ; get offset of nfAWTable or lpDx as appropriate
	add		esi, eax					; esi = offset of Glyph (fs:esi)
	test	edi, edi					; Are we using lpDX?
	jne		@f							; If so, offset = char Offset (already in ebx)
	mov		ebx, edx					; Otherwise, offset is char - in edx
@@:
    mov     edi, ds:[esi].CacheOffset   ; edi = Offset
	mov		ebx, gs:[ecx + ebx*2]		; get width from dx/nfAWTable table
    cmp     edi, 0ffffffffh
    je      ETOBFBPNoDX_LoadChar
ETOBFBPNoDX_CharLoaded:


;   ebx = width to advance to next char, (rubbish in top 16 bits)
    mov     edi, ds:[esi].CacheOffset   ; edi = Offset
    mov     edx, ds:[esi].CachedWidth   ; Get character width
    mov     eax, DestXY                 ; Get X Destination (in 16.16 format)
    test    edx, 080000000h             ; Is top bit of cached widths set?
    je      ETOOST_CharLoaded           ; Its a offscreen cached font if it is not
    and     ebx, 0ffffh
    mov     edx, ds:[edi + GLYPH_COUNT] ; Get Count
    add     ebx, eax                    ; ebx = next char offset
	test	eax, 8000h					; is X negative?
	je		@f							; if so, need to avoid a carry
	test	ebx, 8000h					; is X still negative?
	jne		@f
	sub		ebx, 010000h				; We carried. Remove it.
@@:
    shl     eax, 16
    mov     ecx, ds:[esi].CachedOffsets ; Get X Offset (in 16.16 format)
    mov     DestXY, ebx                 ; update next address
	test	edx, edx					; Count is zero? - GLYPHless character (like space)
	je		ETOBFBP_PrepareNextChar		; Skip loads of stuff if it is!                    
    mov     edx, ecx                    ; replicate
    mov     ebx, ds:[esi].CachedWidth   ; Get character width
    shl     ecx, 16                     ; ebx = xOffset
    add     eax, ecx                    ; work out StartXDom
	mov		ecx, LimitsMaxX
    and     edx, 0ffff0000h             ; edx = yOffset
    and     ebx, 07fffffffh             ; remove top bit   
    cmp     eax, ecx					; Is character completly to the right of clip area?
    jge     ETOBF_FinishText            ; If so, we know we dont have anything else to draw.
	mov		ecx, LimitsMinX
    add     ebx, eax                    ; edi = StartXSub
    cmp     ebx, ecx					; Is character completely to the left of clip area?
    jl      ETOBFBP_PrepareNextChar     ; If so, skip this char.

    mov     ecx, DestXY                 ; Get Y << 16 (ignore the X part)
    mov     esi, [esi].CacheSize		; number of words of cached data
    sub     ecx, edx
	mov		edx, 00d58000h
	sub		esi, 4						;the *GPFifo5++ transfers are from the font cache

	mov		GPFifo1, edx				;The first 4 dwords never get into the font cache
	mov		GPFifo2, eax
	mov		GPFifo3, ebx
	mov		GPFifo4, ecx
	mov		eax, ds:[edi+0]
	mov		ebx, MemTextCmd             ;This is the render command. Dynamic on 24bpp packed
	mov		ecx, ds:[edi+8]
	mov		edx, ds:[edi+12]
	mov		GPFifo5, eax
	mov		GPFifo6, ebx
	mov		GPFifo7, ecx
	mov		GPFifo8, edx				;this is necessarily the fist pixel line of the Glyph

PLABEL ETOBF_FastDownloadData
; download data
; ds:edi = data to download
; ds: = GlintReg segment
; esi = nWords
	cmp		esi, 8
	jl		ETOBFBP_FastDownloadDataLessThan8

ETOBFBP_FastDownloadData8orMore:
	sub		esi, 8
	mov		eax, ds:[edi+16+0]
	mov		ebx, ds:[edi+16+4]
	mov		ecx, ds:[edi+16+8]
	mov		edx, ds:[edi+16+12]
	mov		GPFifo9, eax
	mov		GPFifo10, ebx
	mov		GPFifo11, ecx
	mov		GPFifo12, edx
	mov		eax, ds:[edi+16+16]
	mov		ebx, ds:[edi+16+20]
	mov		ecx, ds:[edi+16+24]
	mov		edx, ds:[edi+16+28]
	mov		GPFifo13, eax
	mov		GPFifo14, ebx
	mov		GPFifo15, ecx
	mov		GPFifo16, edx
	add		edi, 32
	cmp		esi, 8
	jnl		ETOBFBP_FastDownloadData8orMore

ETOBFBP_FastDownloadDataLessThan8:
	jmp		ds:[Fast7 + esi * 4]

	BEST_ALIGN
FastDL5:						;Winbench makes this the important "fall-through" case
	mov		eax, ds:[edi+16+0]
	mov		ebx, ds:[edi+16+4]
	mov		ecx, ds:[edi+16+8]
	mov		edx, ds:[edi+16+12]
	mov		esi, ds:[edi+16+16]
	mov		GPFifo9, eax
	mov		GPFifo10, ebx
	mov		GPFifo11, ecx
	mov		GPFifo12, edx
	mov		GPFifo13, esi
FastDL0:
	;nothing here...

PLABEL ETOBFBP_PrepareNextChar
    mov     ebx, CharOffset             ; get CharOffset
	mov		edx, CharCount				; get count of chars to do
    inc     ebx                         ; Increase char offset
    mov     ecx, lpStrSeg               ; get offset of lpDX
;   ebx = char offset
;   ecx = Offset of string
	cmp		edx,ebx						; Have we reached the end of the string?
    jbe		ETOBF_FinishText			; exit if we have
    mov     CharOffset, ebx             ; store it again
	jmp		ETOBFBPNoDX_NextChar

	BEST_ALIGN
ETOBF_FinishText:
    ; Clear up a few registers
	mov	    edx, GLInfo.dwRenderFamily
	mov		ScissorMode, 0	
    cmp     edx, GLINT_ID
    je      @f                              ; The spanmask block fills on a TX - so dont download it
    mov     SpanMask, 0ffffffffh            ; Zero the span mask - as there is some wierd problem on a permedia.
@@:
    jmp     ETO_UnexcludeCursor


PLABEL SpecificFastDownLoads

	BEST_ALIGN
FastDL7:						; This is the only case where I can't leave the PCI
	mov		eax, ds:[edi+16+0]	; transfer unbroken. "More registers Egor, more registers..."
	mov		ebx, ds:[edi+16+4]
	mov		ecx, ds:[edi+16+8]
	mov		edx, ds:[edi+16+12]
	mov		esi, ds:[edi+16+16]
	mov		GPFifo9, eax
	mov		GPFifo10, ebx
	mov		GPFifo11, ecx
	mov		GPFifo12, edx
	mov		GPFifo13, esi
	mov		eax, ds:[edi+16+20]
	mov		ebx, ds:[edi+16+24]
	mov		GPFifo14, eax
	mov		GPFifo15, ebx
	jmp		ETOBFBP_PrepareNextChar

	BEST_ALIGN
FastDL6:
	mov		eax, ds:[edi+16+0]
	mov		ebx, ds:[edi+16+4]
	mov		ecx, ds:[edi+16+8]
	mov		edx, ds:[edi+16+12]
	mov		esi, ds:[edi+16+16]
	mov		edi, ds:[edi+16+20]
	mov		GPFifo9, eax
	mov		GPFifo10, ebx
	mov		GPFifo11, ecx
	mov		GPFifo12, edx
	mov		GPFifo13, esi
	mov		GPFifo14, edi
	jmp		ETOBFBP_PrepareNextChar

	BEST_ALIGN
FastDL4:
	mov		eax, ds:[edi+16+0]
	mov		ebx, ds:[edi+16+4]
	mov		ecx, ds:[edi+16+8]
	mov		edx, ds:[edi+16+12]
	mov		GPFifo9, eax
	mov		GPFifo10, ebx
	mov		GPFifo11, ecx
	mov		GPFifo12, edx
	jmp		ETOBFBP_PrepareNextChar

	BEST_ALIGN
FastDL3:
	mov		eax, ds:[edi+16+0]
	mov		ebx, ds:[edi+16+4]
	mov		ecx, ds:[edi+16+8]
	mov		GPFifo9, eax
	mov		GPFifo10, ebx
	mov		GPFifo11, ecx
	jmp		ETOBFBP_PrepareNextChar

	BEST_ALIGN
FastDL2:
	mov		eax, ds:[edi+16+0]
	mov		ebx, ds:[edi+16+4]
	mov		GPFifo9, eax
	mov		GPFifo10, ebx
	jmp		ETOBFBP_PrepareNextChar

	BEST_ALIGN
FastDL1:
	mov		eax, ds:[edi+16+0]
	mov		GPFifo9, eax
	jmp		ETOBFBP_PrepareNextChar


	BEST_ALIGN
PLABEL ETOOST_LoadChar
    push    ebx
    push    esi
      
    mov     eax, [ebp].lpFont
    mov     ebx, pFC
    invoke  LoadOSTChar, eax, ebx, esi, edx

    pop     esi
    pop     ebx
    mov     edi, ds:[esi].CacheOffset      ; edi = Offset
	jmp		ETOOST_CharLoaded

  	BEST_ALIGN
ETOOST_XNegative:
    add     ebx, eax                    ; ebx = X Dest + (Y Dest << 16) of next char
	test	ebx, 8000h					; is X still negative?
	jne		@f
	sub		ebx, 010000h				; We carried. Remove it.
@@:
    add     eax, ecx                    ; work out RectangleOrigin (eax)
    test    eax, 8000h                  ; is X still negative?
    jne     ETOOST_AdditionsDone
	sub		eax, 010000h				; We carried. Remove it.
    jmp     ETOOST_AdditionsDone

  	BEST_ALIGN
PLABEL ETOOST_FirstChar

if 0
endif

ETOOST_NextChar:
;   ebx = char
;   ecx = Offset of string
	mov		esi, GlyphIndexed			; get glyph indexed flag
	xor		edx, edx
	test	esi, esi					; Are we Glyph Indexed?
	jne		ETOOST_GetGlyphIndexedChar
    xor     edx, edx                    ; xor to avoid partial stall on Pentium Pro
    mov     dl, es:[ecx + ebx]          ; get this char in dl
	jmp		ETOOST_GotChar
	BEST_ALIGN
ETOOST_GetGlyphIndexedChar:
    xor     edx, edx                    ; xor to avoid partial stall on Pentium Pro
    mov     dx, es:[ecx + ebx*2]        ; get this Glyph Indexed char in dx
ETOOST_GotChar:
	mov		eax, GlyphBase
    mov     esi, edx                    ; esi = char
	mov		edi, [ebp].lpDx   		    ; Get lpDx
    shl     esi, GLYPHINFOSHIFT         ; esi = offset of glyph from glyphbase
    mov     ecx, lpDxSeg                ; get offset of nfAWTable or lpDx as appropriate
	add		esi, eax					; esi = offset of Glyph (fs:esi)
	test	edi, edi					; Are we using lpDX?
	jne		@f							; If so, offset = char Offset (already in ebx)
	mov		ebx, edx					; Otherwise, offset is char - in edx
@@:
    mov     edi, ds:[esi].CacheOffset   ; edi = Offset
	mov		ebx, gs:[ecx + ebx*2]		; get width from dx/nfAWTable table
    cmp     edi, 0ffffffffh
    je      ETOOST_LoadChar
ETOOST_CharLoaded:

;   ebx = width to advance to next char, (rubbish in top 16 bits)

    mov     edi, ds:[esi].CachedWidth   ; Get character width + (height << 16)
    mov     eax, DestXY                 ; Get X Dest + (Y Dest << 16)
    and     ebx, 0ffffh                 ; remove top word of rubish
    mov     ecx, ds:[esi].CachedOffsets ; Get X Offset + (Y Offset << 16)
    test    edi, 080000000h             ; Is top bit of cached widths set?
    jne     ETOBFBPNoDX_CharLoaded      ; Its a memory cached font if it is
	test	eax, 8000h					; is X negative?
	jne		ETOOST_XNegative			; if so, need to avoid a carry
ETOOST_XPositive:
    add     ebx, eax                    ; ebx = X Dest + (Y Dest << 16) of next char
    add     eax, ecx                    ; work out RectangleOrigin (eax)
ETOOST_AdditionsDone:
    mov     DestXY, ebx                 ; Save XY of next char
    test    edi, edi                    ; Got a space?
    je      ETOOST_PrepareNextChar
    shl     ebx, 16                     ; Get X in top 16 bits
    mov     edx, eax
    shl     edx, 16
    mov     ecx, ds:[esi].CacheSize     ; get TextureMapFormat
    cmp     edx, LimitsMaxX    			; Is character completly to the right of clip area?
    jge     ETOOST_FinishText           ; If so, we know we dont have anything else to draw.
    cmp     ebx, LimitsMinX				; Is character completely to the left of clip area?
    jl      ETOOST_PrepareNextChar      ; If so, skip this char.
    mov     edx, ds:[esi].CacheOffset   ; Get Texture Base
    mov     ebx, OSTTextCmd
    cmp     eax, LimitsMinY
    jle     ETOOST_TopClipped
ETOOST_LoadTextureMapFormat:

    cmp     dwTextureMapFormat, ecx
    je      @f
    mov     TextureMapFormat, ecx
    mov     dwTextureMapFormat, ecx
@@:
    cmp     dwRectangleSize, edi
    je      @f
	mov		RectangleSize, edi
    mov     dwRectangleSize, edi
@@:
	mov		RectangleOrigin, eax
	mov		TextureBaseAddress, edx
	mov		Render, ebx

PLABEL ETOOST_PrepareNextChar
    mov     ebx, CharOffset             ; get CharOffset
	mov		edx, CharCount				; get count of chars to do
    inc     ebx                         ; Increase char offset
    mov     ecx, lpStrSeg               ; get offset of lpDX
;   ebx = char offset
;   ecx = Offset of string
	cmp		edx,ebx						; Have we reached the end of the string?
    jbe		ETOOST_FinishText			; exit if we have
    mov     CharOffset, ebx             ; store it again
	jmp		ETOOST_NextChar

ETOOST_TopClipped:
    push    ebx
    mov     ebx, LimitsMinY
    sub     ebx, eax
    shr     ebx, 16
    cmp     di, 32
    ja      ETOOST_MoreThan1TexelPerScan
    lea     edx, [ebx + edx + 1]
    pop     ebx
    jmp     ETOOST_LoadTextureMapFormat

ETOOST_MoreThan1TexelPerScan:
    push    eax
    push    edx
    mov     eax, edi
    inc     ebx
    add     eax, 31
    and     eax, 0ffffh
    shr     eax, 5                      ; Number of 32 bit texels per scan
    mul     ebx
    pop     edx
    add     edx, eax
    pop     eax
    pop     ebx
    jmp     ETOOST_LoadTextureMapFormat

	BEST_ALIGN
PLABEL ETOBF_NotBytePackedFont
;----------------------------------------------------------------------------------------
; Not a byte packed font
;----------------------------------------------------------------------------------------
    jmp     Jmp_DIB_ETO

	BEST_ALIGN
ETOOST_FinishText:
    ; Clear up a few registers
    xor     eax, eax
	mov	    edx, GLInfo.dwRenderFamily
	mov		ScissorMode, eax	
    cmp     edx, GLINT_ID
    je      @f                              ; The spanmask block fills on a TX - so dont download it
    mov     SpanMask, 0ffffffffh            ; Zero the span mask - as there is some wierd problem on a permedia.
@@:
;    jmp     ETO_UnexcludeCursor            ; Fall through to:
PLABEL ETO_UnexcludeCursor
	cmp	wCursorType,0                       ;running with a software cursor?
	je	ETO_ReplaceCursor                   ;nope, no need to worry about hw cursor

PLABEL ETO_Exit
	pop		edi					
	pop		esi					
	pop		ds							;DS
	leave
	mov		eax,1

	retf	sizeof ExtTextOutParms - 12		;8 bytes of return addr and 4 of ebp



	BEST_ALIGN
PLABEL ETO_ReplaceCursor
;We need to unset the BUSY bit in the destination PDevice structure which
;we previously set in order to prevent the DIB engine from asynchonously
;drawing the cursor
	lfs		bx, GLInfo.lpDriverPDevice				
	mov		cl, DIBAccessCallFlag		
	and		fs:[bx].deFlags,NOT BUSY;	

   	test	cl,cl                           ;did we call DIB eng to exclude cursor?
	je	    ETO_Exit   		                ;nope, skip the following!
	push	[ebp].lpDevice		            ;
	push	word ptr CURSOREXCLUDE		            ;
	call	DIB_EndAccess		            ;Let DIB Engine unexclude cursor.
	mov	DIBAccessCallFlag,0	                ;clear the flag
    jmp ETO_Exit                            ;exit

ExtTextOut32	ENDP

_TEXT32 ends
	end
