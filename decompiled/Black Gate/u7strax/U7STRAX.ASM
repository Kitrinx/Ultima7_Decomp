; U7STRAX.DRV's Flex header (144 bytes): the music drivers by device, MUSIC_DEVICE - 1.
; Turbo Assembler 2.51 and TLINK /t rebuild it byte for byte; the MAKEFILE appends the records.

; TLINK /t wants an entry point at 100h; the header itself starts the file.
start	segment para public 'START'
	org	100h
ComEntry:
start	ends

flex	segment para public 'FLEX'
	db	80 dup (0)              ; title, empty
	dd	0FFFF1A00h              ; Flex magic
	dd	2                       ; records
	dd	0CCh                    ; version
	dd	0, 0, 0
	dd	-1
	dd	100h
	dd	0, 0, 0, 0
	dd	90h, 4B7h               ; 0: MT32DRV, at and size
	dd	547h, 0FBCh             ; 1: ADLIBDRV
flex	ends
	end	ComEntry
