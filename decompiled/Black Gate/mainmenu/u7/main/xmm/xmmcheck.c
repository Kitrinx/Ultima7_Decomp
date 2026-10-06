/* Black Gate U7.EXE, resident segment 149 (file offsets 0x03f4da to 0x03f4ef, 21 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
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
