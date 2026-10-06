/* Serpent Isle ENDGAME.EXE, one module of resident segment 6 (file offsets 0x008f28 to 0x008f28, 0 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds its empty code segment as shipped.
 * Its data is DS:0668-06A0: the direction tables U7 keeps in sortitem.c, as in Black Gate ENDGAME. No code reads them.
 * File name inferred.
 */

/* one step in each of the eight directions, north first, then none */
int DirDeltaX[9] = { 0, 1, 1, 1, 0, -1, -1, -1, 0 };
int DirDeltaY[9] = { -1, -1, 0, 1, 1, 1, 0, -1, 0 };
/* the same for the four straight directions */
int CardinalDeltaX[5] = { 0, 1, 0, -1, 0 };
int CardinalDeltaY[5] = { -1, 0, 1, 0, 0 };
