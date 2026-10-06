/* Serpent Isle SI.EXE, resident segment 41 (file offsets 0x01d434 to 0x01d48c, 88 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
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
