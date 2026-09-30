; Black Gate INTRO.EXE, resident segment 77 (file offsets 0x01843e to 0x0194d0, 4242 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Draws a shape frame scaled and turned. DrawScaledFrame looks the frame up,
; works out where its scaled and turned box lands (a Place routine) and jumps
; to one of 32 draw routines in the other segments of this family, picked by
; size (shrunk or enlarged), angle (0, 1-89, 90 or 91-179 degrees) and flip.
; Angles of 180 and more are the same drawing with both flips.

	.MODEL  MEDIUM
	LOCALS

	EXTRN   C MapEmsPointer:FAR, C DrawFrame:FAR

	INCLUDE scalfram.inc

	PUBLIC  C GetScaledFrameBounds, C FillScaledFrameBounds, C DrawEmsScaledFrame, C DrawScaledFrame

	.DATA

; Sines of 0 to 359 degrees in 256ths (255 for 1), sign dropped; the cosine
; of an angle is its sine 90 degrees on.
CosTable        db  255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 251, 250, 250, 248, 247, 246, 245
				db  243, 242, 241, 239, 237, 236, 234, 232, 230, 228, 226, 224, 221, 219, 217, 215, 212, 210
				db  207, 204, 202, 198, 196, 193, 190, 187, 184, 181, 178, 175, 171, 168, 165, 161, 158, 154
				db  150, 147, 143, 139, 136, 131, 128, 124, 120, 116, 112, 108, 104, 100, 96, 92, 88, 83
				db  79, 74, 71, 66, 62, 58, 53, 49, 45, 40, 36, 31, 27, 22, 18, 13, 9, 4
SinTable        db  0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 45, 49, 53, 58, 62, 66, 71, 75
				db  79, 83, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 131, 136, 139, 143, 147
				db  150, 154, 158, 161, 165, 168, 171, 175, 178, 181, 184, 187, 190, 193, 196, 198, 202, 204
				db  207, 210, 212, 215, 217, 219, 221, 224, 226, 228, 230, 232, 234, 236, 237, 239, 241, 242
				db  243, 245, 246, 247, 248, 250, 250, 251, 252, 253, 254, 254, 255, 255, 255, 255, 255, 255
				db  255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 251, 250, 250, 248, 247, 246, 245
				db  243, 242, 241, 239, 237, 236, 234, 232, 230, 228, 226, 224, 221, 219, 217, 215, 212, 210
				db  207, 204, 202, 198, 196, 193, 190, 187, 184, 181, 178, 175, 171, 168, 165, 161, 158, 154
				db  150, 147, 143, 139, 136, 131, 128, 124, 120, 116, 112, 108, 104, 100, 96, 92, 88, 83
				db  79, 74, 71, 66, 62, 58, 53, 49, 45, 40, 36, 31, 27, 22, 18, 13, 9, 4
				db  0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 45, 49, 53, 58, 62, 66, 71, 75
				db  79, 83, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 131, 136, 139, 143, 147
				db  150, 154, 158, 161, 165, 168, 171, 175, 178, 181, 184, 187, 190, 193, 196, 198, 202, 204
				db  207, 210, 212, 215, 217, 219, 221, 224, 226, 228, 230, 232, 234, 236, 237, 239, 241, 242
				db  243, 245, 246, 247, 248, 250, 250, 251, 252, 253, 254, 254, 255, 255, 255, 255, 255, 255
				db  255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 251, 250, 250, 248, 247, 246, 245
				db  243, 242, 241, 239, 237, 236, 234, 232, 230, 228, 226, 224, 221, 219, 217, 215, 212, 210
				db  207, 204, 202, 198, 196, 193, 190, 187, 184, 181, 178, 175, 171, 168, 165, 161, 158, 154
				db  150, 147, 143, 139, 136, 131, 128, 124, 120, 116, 112, 108, 104, 100, 96, 92, 88, 83
				db  79, 74, 71, 66, 62, 58, 53, 49, 45, 40, 36, 31, 27, 22, 18, 13, 9, 4
; Which quarter an angle falls in: 0 upright, 2 turned, 4 sideways, 6 turned back.
QuarterTable    db  0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
				db  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
				db  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
				db  4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
				db  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
				db  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
				db  0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
				db  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
				db  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
				db  4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
				db  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
				db  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
; From 180 degrees on, the drawing of the angle less 180 with both flips.
HalfTurnTable   db  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
				db  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
				db  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
				db  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
				db  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
				db  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				db  30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h, 30h
				dw  ?
StepFraction    dw  ?
XFraction       db  ?
YFraction       db  ?
				dw  ?
FillGaps        dw  ?
; Rows of eight, one row per flip: shrunk, then enlarged, each upright,
; turned, sideways and turned back.
DrawRoutines    dd  ShrinkFrame, ShrinkTurned, ShrinkSideways, ShrinkTurnedBack
				dd  EnlargeFrame, EnlargeTurned, EnlargeSideways, EnlargeTurnedBack
				dd  ShrinkFrameMirror, ShrinkTurnedMirror, ShrinkSidewaysMirror, ShrinkTurnedBackMirror
				dd  EnlargeFrameMirror, EnlargeTurnedMirror, EnlargeSidewaysMirror, EnlargeTurnedBackMirror
				dd  ShrinkFrameFlip, ShrinkTurnedFlip, ShrinkSidewaysFlip, ShrinkTurnedBackFlip
				dd  EnlargeFrameFlip, EnlargeTurnedFlip, EnlargeSidewaysFlip, EnlargeTurnedBackFlip
				dd  ShrinkFrameBoth, ShrinkTurnedBoth, ShrinkSidewaysBoth, ShrinkTurnedBackBoth
				dd  EnlargeFrameBoth, EnlargeTurnedBoth, EnlargeSidewaysBoth, EnlargeTurnedBackBoth
PlaceRoutines   dw  PlaceFrame, PlaceTurned, PlaceSideways, PlaceTurnedBack
				dw  PlaceEnlarged, PlaceEnlargedTurned, PlaceEnlargedSideways, PlaceEnlargedTurnedBack
				dw  PlaceFrameMirror, PlaceTurnedMirror, PlaceSidewaysMirror, PlaceTurnedBackMirror
				dw  PlaceEnlargedMirror, PlaceEnlargedTurnedMirror, PlaceEnlargedSidewaysMirror
				dw  PlaceEnlargedTurnedBackMirror
				dw  PlaceFrameFlip, PlaceTurnedFlip, PlaceSidewaysFlip, PlaceTurnedBackFlip
				dw  PlaceEnlargedFlip, PlaceEnlargedTurnedFlip, PlaceEnlargedSidewaysFlip, PlaceEnlargedTurnedBackFlip
				dw  PlaceFrameBoth, PlaceTurnedBoth, PlaceSidewaysBoth, PlaceTurnedBackBoth
				dw  PlaceEnlargedBoth, PlaceEnlargedTurnedBoth, PlaceEnlargedSidewaysBoth, PlaceEnlargedTurnedBackBoth
				dw  ?
SpanRow         dw  ?                   ; the frame's top row, from its hot spot
FrameWidth      dw  ?
FrameHeight     dw  ?
ScaledWidth     dw  ?
ScaledHeight    dw  ?
DrawLeft        dw  ?                   ; the scaled frame's box on the view
DrawTop         dw  ?
DrawRight       dw  ?
DrawBottom      dw  ?
FrameRight      dw  ?                   ; the frame's extent around its hot spot
FrameLeft       dw  ?
FrameTop        dw  ?
FrameBottom     dw  ?
ScreenX         dw  ?
ScreenY         dw  ?
SpanStart       dw  ?
LineX           dw  ?
LineY           dw  ?
CosStep         dw  ?
SinStep         dw  ?
ScaledCos       db  ?
ScaledSin       db  ?
CosCount        db  ?
SinCount        db  ?
CosWhole        db  ?
SinWhole        db  ?
SpanWidth       dw  ?
CosFraction     db  ?
SinFraction     db  ?
CurrentScale    dw  ?
				db  ?
RowRepeat       db  ?
ClipEdges       db  ?                   ; 1 left, 2 top, 4 right, 8 bottom
DriftX          dw  ?
DriftY          dw  ?
RunLength       dw  ?
				dw  ?, ?
ColumnsLeft     dw  ?
RowsLeft        dw  ?
; A copy of the view drawn on.
ViewSeg         dw  ?
ViewRows        dw  ?
ClipLeft        dw  ?
ClipTop         dw  ?
ClipRight       dw  ?
ClipBottom      dw  ?
RowPitch        dw  ?
				db  ?
SpanBuffer      db  320 dup (?)
SpanProc        dw  ?
StepProc        dw  ?
				dw  ?, ?
FillColor       db  ?

	.CODE

; Carry set when the scaled frame's box crosses the clip rectangle, so
; drawing it needs clipping.
FrameNeedsClip PROC FAR
	mov     ax, ss:DrawLeft
	cmp     ax, ss:ClipLeft
	jl      @@crosses
	mov     ax, ss:DrawRight
	cmp     ax, ss:ClipRight
	jg      @@crosses
	mov     ax, ss:DrawTop
	cmp     ax, ss:ClipTop
	jl      @@crosses
	mov     ax, ss:DrawBottom
	cmp     ax, ss:ClipBottom
	jg      @@crosses
	clc
	ret
@@crosses:
	stc
	ret
FrameNeedsClip ENDP

; Copies the view, finds the frame and reads its extents, then jumps to the
; Place routine for the scale, angle and flip, which returns to the caller.
; Runs in its caller's frame. Carry set when the frame does not exist or
; lands off the view.
PrepareFrame PROC FAR
	mov     ax, ds
	mov     es, ax
	mov     si, view
	mov     di, offset ViewSeg
	mov     cx, 6                   ; the view's six words
	rep movsw
	mov     si, ClipTop
	shl     si, 1
	add     si, ss:ViewRows
	mov     ax, word ptr [si + 2]
	sub     ax, word ptr [si]
	mov     RowPitch, ax
	lds     si, shape
	mov     bx, ss:frame
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     word ptr [si + 4], bx
	jb      @@looked
	stc
	je      @@looked
; ds:si at the frame, normalised so si is below 16
	mov     ax, ds
	mov     dx, 0
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	add     ax, si
	adc     dx, 0
	add     ax, word ptr [bx + si]
	adc     dx, word ptr [bx + si + 2]
	mov     si, ax
	and     si, 0fh
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	mov     ds, ax
; its extents around the hot spot
	lodsw
	mov     ss:FrameRight, ax
	mov     di, ax
	lodsw
	mov     ss:FrameLeft, ax
	stc
	adc     ax, di
	mov     ss:FrameWidth, ax
	lodsw
	mov     ss:FrameTop, ax
	mov     di, ax
	neg     ax
	mov     ss:SpanRow, ax
	lodsw
	mov     ss:FrameBottom, ax
	stc
	adc     ax, di
	mov     ss:FrameHeight, ax
	mov     ax, ss:ViewSeg
	mov     es, ax
	clc
@@looked:
	jb      @@done
	mov     bx, angle
	or      bx, bx
	jns     @@positive
	add     bx, FULL_TURN
@@positive:
	mov     cl, ss:CosTable[bx]
	mov     ss:ScaledCos, cl
	mov     ch, ss:SinTable[bx]
	mov     ss:ScaledSin, ch
; the routine for the angle's quarter, its half turn and the flip
	mov     al, ss:QuarterTable[bx]
	add     al, ss:HalfTurnTable[bx]
	xor     al, flip
	cbw
	mov     di, ax
	mov     ax, scale
	mov     ss:CurrentScale, ax
	cmp     ax, FULL_SIZE
	jae     @@enlarged
	jmp     ss:PlaceRoutines[di]
@@enlarged:
	add     di, 8
	jmp     ss:PlaceRoutines[di]
@@done:
	ret
PrepareFrame ENDP

; Shrunk and upright. al is the scale; ax and cx are the frame's left and top
; extents, or the opposite sides when flipped. Sets the box on the view and
; ScreenX and ScreenY, its corner; carry set when it lies off the clip
; rectangle. Returns bx and bp at that corner.
PlaceFrame PROC FAR
	mov     dh, al
	mov     ax, ss:FrameLeft
	mov     cx, ss:FrameTop
	jmp     short PlaceFrameAt
PlaceFrame ENDP

PlaceFrameMirror PROC FAR
	mov     dh, al
	mov     ax, ss:FrameRight
	mov     cx, ss:FrameTop
	jmp     short PlaceFrameAt
PlaceFrameMirror ENDP

PlaceFrameFlip PROC FAR
	mov     dh, al
	mov     ax, ss:FrameLeft
	mov     cx, ss:FrameBottom
	jmp     short PlaceFrameAt
PlaceFrameFlip ENDP

PlaceFrameBoth PROC FAR
	mov     dh, al
	mov     ax, ss:FrameRight
	mov     cx, ss:FrameBottom

PlaceFrameAt:
	mov     bx, x
	mul     dh
	neg     al
	mov     ss:XFraction, al
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ss:ScreenX, bx
	mov     ss:DrawLeft, bx
	cmp     bx, ss:ClipRight
	jg      @@outside
	mov     bp, y
	mov     ax, cx
	mul     dh
	neg     al
	mov     ss:YFraction, al
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ax, bp
	mov     ss:ScreenY, ax
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:FrameWidth
	mul     dh
	add     al, ss:XFraction
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	inc     ax
	mov     ss:ScaledWidth, ax
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     dh
	add     al, ss:YFraction
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	inc     ax
	mov     ss:ScaledHeight, ax
	add     ax, bp
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceFrameBoth ENDP

; Shrunk and turned 90 degrees: the frame's height runs across the view.
PlaceSideways PROC FAR
	mov     dh, al
	mov     ax, ss:FrameBottom
	mov     cx, ss:FrameLeft
	jmp     short PlaceSidewaysAt
PlaceSideways ENDP

PlaceSidewaysMirror PROC FAR
	mov     dh, al
	mov     ax, ss:FrameTop
	mov     cx, ss:FrameLeft
	jmp     short PlaceSidewaysAt
PlaceSidewaysMirror ENDP

PlaceSidewaysFlip PROC FAR
	mov     dh, al
	mov     ax, ss:FrameBottom
	mov     cx, ss:FrameRight
	jmp     short PlaceSidewaysAt
PlaceSidewaysFlip ENDP

PlaceSidewaysBoth PROC FAR
	mov     dh, al
	mov     ax, ss:FrameTop
	mov     cx, ss:FrameRight

PlaceSidewaysAt:
	mov     bx, x
	mul     dh
	neg     al
	mov     ss:XFraction, al
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ss:ScreenX, bx
	mov     ss:DrawLeft, bx
	cmp     bx, ss:ClipRight
	jg      @@outside
	mov     bp, y
	mov     ax, cx
	mul     dh
	neg     al
	mov     ch, al
	mov     ss:YFraction, al
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ss:ScreenY, bp
	mov     ss:DrawTop, bp
	cmp     bp, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:FrameWidth
	mul     dh
	add     al, ss:YFraction
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	inc     ax
	mov     ss:ScaledHeight, ax
	add     ax, bp
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     dh
	add     al, ss:XFraction
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	inc     ax
	mov     ss:ScaledWidth, ax
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceSidewaysBoth ENDP

; Scales the angle's cosine and sine (cl, ch) by the scale in al: CosStep
; and SinStep, never 0, and the size of the turned frame's box. Returns bx
; and bp at x and y.
TurnSteps PROC FAR
	mov     dl, al
	mov     al, cl
	mul     dl
	mov     al, ah
	xor     ah, ah
	mov     cl, al
	or      ax, ax
	jne     @@cosStep
	inc     ax
@@cosStep:
	mov     ss:CosStep, ax
	mov     al, ch
	mul     dl
	mov     al, ah
	xor     ah, ah
	mov     ch, al
	or      ax, ax
	jne     @@sinStep
	inc     ax
@@sinStep:
	mov     ss:SinStep, ax
	mov     word ptr ss:ScaledCos, cx
	mov     ax, ss:FrameHeight
	mul     cl
	mov     di, ax
	mov     ax, ss:FrameWidth
	mul     ch
	add     ax, di
	neg     al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	inc     ax
	mov     ss:ScaledHeight, ax
	mov     ax, ss:FrameHeight
	mul     ch
	mov     di, ax
	mov     ax, ss:FrameWidth
	mul     cl
	add     ax, di
	neg     al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	inc     ax
	mov     ss:ScaledWidth, ax
	mov     bx, x
	mov     bp, y
	ret
TurnSteps ENDP

; Shrunk and turned 1 to 89 degrees: the box around the turned frame, whose
; corner is the hot spot less the turned left and top extents.
PlaceTurned PROC FAR
	call    TurnSteps
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ch
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     cl
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     cl
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     ch
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ss:DrawTop, bp
	cmp     bp, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:ScaledHeight
	add     ax, bp
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     ch
	mov     al, ah
	xor     ah, ah
	inc     ax
	neg     ax
	add     ax, bx
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	add     ax, ss:ScaledWidth
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceTurned ENDP

PlaceTurnedMirror PROC FAR
	call    TurnSteps
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ch
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     cl
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     cl
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     ch
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ss:DrawRight, bx
	cmp     bx, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:ScaledWidth
	neg     ax
	add     ax, bx
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ax, ss:FrameWidth
	mul     ch
	mov     al, ah
	xor     ah, ah
	inc     ax
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	add     ax, ss:ScaledHeight
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceTurnedMirror ENDP

PlaceTurnedFlip PROC FAR
	call    TurnSteps
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ch
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     cl
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     cl
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     ch
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ss:DrawLeft, bx
	cmp     bx, ss:ClipRight
	jg      @@outside
	mov     ax, ss:ScaledWidth
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     cl
	mov     al, ah
	xor     ah, ah
	inc     ax
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	add     ax, ss:ScaledHeight
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceTurnedFlip ENDP

PlaceTurnedBoth PROC FAR
	call    TurnSteps
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ch
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     cl
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     cl
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     ch
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ss:DrawBottom, bp
	cmp     bp, ss:ClipTop
	jl      @@outside
	mov     ax, ss:ScaledHeight
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:FrameHeight
	mul     ch
	mov     al, ah
	xor     ah, ah
	inc     ax
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	sub     ax, ss:ScaledWidth
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceTurnedBoth ENDP

; Shrunk and turned 91 to 179 degrees.
PlaceTurnedBack PROC FAR
	call    TurnSteps
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ch
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     cl
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     cl
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     ch
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ss:DrawRight, bx
	cmp     bx, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:ScaledWidth
	neg     ax
	add     ax, bx
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ax, ss:FrameHeight
	mul     cl
	mov     al, ah
	xor     ah, ah
	inc     ax
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	add     ax, ss:ScaledHeight
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceTurnedBack ENDP

PlaceTurnedBackMirror PROC FAR
	call    TurnSteps
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ch
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     cl
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     cl
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     ch
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ss:DrawBottom, bp
	cmp     bp, ss:ClipTop
	jl      @@outside
	mov     ax, ss:ScaledHeight
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:FrameWidth
	mul     cl
	mov     al, ah
	xor     ah, ah
	inc     ax
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	sub     ax, ss:ScaledWidth
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceTurnedBackMirror ENDP

PlaceTurnedBackFlip PROC FAR
	call    TurnSteps
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ch
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     cl
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     cl
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     ch
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ss:DrawTop, bp
	cmp     bp, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:ScaledHeight
	add     ax, bp
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     ch
	mov     al, ah
	xor     ah, ah
	inc     ax
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	sub     ax, ss:ScaledWidth
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceTurnedBackFlip ENDP

PlaceTurnedBackBoth PROC FAR
	call    TurnSteps
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ch
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     cl
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     cl
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     ch
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ss:DrawLeft, bx
	cmp     bx, ss:ClipRight
	jg      @@outside
	mov     ax, ss:ScaledWidth
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:FrameWidth
	mul     ch
	mov     al, ah
	xor     ah, ah
	inc     ax
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	add     ax, ss:ScaledHeight
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceTurnedBackBoth ENDP

; Enlarged and upright: as PlaceFrame with the whole scale in ax.
PlaceEnlarged PROC FAR
	mov     bx, ax
	mov     ax, ss:FrameLeft
	mov     cx, ss:FrameTop
	jmp     short PlaceEnlargedAt
PlaceEnlarged ENDP

PlaceEnlargedMirror PROC FAR
	mov     bx, ax
	mov     ax, ss:FrameRight
	mov     cx, ss:FrameTop
	jmp     short PlaceEnlargedAt
PlaceEnlargedMirror ENDP

PlaceEnlargedFlip PROC FAR
	mov     bx, ax
	mov     ax, ss:FrameLeft
	mov     cx, ss:FrameBottom
	jmp     short PlaceEnlargedAt
PlaceEnlargedFlip ENDP

PlaceEnlargedBoth PROC FAR
	mov     bx, ax
	mov     ax, ss:FrameRight
	mov     cx, ss:FrameBottom

PlaceEnlargedAt:
	mul     bx
	neg     al
	mov     ss:XFraction, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, x
	mov     ss:ScreenX, ax
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ax, cx
	mul     bx
	neg     al
	mov     ss:YFraction, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, y
	mov     ss:ScreenY, ax
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:FrameWidth
	mul     bx
	add     al, ss:XFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	dec     ax
	jl      @@outside
	add     ax, ss:ScreenX
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     bx
	add     al, ss:YFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	dec     ax
	jl      @@outside
	add     ax, ss:ScreenY
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedBoth ENDP

; TurnSteps for scales of 256 and more: 8.8 steps, whose whole parts also go
; to CosCount, SinCount, CosWhole and SinWhole.
TurnStepsEnlarged PROC FAR
	mov     di, ax
	xor     ax, ax
	mov     al, cl
	mul     di
	mov     al, ah
	mov     ah, dl
	mov     ss:CosStep, ax
	mov     ss:CosCount, ah
	mov     ss:CosWhole, ah
	xor     ax, ax
	mov     al, ch
	mul     di
	mov     al, ah
	mov     ah, dl
	mov     ss:SinStep, ax
	mov     ss:SinCount, ah
	mov     ss:SinWhole, ah
	mov     ax, ss:FrameHeight
	mul     ss:CosStep
	mov     di, dx
	mov     bx, ax
	mov     ax, ss:FrameWidth
	mul     ss:SinStep
	add     ax, bx
	adc     dx, di
	neg     al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     ss:ScaledHeight, ax
	mov     ax, ss:FrameHeight
	mul     ss:SinStep
	mov     di, dx
	mov     bx, ax
	mov     ax, ss:FrameWidth
	mul     ss:CosStep
	add     ax, bx
	adc     dx, di
	neg     al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     ss:ScaledWidth, ax
	mov     bx, x
	mov     bp, y
	ret
TurnStepsEnlarged ENDP

; Enlarged and turned 1 to 89 degrees.
PlaceEnlargedTurned PROC FAR
	call    TurnStepsEnlarged
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	add     bx, ax
	mov     ax, di
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	sub     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	sub     bx, ax
	mov     ax, di
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	sub     bp, ax
	mov     ss:DrawTop, bp
	cmp     bp, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:ScaledHeight
	add     ax, bp
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	inc     ax
	neg     ax
	add     ax, bx
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	add     ax, ss:ScaledWidth
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	mov     ss:ScreenX, bx
	mov     ss:ScreenY, bp
	mov     bx, word ptr ss:ScaledCos
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedTurned ENDP

PlaceEnlargedTurnedMirror PROC FAR
	call    TurnStepsEnlarged
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	add     bx, ax
	mov     ax, di
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	sub     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	add     bx, ax
	mov     ax, di
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	add     bp, ax
	mov     ss:DrawRight, bx
	cmp     bx, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:ScaledWidth
	neg     ax
	add     ax, bx
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ax, ss:FrameWidth
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	inc     ax
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	add     ax, ss:ScaledHeight
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ss:ScreenX, bx
	mov     ss:ScreenY, bp
	mov     bx, word ptr ss:ScaledCos
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedTurnedMirror ENDP

PlaceEnlargedTurnedFlip PROC FAR
	call    TurnStepsEnlarged
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	sub     bx, ax
	mov     ax, di
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	add     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	sub     bx, ax
	mov     ax, di
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	sub     bp, ax
	mov     ss:DrawLeft, bx
	cmp     bx, ss:ClipRight
	jg      @@outside
	mov     ax, ss:ScaledWidth
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	inc     ax
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	add     ax, ss:ScaledHeight
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ss:ScreenX, bx
	mov     ss:ScreenY, bp
	mov     bx, word ptr ss:ScaledCos
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedTurnedFlip ENDP

PlaceEnlargedTurnedBoth PROC FAR
	call    TurnStepsEnlarged
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	sub     bx, ax
	mov     ax, di
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	add     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	add     bx, ax
	mov     ax, di
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	add     bp, ax
	mov     ss:DrawBottom, bp
	cmp     bp, ss:ClipTop
	jl      @@outside
	mov     ax, ss:ScaledHeight
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:FrameHeight
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	inc     ax
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	sub     ax, ss:ScaledWidth
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ss:ScreenX, bx
	mov     ss:ScreenY, bp
	mov     bx, word ptr ss:ScaledCos
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedTurnedBoth ENDP

; Enlarged and turned 90 degrees.
PlaceEnlargedSideways PROC FAR
	mov     bx, ax
	mov     ax, ss:FrameBottom
	mov     cx, ss:FrameLeft
	jmp     short PlaceEnlargedSidewaysAt
PlaceEnlargedSideways ENDP

PlaceEnlargedSidewaysMirror PROC FAR
	mov     bx, ax
	mov     ax, ss:FrameTop
	mov     cx, ss:FrameLeft
	jmp     short PlaceEnlargedSidewaysAt
PlaceEnlargedSidewaysMirror ENDP

PlaceEnlargedSidewaysFlip PROC FAR
	mov     bx, ax
	mov     ax, ss:FrameBottom
	mov     cx, ss:FrameRight
	jmp     short PlaceEnlargedSidewaysAt
PlaceEnlargedSidewaysFlip ENDP

PlaceEnlargedSidewaysBoth PROC FAR
	mov     bx, ax
	mov     ax, ss:FrameTop
	mov     cx, ss:FrameRight

PlaceEnlargedSidewaysAt:
	inc     ax
	mul     bx
	mov     ss:XFraction, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, x
	mov     ss:ScreenX, ax
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ax, cx
	mul     bx
	mov     ss:YFraction, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, y
	mov     ss:ScreenY, ax
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:FrameHeight
	mul     bx
	add     al, ss:XFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	dec     ax
	jl      @@outside
	add     ax, ss:ScreenX
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:FrameWidth
	mul     bx
	add     al, ss:YFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	dec     ax
	jl      @@outside
	add     ax, ss:ScreenY
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedSidewaysBoth ENDP

; Enlarged and turned 91 to 179 degrees.
PlaceEnlargedTurnedBack PROC FAR
	call    TurnStepsEnlarged
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	add     bx, ax
	mov     ax, di
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	add     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	add     bx, ax
	mov     ax, di
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	sub     bp, ax
	mov     ss:DrawRight, bx
	cmp     bx, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:ScaledWidth
	neg     ax
	add     ax, bx
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ax, ss:FrameHeight
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	inc     ax
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	add     ax, ss:ScaledHeight
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ss:ScreenX, bx
	mov     ss:ScreenY, bp
	mov     bx, word ptr ss:ScaledCos
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedTurnedBack ENDP

PlaceEnlargedTurnedBackMirror PROC FAR
	call    TurnStepsEnlarged
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	add     bx, ax
	mov     ax, di
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	add     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	sub     bx, ax
	mov     ax, di
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	add     bp, ax
	mov     ss:DrawBottom, bp
	cmp     bp, ss:ClipTop
	jl      @@outside
	mov     ax, ss:ScaledHeight
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:FrameWidth
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	inc     ax
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	sub     ax, ss:ScaledWidth
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ss:ScreenX, bx
	mov     ss:ScreenY, bp
	mov     bx, word ptr ss:ScaledCos
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedTurnedBackMirror ENDP

PlaceEnlargedTurnedBackFlip PROC FAR
	call    TurnStepsEnlarged
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	sub     bx, ax
	mov     ax, di
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	sub     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	add     bx, ax
	mov     ax, di
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	sub     bp, ax
	mov     ss:DrawTop, bp
	cmp     bp, ss:ClipBottom
	jg      @@outside
	mov     ax, ss:ScaledHeight
	add     ax, bp
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ax, ss:FrameHeight
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	inc     ax
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	sub     ax, ss:ScaledWidth
	mov     ss:DrawLeft, ax
	cmp     ax, ss:ClipRight
	jg      @@outside
	mov     ss:ScreenX, bx
	mov     ss:ScreenY, bp
	mov     bx, word ptr ss:ScaledCos
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedTurnedBackFlip ENDP

PlaceEnlargedTurnedBackBoth PROC FAR
	call    TurnStepsEnlarged
	mov     ax, ss:FrameTop
	mov     di, ax
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	sub     bx, ax
	mov     ax, di
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	sub     bp, ax
	mov     ax, ss:FrameLeft
	mov     di, ax
	mul     ss:CosStep
	mov     al, ah
	mov     ah, dl
	sub     bx, ax
	mov     ax, di
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	add     bp, ax
	mov     ss:DrawLeft, bx
	cmp     bx, ss:ClipRight
	jg      @@outside
	mov     ax, ss:ScaledWidth
	add     ax, bx
	mov     ss:DrawRight, ax
	cmp     ax, ss:ClipLeft
	jl      @@outside
	mov     ax, ss:FrameWidth
	mul     ss:SinStep
	mov     al, ah
	mov     ah, dl
	inc     ax
	neg     ax
	add     ax, bp
	mov     ss:DrawTop, ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	add     ax, ss:ScaledHeight
	mov     ss:DrawBottom, ax
	cmp     ax, ss:ClipTop
	jl      @@outside
	mov     ss:ScreenX, bx
	mov     ss:ScreenY, bp
	mov     bx, word ptr ss:ScaledCos
	clc
	ret
@@outside:
	stc
	ret
PlaceEnlargedTurnedBackBoth ENDP

; The box DrawScaledFrame would draw in, for a shape that may live in
; expanded memory. Returns 1, or 0 when nothing would be drawn.
GetScaledFrameBounds PROC C FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frame:WORD, angle:WORD, scale:WORD, flip:BYTE, bounds:WORD
	push    word ptr shape+2
	push    word ptr shape
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	je      @@done
	mov     word ptr shape, ax
	mov     word ptr shape+2, dx
	push    bounds
	push    word ptr flip
	push    scale
	push    angle
	push    frame
	push    word ptr shape+2
	push    word ptr shape
	push    y
	push    x
	push    view
	call    far ptr ScaledFrameBounds
	add     sp, 20
@@done:
	ret
GetScaledFrameBounds ENDP

ScaledFrameBounds PROC C FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frame:WORD, angle:WORD, scale:WORD, flip:BYTE, bounds:WORD
	USES    di, ds
	push    ds
	mov     ax, bounds
	push    ax
	call    PrepareFrame
	pop     ax
	pop     ds
	jb      @@missed
	mov     di, ax
	mov     bx, 6
@@copy:
	mov     ax, ss:DrawLeft[bx]
	mov     word ptr [bx + di], ax
	sub     bx, 2
	jge     @@copy
	mov     ax, 1
	jmp     short @@done
@@missed:
	mov     ax, 0
@@done:
	ret
ScaledFrameBounds ENDP

; Fills the box a scaled frame covers, clipped to the view, with a color.
FillScaledFrameBounds PROC C FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frame:WORD, angle:WORD, scale:WORD, flip:BYTE, color:BYTE
	push    word ptr shape+2
	push    word ptr shape
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	je      @@done
	mov     word ptr shape, ax
	mov     word ptr shape+2, dx
	push    word ptr color
	push    word ptr flip
	push    scale
	push    angle
	push    frame
	push    word ptr shape+2
	push    word ptr shape
	push    y
	push    x
	push    view
	call    far ptr FillScaledBounds
	add     sp, 20
@@done:
	ret
FillScaledFrameBounds ENDP

FillScaledBounds PROC C FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frame:WORD, angle:WORD, scale:WORD, flip:BYTE, color:BYTE
	USES    si, di, ds
	mov     ax, word ptr color
	mov     ss:FillColor, al
	call    PrepareFrame
	jb      @@done
	mov     bx, ss:DrawLeft
	mov     ax, ss:DrawRight
	cmp     ax, ss:ClipRight
	jle     @@right
	mov     ax, ss:ClipRight
@@right:
	cmp     bx, ss:ClipLeft
	jge     @@width
	mov     bx, ss:ClipLeft
@@width:
	sub     ax, bx
	inc     ax
	mov     dx, ax
	mov     bp, ss:DrawTop
	mov     ax, ss:DrawBottom
	cmp     ax, ss:ClipBottom
	jle     @@bottom
	mov     ax, ss:ClipBottom
@@bottom:
	cmp     bp, ss:ClipTop
	jge     @@height
	mov     bp, ss:ClipTop
@@height:
	sub     ax, bp
	inc     ax
	mov     si, ax
	shl     bp, 1
	add     bp, ss:ViewRows
	mov     al, ss:FillColor
	mov     ah, al
@@row:
	mov     di, word ptr [bp]
	add     di, bx
	mov     cx, dx
	shr     cx, 1
	rep stosw
	rcl     cx, 1
	rep stosb
	add     bp, 2
	dec     si
	jg      @@row
@@done:
	ret
FillScaledBounds ENDP

; DrawScaledFrame for a shape that may live in expanded memory.
DrawEmsScaledFrame PROC C FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frame:WORD, angle:WORD, scale:WORD, flip:BYTE
	push    bp
	mov     bp, sp
	push    word ptr shape+2
	push    word ptr shape
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	je      @@done
	mov     word ptr shape, ax
	mov     word ptr shape+2, dx
	pop     bp
	jmp     far ptr DrawScaledFrame
@@done:
	pop     bp
	ret
DrawEmsScaledFrame ENDP

; Draws a frame scaled (in 256ths) and turned (in degrees). Full size and
; unturned is a plain DrawFrame; otherwise a Place routine sets up and a draw
; routine finishes, returning to the caller.
DrawScaledFrame PROC C FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frame:WORD, angle:WORD, scale:WORD, flip:BYTE
	USES    si, di, ds
	cmp     scale, FULL_SIZE
	jne     @@scaled
	mov     ax, angle
	or      al, flip
	jne     @@scaled
	pop     ds
	pop     di
	pop     si
	pop     bp
	jmp    DrawFrame
@@scaled:
	mov     ax, ds
	mov     es, ax
	mov     si, view
	mov     di, offset ViewSeg
	mov     cx, 6                   ; the view's six words
	rep movsw
	mov     si, ClipTop
	shl     si, 1
	add     si, ss:ViewRows
	mov     ax, word ptr [si + 2]
	sub     ax, word ptr [si]
	mov     RowPitch, ax
	lds     si, shape
	mov     bx, ss:frame
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     word ptr [si + 4], bx
	jb      @@looked
	stc
	je      @@looked
; ds:si at the frame, normalised so si is below 16
	mov     ax, ds
	mov     dx, 0
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	add     ax, si
	adc     dx, 0
	add     ax, word ptr [bx + si]
	adc     dx, word ptr [bx + si + 2]
	mov     si, ax
	and     si, 0fh
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	mov     ds, ax
; its extents around the hot spot
	lodsw
	mov     ss:FrameRight, ax
	mov     di, ax
	lodsw
	mov     ss:FrameLeft, ax
	stc
	adc     ax, di
	mov     ss:FrameWidth, ax
	lodsw
	mov     ss:FrameTop, ax
	mov     di, ax
	neg     ax
	mov     ss:SpanRow, ax
	lodsw
	mov     ss:FrameBottom, ax
	stc
	adc     ax, di
	mov     ss:FrameHeight, ax
	mov     ax, ss:ViewSeg
	mov     es, ax
	clc
@@looked:
	jae     @@found
	ret
@@found:
	mov     bx, angle
	or      bx, bx
	jns     @@positive
	add     bx, FULL_TURN
@@positive:
	mov     cl, ss:CosTable[bx]
	mov     ss:ScaledCos, cl
	mov     ch, ss:SinTable[bx]
	mov     ss:ScaledSin, ch
; the routine for the angle's quarter, its half turn and the flip
	mov     al, ss:QuarterTable[bx]
	add     al, ss:HalfTurnTable[bx]
	xor     al, flip
	cbw
	mov     di, ax
	mov     ax, scale
	mov     ss:CurrentScale, ax
	cmp     ax, FULL_SIZE
	jae     @@enlarged
	shl     di, 1
	jmp     ss:DrawRoutines[di]     ; bp, si, di and ds still pushed
@@enlarged:
	add     di, 8
	shl     di, 1
	jmp     ss:DrawRoutines[di]
DrawScaledFrame ENDP

	END
