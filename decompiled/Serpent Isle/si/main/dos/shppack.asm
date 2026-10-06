; Serpent Isle SI.EXE, resident segment 140 (file offsets 0x03eaf8 to 0x03ec33, 315 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _PackShapeFrames

	.CODE

; Packs a cached shape (linear address) down to the frames a bit mask (linear address) keeps,
; closing the gaps and rewriting the frame offsets. Returns the new size in dx:ax.
_PackShapeFrames    PROC FAR
	ARG     shape:DWORD, keep:DWORD
	LOCAL   newSize:DWORD, shapeBase:DWORD = frameSize
	enter   frameSize, 0
	push    edi
	push    esi
	push    ds
	push    es
	pushf
	cld
	mov     edi, shape
	mov     shapeBase, edi
	mov     ebx, keep
	xor     eax, eax
	mov     ds, ax
	mov     es, ax
	mov     eax, dword ptr [edi+4]      ; five loads nothing uses
	mov     eax, dword ptr [edi+8]
	mov     eax, dword ptr [edi+12]
	mov     eax, dword ptr [edi+16]
	mov     eax, dword ptr [edi+20]
	xor     edx, edx
	mov     edx, dword ptr [ebx]        ; one keep bit per frame
	mov     ebx, dword ptr [edi]        ; the shape's size
	mov     newSize, ebx
	mov     eax, dword ptr [edi+4]      ; frames: the first offset / 4, less the size
	shr     eax, 2
	dec     eax
	mov     ecx, eax
	mov     eax, dword ptr [edi+4]
	sub     ebx, eax
	cmp     ecx, 1
	jle     @@done
@@nextFrame:
	cmp     ecx, 1
	je      @@lastFrame
	add     edi, 4
	push    edi
	mov     esi, dword ptr [edi+4]
	mov     edi, dword ptr [edi]
	add     esi, shapeBase
	add     edi, shapeBase
	mov     eax, esi
	sub     eax, edi
	sub     ebx, eax
	shr     edx, 1
	push    ecx
	jb      @@frameKept
	sub     newSize, eax
	mov     ecx, ebx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	mov     ecx, ebx
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	pop     ecx
	pop     edi
	push    edi
	push    ecx
	push    ebx
	xor     ebx, ebx
	mov     dword ptr [edi], ebx
	dec     ecx
	jcxz    @@offsetsFixed
@@fixOffset:
	add     edi, 4
	mov     ebx, dword ptr [edi]
	sub     ebx, eax
	mov     dword ptr [edi], ebx
	dec     ecx
	jne     @@fixOffset
@@offsetsFixed:
	mov     eax, newSize
	mov     ebx, shapeBase
	mov     dword ptr [ebx], eax
	pop     ebx
@@frameKept:
	pop     ecx
	pop     edi
	dec     ecx
	jne     @@nextFrame
	jmp     @@done
@@lastFrame:
	add     edi, 4
	mov     esi, shapeBase
	push    esi
	push    edi
	mov     edi, dword ptr [edi]
	mov     esi, dword ptr [esi]
	mov     eax, esi
	sub     eax, edi
	shr     edx, 1
	pop     edi
	jb      @@lastKept
	sub     newSize, eax
	xor     eax, eax
	mov     dword ptr [edi], eax
@@lastKept:
	pop     edi
	mov     eax, newSize
	mov     dword ptr [edi], eax
@@done:
	mov     eax, newSize
	push    eax
	pop     ax
	pop     dx
	popf
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_PackShapeFrames    ENDP

	END
