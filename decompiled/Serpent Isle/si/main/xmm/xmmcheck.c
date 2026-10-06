/* Serpent Isle SI.EXE, resident segment 148 (file offsets 0x03f062 to 0x03f077, 21 bytes).
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
