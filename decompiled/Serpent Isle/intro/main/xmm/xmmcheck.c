/* Serpent Isle INTRO.EXE, resident segment 77 (file offsets 0x015498 to 0x0154ad, 21 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

int IsXMSPresent(void)
{
	asm {
		mov     ax, 4300h
		int     2Fh
		cmp     al, 80h
		jne     absent
	}
	return 1;
absent:
	return 0;
}
