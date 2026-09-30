; Scaled and turned frame drawing: what the modules of the family share.
; scalfram.asm places a frame and holds the shared data; each draw module
; defines four of the draw routines.

FULL_SIZE       EQU 100h                ; a scale of 256ths
FULL_TURN       EQU 360

; A span: its length times two, plus 1 when it is run-encoded, then where it
; sits relative to the frame's hot spot. A length of 0 ends the frame.
SPANHDR STRUC
span_length     dw  ?
span_x          dw  ?
span_y          dw  ?
SPANHDR ENDS

; ClipEdges: the clip box edges a span crosses.
CLIP_LEFT       EQU 1
CLIP_TOP        EQU 2
CLIP_RIGHT      EQU 4
CLIP_BOTTOM     EQU 8

; FillGaps: which of a span drawer's two pixel loops to use.
PLAIN_LOOP      EQU 0
FILL_LOOP       EQU 2

; The draw routines are reached by a far jump with bp, si, di and ds pushed,
; ds:si at the frame's first span, es the view's segment and al the scale.
; They leave through this.
DRAW_EXIT MACRO
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
	ENDM

; The Place routines return carry set when the frame lies off the view.
	GLOBAL  FrameNeedsClip:FAR
	GLOBAL  PlaceFrame:FAR, PlaceFrameMirror:FAR, PlaceFrameFlip:FAR, PlaceFrameBoth:FAR
	GLOBAL  PlaceTurned:FAR, PlaceTurnedMirror:FAR, PlaceTurnedFlip:FAR, PlaceTurnedBoth:FAR
	GLOBAL  PlaceSideways:FAR, PlaceSidewaysMirror:FAR, PlaceSidewaysFlip:FAR, PlaceSidewaysBoth:FAR
	GLOBAL  PlaceTurnedBack:FAR, PlaceTurnedBackMirror:FAR, PlaceTurnedBackFlip:FAR, PlaceTurnedBackBoth:FAR
	GLOBAL  PlaceEnlarged:FAR, PlaceEnlargedMirror:FAR, PlaceEnlargedFlip:FAR, PlaceEnlargedBoth:FAR
	GLOBAL  PlaceEnlargedTurned:FAR, PlaceEnlargedTurnedMirror:FAR, PlaceEnlargedTurnedFlip:FAR, PlaceEnlargedTurnedBoth:FAR
	GLOBAL  PlaceEnlargedSideways:FAR, PlaceEnlargedSidewaysMirror:FAR, PlaceEnlargedSidewaysFlip:FAR, PlaceEnlargedSidewaysBoth:FAR
	GLOBAL  PlaceEnlargedTurnedBack:FAR, PlaceEnlargedTurnedBackMirror:FAR, PlaceEnlargedTurnedBackFlip:FAR, PlaceEnlargedTurnedBackBoth:FAR

; One draw routine for each size, angle and flip.
	GLOBAL  ShrinkFrame:FAR, ShrinkFrameMirror:FAR, ShrinkFrameFlip:FAR, ShrinkFrameBoth:FAR
	GLOBAL  ShrinkTurned:FAR, ShrinkTurnedMirror:FAR, ShrinkTurnedFlip:FAR, ShrinkTurnedBoth:FAR
	GLOBAL  ShrinkSideways:FAR, ShrinkSidewaysMirror:FAR, ShrinkSidewaysFlip:FAR, ShrinkSidewaysBoth:FAR
	GLOBAL  ShrinkTurnedBack:FAR, ShrinkTurnedBackMirror:FAR, ShrinkTurnedBackFlip:FAR, ShrinkTurnedBackBoth:FAR
	GLOBAL  EnlargeFrame:FAR, EnlargeFrameMirror:FAR, EnlargeFrameFlip:FAR, EnlargeFrameBoth:FAR
	GLOBAL  EnlargeTurned:FAR, EnlargeTurnedMirror:FAR, EnlargeTurnedFlip:FAR, EnlargeTurnedBoth:FAR
	GLOBAL  EnlargeSideways:FAR, EnlargeSidewaysMirror:FAR, EnlargeSidewaysFlip:FAR, EnlargeSidewaysBoth:FAR
	GLOBAL  EnlargeTurnedBack:FAR, EnlargeTurnedBackMirror:FAR, EnlargeTurnedBackFlip:FAR, EnlargeTurnedBackBoth:FAR

	.DATA
	GLOBAL  StepFraction:WORD, XFraction:BYTE, YFraction:BYTE, FillGaps:WORD
	GLOBAL  SpanRow:WORD, FrameWidth:WORD, FrameHeight:WORD, ScaledWidth:WORD, ScaledHeight:WORD
	GLOBAL  DrawLeft:WORD, DrawTop:WORD, DrawRight:WORD, DrawBottom:WORD
	GLOBAL  FrameRight:WORD, FrameLeft:WORD, FrameTop:WORD, FrameBottom:WORD
	GLOBAL  ScreenX:WORD, ScreenY:WORD, SpanStart:WORD, LineX:WORD, LineY:WORD
	GLOBAL  CosStep:WORD, SinStep:WORD, ScaledCos:BYTE, ScaledSin:BYTE
	GLOBAL  CosCount:BYTE, SinCount:BYTE, CosWhole:BYTE, SinWhole:BYTE
	GLOBAL  SpanWidth:WORD, CosFraction:BYTE, SinFraction:BYTE, CurrentScale:WORD
	GLOBAL  RowRepeat:BYTE, ClipEdges:BYTE, DriftX:WORD, DriftY:WORD
	GLOBAL  RunLength:WORD, ColumnsLeft:WORD, RowsLeft:WORD
	GLOBAL  ViewSeg:WORD, ViewRows:WORD, ClipLeft:WORD, ClipTop:WORD, ClipRight:WORD, ClipBottom:WORD
	GLOBAL  RowPitch:WORD, SpanBuffer:BYTE, SpanProc:WORD, StepProc:WORD
