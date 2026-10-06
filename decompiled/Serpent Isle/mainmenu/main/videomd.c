/* Serpent Isle MAINMENU.EXE, resident segment 23 (file offsets 0x00fe34 to 0x00fe80, 76 bytes).
 * Borland C++ 2.0 -mm -O -1 rebuilds it byte for byte.
 */

#include <dos.h>

/* Reads the BIOS video mode. */
void GetVideoMode(unsigned char *mode)
{
	union REGS inregs, outregs;

	inregs.h.ah = 0x0f;
	int86(0x10, &inregs, &outregs);
	*mode = outregs.h.al;
}

/* Sets the BIOS video mode. */
void SetVideoMode(unsigned char *mode)
{
	union REGS inregs, outregs;

	inregs.h.ah = 0;
	inregs.h.al = *mode;
	int86(0x10, &inregs, &outregs);
}
