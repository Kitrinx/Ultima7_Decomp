/* Black Gate MAINMENU.EXE, one module of resident segment 29 (file offset 0x0107e4, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -1 -P rebuilds its empty code segment as shipped.
 * Its data is DS:1232-126A, between easyfile.c's and flex.c's: the direction tables U7 keeps in
 * sortitem.c, which follows strfmt.c in U7's link too. No code reads them. File name inferred.
 */

/* one step in each of the eight directions, north first, then none */
int DirDeltaX[9] = { 0, 1, 1, 1, 0, -1, -1, -1, 0 };
int DirDeltaY[9] = { -1, -1, 0, 1, 1, 1, 0, -1, 0 };
/* the same for the four straight directions */
int CardinalDeltaX[5] = { 0, 1, 0, -1, 0 };
int CardinalDeltaY[5] = { -1, 0, 1, 0, 0 };
