/* Serpent Isle MAINMENU.EXE, resident segment 71 (file offsets 0x01989e to 0x0199b1, 275 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include "screen.h"
#include "vidmode.h"
#pragma inline

#define EQUIPMENT_FLAGS 10h     /* BIOS equipment flags, 0040:0010 */
#define SLOT_COUNT 256

struct View;

char DisplayMode = -1;
char UnusedModeByte = 3;
char BiosVideoModes[] = { 0x13, 0x0d, 0x04, 0x09, 0x07, 0 };  /* BIOS video mode for each display mode */
int SlotRunLengths[SLOT_COUNT]; /* a run's length at each slot it holds, 0 when free */
extern struct View ScreenView;

void pascal SetBiosVideoMode(char mode);

/* Switches to a display mode. A graphics mode becomes the current one and frees every slot. */
void pascal SetDisplayMode(char mode)
{
	char biosMode = BiosVideoModes[mode];
	int i;

	if (biosMode > 3 && biosMode != 7) {
		DisplayMode = mode;
		for (i = 0; i < SLOT_COUNT; i++)
			SlotRunLengths[i] = 0;
	}
	SetBiosVideoMode(biosMode);
	FindCrtStatusPort();
	ApplyModeColors(mode);
	InitVgaScreen(&ScreenView, 0xff);
}

/* Sets a BIOS video mode, first telling the equipment flags whether the display is monochrome
 * (mode 7 or 0Fh) or color. */
void pascal SetBiosVideoMode(char mode)
{
	asm mov     ax, word ptr mode
	asm push    ds
	asm mov     cx, 40h
	asm mov     ds, cx
	asm mov     bl, 20h
	asm mov     ah, al
	asm and     ah, 7
	asm cmp     ah, 7
	asm jne     color
	asm mov     bl, 30h
color:
	asm and     byte ptr ds:[EQUIPMENT_FLAGS], 0CFh
	asm or      byte ptr ds:[EQUIPMENT_FLAGS], bl
	asm xor     ah, ah
	asm push    bp
	asm int     10h
	asm pop     bp
	asm pop     ds
}

/* Takes the first run of size free slots, marking each with size.
 * Returns the first slot, or -1 when no run is long enough. */
int pascal AllocateSlotRun(int size)
{
	asm mov     di, size
	asm xor     ax, ax
	asm mov     dx, ax                  /* first slot of the run */
	asm mov     cx, ax                  /* its length so far */
	asm mov     si, ax
	asm jmp     short test
scan:
	asm mov     bx, si
	asm shl     bx, 1
	asm cmp     word ptr SlotRunLengths[bx], 0
	asm je      free
	asm xor     cx, cx
	asm mov     ax, si
	asm inc     ax
	asm mov     dx, ax
	asm jmp     short check
free:
	asm inc     cx
check:
	asm cmp     cx, di
	asm jne     next
	asm xor     si, si
	asm jmp     short marked
mark:
	asm mov     bx, si
	asm add     bx, dx
	asm shl     bx, 1
	asm mov     SlotRunLengths[bx], di
	asm inc     si
marked:
	asm cmp     si, di
	asm jl      mark
	asm mov     ax, dx
	asm jmp     short done
next:
	asm inc     si
test:
	asm cmp     si, SLOT_COUNT
	asm jl      scan
	asm mov     ax, -1
done:
	;
}

/* Frees the run of slots that starts at first. */
void pascal FreeSlotRun(int first)
{
	asm mov     di, first
	asm mov     bx, di
	asm shl     bx, 1
	asm mov     ax, SlotRunLengths[bx]
	asm mov     dx, ax
	asm xor     si, si
	asm jmp     short test
clear:
	asm mov     bx, si
	asm add     bx, di
	asm shl     bx, 1
	asm mov     word ptr SlotRunLengths[bx], 0
	asm inc     si
test:
	asm cmp     si, dx
	asm jl      clear
}
