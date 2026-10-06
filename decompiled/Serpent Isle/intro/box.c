/* Serpent Isle INTRO.EXE, one module of resident segment 26 (file offsets 0x00ceaf to 0x00ceaf, 0 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds its empty code segment as shipped.
 * Its data is DS:0E32-0E34, between palstep.c's and systimer.c's: Black Gate ENDGAME's box.c keeps this
 * byte beside Box::draw, which Serpent Isle no longer links. No code reads it.
 */

unsigned char FigureColor = 0;
