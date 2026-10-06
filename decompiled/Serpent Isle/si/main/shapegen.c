/* Serpent Isle SI.EXE, overlay segment 334 (file offsets 0x09cb30 to 0x09cd9e, 622 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "u7manage.h"

/* A view's clip corners, each x and y read as one long. */
struct View {
	int id;
	long rowTable;
	long topLeft;
	long bottomRight;
};

/* Builds a new shape block from the image in view source: EncodeFrame first measures (no
 * destination), then writes the data after an 8-byte header holding its length and the offset 8. */
int far MakeShapeFromImage(int source)
{
	View *view;
	unsigned size;
	int block;
	long dest;

	view = gShapeManager.lockView(source);
	size = EncodeFrame(view, view->topLeft, view->bottomRight, view->topLeft, 0L, 0xff, 101);
	block = gShapeManager.allocateBlock(size + 100, 0x7fff, 0);
	view = gShapeManager.lockView(source);
	dest = gShapeManager.get(block) + 8;
	view = gShapeManager.lockView(source);
	EncodeFrame(view, view->topLeft, view->bottomRight, view->topLeft, dest, 0xff, 101);
	PokeLong(gShapeManager.get(block), size + 8L);
	PokeLong(gShapeManager.get(block) + 4, 8L);
	return block;
}
