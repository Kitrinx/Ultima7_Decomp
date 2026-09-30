/* Black Gate U7.EXE, overlay segment 346 (file offsets 0x0a6b90 to 0x0a6e2c, 668 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include <string.h>
#include "objref.h"
#include "u7manage.h"
#include "itable.h"
#include "debug.h"
#include "bltshape.h"
#include "signgump.h"

struct View;

struct Position {
	int16_t x;
	int16_t y;
};

struct Rect {
	int16_t x0;
	int16_t y0;
	int16_t x1;
	int16_t y1;
};

/* Per style, numbered from 49: where the box goes, the text area inside it, and the font. */
static const Position SignGumpPositions[3] = { { 66, 41 }, { 63, 73 }, { 45, 35 } };
static const Rect SignTextAreas[3] = { { 5, 3, 182, 91 }, { 22, 19, 176, 109 }, { 37, 10, 196, 92 } };
static const int16_t SignFonts[3] = { 1, 3, 6 };

SignGump::SignGump(int16_t signStyle, char *text)
{
	char *p = text;
	char *start = text;
	int16_t n = 0;

	shape = signStyle + FIRST_GUMP_SHAPE;
	style = signStyle;
	for (; *p != 0; p++) {
		if (*p == '\r') {
			*p = 0;
			strncpy(lines[n], start, 40);
			*p = '\r';
			n++;
			if (n == 10)
				break;
			start = p + 1;
		}
	}
	lineCount = n;
	moveTo(SignGumpPositions[style - 49].x, SignGumpPositions[style - 49].y);
	printer.setFont(SignFonts[style - 49]);
}

void SignGump::moveTo(int16_t newX, int16_t newY)
{
	x = newX;
	y = newY;
}

uint8_t SignGump::handle(MouseState *)
{
	return 0;
}

void SignGump::draw(View *view)
{
	View *oldTarget;
	int16_t lineHeight, areaWidth, areaHeight, top;
	int16_t i;

	if (visible) {
		ShapeManager_draw(&gShapeManager, view, x, y, shape, 0, 0, 0);
		oldTarget = printer.target;
		printer.target = view;
		lineHeight = printer.charHeight('A');
		areaWidth = SignTextAreas[style - 49].x1 - SignTextAreas[style - 49].x0;
		areaHeight = SignTextAreas[style - 49].y1 - SignTextAreas[style - 49].y0;
		top = (areaHeight - lineCount * lineHeight) >> 1;
		for (i = 0; i < lineCount; i++) {
			int16_t textX = 0, textY = 0;
			int16_t width = printer.textWidth(lines[i]);

			textX = x + ((areaWidth - width) >> 1) + SignTextAreas[style - 49].x0;
			textY = y + i * lineHeight + lineHeight + SignTextAreas[style - 49].y0 + top;
			if (i > 10) {
				DebugPrintfWait("Too many lines!");
				break;
			}
			printer.print(textX, textY, lines[i]);
		}
		printer.target = oldTarget;
	}
}
