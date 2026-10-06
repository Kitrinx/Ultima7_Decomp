/* Serpent Isle MAINMENU.EXE, one module of resident segment 29 (file offsets 0x011060 to 0x011060, 0 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds its empty code segment as shipped.
 * Its data is DS:1146-117E, between strfmt.c's and flex.c's: the direction tables U7 keeps in
 * sortitem.c. No code reads them. File name inferred.
 */

/* one step in each of the eight directions, north first, then none */
int DirDeltaX[9] = { 0, 1, 1, 1, 0, -1, -1, -1, 0 };
int DirDeltaY[9] = { -1, -1, 0, 1, 1, 1, 0, -1, 0 };
/* the same for the four straight directions */
int CardinalDeltaX[5] = { 0, 1, 0, -1, 0 };
int CardinalDeltaY[5] = { -1, 0, 1, 0, 0 };
