/* Black Gate INTRO.EXE, one module of resident segment 45 (file offsets 0x0115af to 0x0116de, 303 bytes).
 * Borland C++ 2.0 -mm -O -G -zCVIDMODE_TEXT rebuilds it byte for byte.
 * It links last: its data follows crtport.asm's.
 */

#include <stdio.h>
#include "vidmode.h"
#include "vidpage.h"
#pragma inline

#define EQUIPMENT_FLAGS 10h     /* BIOS equipment flags, 0040:0010 */
#define PAGE_COUNT      256
#define DUMP_ROWS       4
#define DUMP_COLUMNS    64

int ScreenMode = -1;
int PageRunLengths[PAGE_COUNT]; /* a run's length at each page it holds, 0 when free */

/* Sets a BIOS video mode. A graphics mode becomes the current one and frees every page; the
 * equipment flags are told whether the display is monochrome (mode 7 or 0Fh) or color. */
void pascal SetScreenMode(int mode)
{
	int i;

	if (mode > 3 && mode != 7) {
		ScreenMode = mode;
		for (i = 0; i < PAGE_COUNT; i++)
			PageRunLengths[i] = 0;
	}
	asm mov     ax, mode
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
	FindCrtStatusPort();
}

/* Takes the first run of size free pages, marking each with size.
 * Returns the first page, or -1 when no run is long enough. */
int pascal AllocatePageRun(int size)
{
	asm mov     di, size
	asm xor     ax, ax
	asm mov     dx, ax                  /* first page of the run */
	asm mov     cx, ax                  /* its length so far */
	asm mov     si, ax
	asm jmp     short test
scan:
	asm mov     bx, si
	asm shl     bx, 1
	asm cmp     word ptr PageRunLengths[bx], 0
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
	asm mov     PageRunLengths[bx], di
	asm inc     si
marked:
	asm cmp     si, di
	asm jl      mark
	asm mov     ax, dx
	asm jmp     short done
next:
	asm inc     si
test:
	asm cmp     si, PAGE_COUNT
	asm jl      scan
	asm mov     ax, -1
done:
	;
}

/* Frees the run of pages that starts at first. */
void pascal FreePageRun(int first)
{
	asm mov     di, first
	asm mov     bx, di
	asm shl     bx, 1
	asm mov     ax, PageRunLengths[bx]
	asm mov     dx, ax
	asm xor     si, si
	asm jmp     short test
clear:
	asm mov     bx, si
	asm add     bx, di
	asm shl     bx, 1
	asm mov     word ptr PageRunLengths[bx], 0
	asm inc     si
test:
	asm cmp     si, dx
	asm jl      clear
}

/* Prints a map of the pages, '.' for each one in use and '_' for each free one. */
void DumpPageRuns(void)
{
	int row;
	int column;
	int page;

	for (row = page = 0; row < DUMP_ROWS; row++) {
		for (column = 0; column < DUMP_COLUMNS; column++)
			printf("%c", (char) (PageRunLengths[page++] ? '.' : '_'));
		printf("\n");
	}
}
