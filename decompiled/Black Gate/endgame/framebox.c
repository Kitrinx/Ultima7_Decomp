/* Black Gate ENDGAME.EXE, resident segment 30 (file offsets 0x00cfbb to 0x00d035, 122 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "figure.h"

/* Fills the box on view. */
void FramedBox::fill(View *view)
{
	FillRect(view, x0, y0, x1, y1, FigureColor);
}

/* Outlines the box on the current view and fills it on view. */
void FramedBox::drawFramed(View *view)
{
	Box::draw();
	fill(view);
}

void FramedBox::set(int x0, int y0, int x1, int y1, unsigned char color, View *view)
{
	Point::set(x0, y0);
	this->x1 = x1;
	this->y1 = y1;
	FigureColor = color;
	drawFramed(view);
}
