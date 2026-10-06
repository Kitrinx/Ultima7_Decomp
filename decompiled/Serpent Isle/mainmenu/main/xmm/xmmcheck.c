/* Serpent Isle MAINMENU.EXE, resident segment 61 (file offsets 0x018fb8 to 0x018fcd, 21 bytes).
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
