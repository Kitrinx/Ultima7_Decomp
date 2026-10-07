/* Serpent Isle SI.EXE, overlay segment 335 (file offsets 0x09cdc0 to 0x09d1ce, 1038 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <string.h>
#include "objref.h"
#include "u7manage.h"
#include "itable.h"
#include "npcref.h"
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

/* Per style, numbered from 44: where the box goes, the text area inside it, and the font. */
static const Position SignGumpPositions[3] = { { 66, 41 }, { 63, 73 }, { 0, 35 } };
static const Rect SignTextAreas[3] = { { 5, 3, 182, 91 }, { 22, 19, 176, 109 }, { 37, 10, 280, 94 } };
static const uint8_t SignFonts[3] = { 1, 3, 6 };
static const uint8_t RunicFonts[3] = { 9, 9, 10 };

/* Styles past 46 are the same signs in a runic font. While the avatar's flag is set, every sign
 * is shown in capitals, its runic ligatures ('(' for TH and so on) spelled out. */
SignGump::SignGump(int16_t signStyle, char *text)
{
	char *p = text;
	char *start = text;
	uint8_t runic = 0;
	int16_t n = 0;

	if (signStyle > 46) {
		signStyle -= 3;
		runic = 1;
	}
	shape = signStyle + FIRST_GUMP_SHAPE;
	style = signStyle;
	for (; *p != 0; p++) {
		if (*p == '\r') {
			*p = 0;
			strncpy(lines[n], start, 40);
			if (Npc_hasReadFlag(&AvatarRef)) {
				/* SI.EXE's frame: past plain's 40 bytes lay a spare byte, then j and i, which a long
				 * spelled-out line overwrites; spill takes the rest of what j can reach. */
				struct { char plain[40]; char spare; int8_t j, i; char spill[85]; } frame = {};
				char *plain = (char *)&frame;
				int8_t &i = frame.i, &j = frame.j;
				int8_t c;

				for (i = 0, j = 0; i < 40; i++, j++) {
					switch (lines[n][i]) {
					case '+':
						plain[j++] = 'E';
						plain[j] = 'A';
						break;
					case '(':
						plain[j++] = 'T';
						plain[j] = 'H';
						break;
					case ')':
						plain[j++] = 'E';
						plain[j] = 'E';
						break;
					case '*':
						plain[j++] = 'N';
						plain[j] = 'G';
						break;
					case ',':
						plain[j++] = 'S';
						plain[j] = 'T';
						break;
					default:
						c = lines[n][i];
						plain[j] = c >= 'a' && c <= 'z' ? c - 32 : c;
					}
				}
				strcpy(lines[n], plain);
			}
			*p = '\r';
			n++;
			if (n == 10)
				break;
			start = p + 1;
		}
	}
	lineCount = n;
	moveTo(SignGumpPositions[style - 44].x, SignGumpPositions[style - 44].y);
	printer.setFont(runic && !Npc_hasReadFlag(&AvatarRef) ? RunicFonts[style - 44] :
		SignFonts[style - 44]);
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
		areaWidth = SignTextAreas[style - 44].x1 - SignTextAreas[style - 44].x0;
		areaHeight = SignTextAreas[style - 44].y1 - SignTextAreas[style - 44].y0;
		top = (areaHeight - lineCount * lineHeight) >> 1;
		for (i = 0; i < lineCount; i++) {
			int16_t textX = 0, textY = 0;
			int16_t width = printer.textWidth(lines[i]);

			textX = x + ((areaWidth - width) >> 1) + SignTextAreas[style - 44].x0;
			textY = y + i * lineHeight + lineHeight + SignTextAreas[style - 44].y0 + top;
			if (i > 10)
				break;
			printer.print(textX, textY, lines[i]);
		}
		printer.target = oldTarget;
	}
}
