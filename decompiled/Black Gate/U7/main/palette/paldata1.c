/* Black Gate U7.EXE, resident segment 102 (file offset 0x03678a, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -G rebuilds its empty code segment as shipped.
 * Its data is DS:5CC2-5CC4, between occlude.c's and crawpal.c's. No code reads it, so it may be
 * palctrl.c's (segment 103) instead; the linked bytes are the same either way.
 */

int UnusedPaletteGlobal1 = 0;
