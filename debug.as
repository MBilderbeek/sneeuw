	psect	text
	global	_DBG_mode, _DBG_out
	global	csv,cret

; void DBG_mode(uchar mode)
_DBG_mode:
	call	csv
	ld	a,(ix+6)
	out	(02Eh),a
	jp	cret

; void DBG_out(uchar byte)
_DBG_out:
	call	csv
	ld	a,(ix+6)
	out	(02Fh),a
	jp	cret

