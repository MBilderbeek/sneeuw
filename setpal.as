;--------------------------------------------------------------

; void setpal(uchar pal, uint grbdat)

        psect   text
        global  _setpal,csv,cret

_setpal:
        call    csv
        ld      a,(ix+6)        ; a=palette number
	di
	out	(99h),a
	ld	a,16+128
	out	(99h),a
        ld      a,(ix+8)        ; set RB
	out	(9ah),a
        ld      a,(ix+9)        ; set G
	out	(9ah),a
	ei
	jp	cret

