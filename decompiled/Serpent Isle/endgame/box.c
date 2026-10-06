/* Serpent Isle ENDGAME.EXE, one module of resident segment 25 (file offsets 0x00c213 to 0x00c213, 0 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds its empty code segment as shipped.
 * Its data is DS:0A94-0A96, between palstep.c's and systimer.c's: Black Gate ENDGAME's box.c keeps this
 * byte beside Box::draw, which Serpent Isle no longer links. No code reads it.
 */

unsigned char FigureColor = 0;
