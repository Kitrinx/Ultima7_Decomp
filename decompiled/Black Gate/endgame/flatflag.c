/* Black Gate ENDGAME.EXE, one module of resident segment 79 (file offset 0x014900, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -1 -P rebuilds its empty code segment as shipped.
 * Its data is DS:4332-4334, after flatmode.asm's and before freexmm.c's. linear.asm and xmmblock.asm
 * read it; U7 and MAINMENU define it in drawtile.asm, which ENDGAME does not link. File name inferred.
 */

/* bit 0: enter flat mode before each access */
int FlatModeFlags = 0;
