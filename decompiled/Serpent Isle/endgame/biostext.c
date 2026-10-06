/* Serpent Isle ENDGAME.EXE, resident segment 47 (file offsets 0x0101c3 to 0x010382, 447 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "biostext.h"

/* Prints s through the BIOS teletype, each newline followed by a carriage return. */
void PutBiosString(char *s)
{
	char c;
	union REGS inregs, outregs;

	inregs.h.ah = 0x0e;
	inregs.h.bl = 0;
	while ((c = *s++) != 0) {
		for (;;) {
			inregs.h.al = c;
			int86(0x10, &inregs, &outregs);
			if (c != '\n')
				break;
			c = '\r';
		}
	}
}

/* Waits for a key. An extended key comes back as its scan code plus 256. */
unsigned GetBiosKey()
{
	union REGS regs;
	unsigned key;

	regs.h.ah = 0;
	int86(0x16, &regs, &regs);
	key = regs.h.al;
	if (key == 0)
		key = regs.h.ah | 0x100;
	return key;
}

void GetCursor(CursorPos *pos, unsigned char page)
{
	union REGS inregs, outregs;

	inregs.h.ah = 3;
	inregs.h.bh = page;
	int86(0x10, &inregs, &outregs);
	pos->row = outregs.h.dh;
	pos->column = outregs.h.dl;
}

void SetCursor(CursorPos *pos, unsigned char page)
{
	union REGS inregs, outregs;

	inregs.h.ah = 2;
	inregs.h.bh = page;
	inregs.h.dh = pos->row;
	inregs.h.dl = pos->column;
	int86(0x10, &inregs, &outregs);
}

/* Tells the adapter by the rows it can put the cursor on: 50 on VGA, 43 on EGA. */
unsigned char DetectDisplay()
{
	CursorPos saved, probe;
	unsigned char display;

	display = DISPLAY_VGA;
	GetCursor(&saved, 0);
	probe.column = 0;
	probe.row = 49;
	SetCursor(&probe, 0);
	GetCursor(&probe, 0);
	if ((int) probe.row != 49) {
		display = DISPLAY_EGA;
		probe.row = 39;
		SetCursor(&probe, 0);
		GetCursor(&probe, 0);
		if ((int) probe.row != 49)
			display = DISPLAY_CGA;
	}
	SetCursor(&saved, 0);
	return display;
}

/* Picks 200, 350 or 400 scan lines (0, 1 or 2) for the next text mode set. */
void SetScanLines(unsigned char lines)
{
	union REGS inregs, outregs;

	inregs.h.ah = 0x12;
	inregs.h.bl = 0x30;
	inregs.h.al = lines;
	int86(0x10, &inregs, &outregs);
}

/* Prints s through DOS, each newline followed by a carriage return. */
void PutDosString(char *s)
{
	char c;
	union REGS inregs, outregs;

	inregs.h.ah = 2;
	while ((c = *s++) != 0) {
		for (;;) {
			inregs.h.dl = c;
			int86(0x21, &inregs, &outregs);
			if (c != '\n')
				break;
			c = '\r';
		}
	}
}
