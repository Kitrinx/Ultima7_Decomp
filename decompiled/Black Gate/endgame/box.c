/* Black Gate ENDGAME.EXE, resident segment 28 (file offsets 0x00c90f to 0x00c9bd, 174 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "figure.h"

unsigned char FigureColor = 0;

/* Outlines the box on the current view, whichever way round its corners are. */
void Box::draw()
{
	if (x1 < x0 && y1 < y0)
		OutlineRect(CurrentView, x1, y1, x0, y0, FigureColor);
	else if (x1 < x0)
		OutlineRect(CurrentView, x1, y0, x0, y1, FigureColor);
	else if (y1 < y0)
		OutlineRect(CurrentView, x0, y1, x1, y0, FigureColor);
	else
		OutlineRect(CurrentView, x0, y0, x1, y1, FigureColor);
}

void Box::set(int x0, int y0, int x1, int y1, unsigned char color)
{
	Point::set(x0, y0);
	this->x1 = x1;
	this->y1 = y1;
	FigureColor = color;
	draw();
}
