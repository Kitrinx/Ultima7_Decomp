/* Black Gate U7.EXE, resident segment 66 (file offsets 0x02691e to 0x026976, 88 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Original folder unknown.
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
