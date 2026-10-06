/* Serpent Isle INTRO.EXE, resident segment 18 (file offsets 0x00c932 to 0x00c983, 81 bytes).
 * Borland C++ 2.0 -mm -1 rebuilds it byte for byte.
 */

#include <dos.h>
#include "vidmode.h"

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
	FindCrtStatusPort();
}
