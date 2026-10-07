/* Serpent Isle SI.EXE, overlay segment 322 (file offsets 0x08f910 to 0x0906e7, 3543 bytes).
 * Borland C++ 2.0 -mm -O -P -d rebuilds it byte for byte as C++.
 */

/* path: convgump.c */
#include "u7port.h"
#include "plat.h"
#include <string.h>
#include "objref.h"
#include "u7manage.h"
#include "itable.h"
#include "colbuf.h"
#include "keywords.h"
#include "gumps.h"
#include "init.h"
#include "debug.h"
#include "u7event.h"
#include "bltshape.h"
#include "npcref.h"
#include "text.h"

struct View;

/* a screen rectangle, corners included */
struct Rect : Point {
	int16_t x1, y1;
	Rect() { x1 = 0; y1 = 0; }
	Rect(int16_t a, int16_t b, int16_t c, int16_t d) : Point(a, b) { x1 = c; y1 = d; }
	void set(int16_t a, int16_t b) { x = a; y = b; x1 = -1; y1 = -1; }
	void set(int16_t a, int16_t b, int16_t c, int16_t d) { x = a; y = b; x1 = c; y1 = d; }
	int16_t width() { return x1 - x + 1; }
	int16_t height() { return y1 - y + 1; }
	void moveTo(int16_t a, int16_t b) { x1 = a + (x1 - x); y1 = b + (y1 - y); x = a; y = b; }
	int16_t contains(int16_t a, int16_t b) { return (x <= a) & (x1 >= a) & (y <= b) & (y1 >= b); }
};

/* a face, drawn at a corner of `where` */
struct FaceGump : Control {
	int16_t shape;
	Rect where;
	int16_t frame;
	FaceGump(int16_t x, int16_t y);
	FaceGump(const Rect &r);
	void set(int16_t shape, int16_t frame, uint8_t absolute);
	void moveTo(int16_t x, int16_t y);
	uint8_t handle(MouseState *);
	void draw(View *);
};

/* the answers, laid out in lines; `top` is the first line shown */
struct AnswerPane : Control {
	Rect bounds;
	uint8_t lines;
	int16_t top;
	AnswerPane(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
	void layout();
	void moveTo(int16_t x, int16_t y);
	uint8_t handle(MouseState *);
	void draw(View *);
	void scrollUp();
	void scrollDown();
};

/* the gump that lists the answers */
struct AnswerGump : Control {
	AnswerPane options;
	Rect bounds;
	int16_t unusedWord;
	AnswerGump(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
	void moveTo(int16_t x, int16_t y);
	uint8_t handle(MouseState *);
	void draw(View *);
};

/* a box of text lines, `width` characters each */
struct TextBox : Control {
	Rect r;
	int16_t used;
	char *text;
	int16_t width;
	int16_t height;
	int8_t centered;
	TextBox(const Rect &r, int16_t width, int16_t height);
	~TextBox();
	void add(char *s);
	void moveTo(int16_t x, int16_t y);
	uint8_t handle(MouseState *);
	void draw(View *);
	int16_t lines();
};

/* where each answer sits: its x in the line and its line, from 1 */
struct AnswerSpot {
	uint8_t x;
	uint8_t line;
};

AnswerSpot AnswerSpots[25] = { 0 };

FaceGump::FaceGump(int16_t x, int16_t y)
{
	where.set(x, y);
}

FaceGump::FaceGump(const Rect &r)
{
	shape = -1;
	where = r;
}

void FaceGump::set(int16_t s, int16_t f, uint8_t absolute)
{
	if (!absolute && s == 0 || absolute && s + 1098 == 0) {
		f = Npc_isMale(&AvatarRef) != 0;
		switch (Npc_getSkinColor(&AvatarRef)) {
		case 0:
			break;
		case 1:
			f += 2;
			break;
		case 2:
			f += 4;
			break;
		case 3:
			Npc_setSkinColor(&AvatarRef, 1);
			f += 2;
			break;
		default:
			f = 0;
		}
		if (Npc_hasCombatLowFlag(&AvatarRef))
			shape = 1397;
		else
			shape = 1098;
		if (Npc_hasPetraFlag(&AvatarRef)) {
			shape = 1126;
			f = 0;
		}
	} else if (Npc_hasPetraFlag(&AvatarRef) && (!absolute && s == 28 || absolute && s + 1098 == 28)) {
		objref npc;
		GetNpcIbo(&npc, 28);
		f = Npc_isMale(&npc) != 0;
		switch (Npc_getSkinColor(&AvatarRef)) {
		case 0:
			break;
		case 1:
			f += 2;
			break;
		case 2:
			f += 4;
			break;
		case 3:
			Npc_setSkinColor(&AvatarRef, 1);
			f += 2;
			break;
		default:
			f = 0;
		}
		if (Npc_hasCombatLowFlag(&AvatarRef))
			shape = 1397;
		else
			shape = 1098;
	} else
		shape = absolute ? s : s + 1098;
	frame = f;
}

uint8_t FaceGump::handle(MouseState *)
{
	return 0;
}

void FaceGump::draw(View *target)
{
	int16_t w, h;

	if (visible) {
		gShapeManager.getFrameSize(&w, &h, shape, frame);
		if (where.x1 == -1)
			ShapeManager_draw(&gShapeManager, target, where.x, where.y, shape, frame, 0, 0);
		else
			ShapeManager_draw(&gShapeManager, target, where.x1, where.y + h - 10, shape, frame, 0, 0);
	}
}

void FaceGump::moveTo(int16_t x, int16_t y)
{
	if (where.x1 != -1) {
		int16_t w = where.width();
		int16_t h = where.height();
		where.moveTo(x - w - 1, y - h - 1);
	} else
		where.set(x, y);
}

/* how many times c appears in s; loops forever unless s is "" */
int16_t CountChar(char *s, int8_t c)
{
	int16_t n = 0;

	while (*s)
		if (*s == c)
			n++;
	return n;
}

AnswerPane::AnswerPane(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	top = 0;
	bounds.set(x0 - 1, y0 + 1, x1 - 1, y1 - 1);
}

/* place each answer after the last, starting a line when it will not fit */
void AnswerPane::layout()
{
	Answer *a = 0;
	Answer *prev = 0;
	uint8_t x = 0;
	int16_t n;

	lines = 0;
	n = 0;
	while (OfferedAnswers.next(&a)) {
		n++;
		int16_t w = YellowTextPrinter.charWidth(0x7f) + YellowTextPrinter.textWidth(a->text) + 9;
		int8_t full = bounds.width() < x + w;
		int8_t both = prev && CountChar(prev->text, ' ') && CountChar(a->text, ' ');
		if (full || both) {
			AnswerSpots[n].x = 0;
			AnswerSpots[n].line = ++lines;
			x = w;
		} else {
			AnswerSpots[n].x = x;
			AnswerSpots[n].line = lines;
			x += w;
		}
	}
	prev = a;   /* after the loop: prev is 0 throughout it */
}

void AnswerPane::moveTo(int16_t x, int16_t y)
{
	bounds.moveTo(x, y);
}

/* 8 when an answer is clicked; its text is copied out for the conversation */
uint8_t AnswerPane::handle(MouseState *m)
{
	Answer *a = 0;

	if (bounds.contains(MouseState_getX(m), m->y)) {
		int16_t lh = YellowTextPrinter.charHeight(0);
		int16_t n = 0;
		int16_t shown = bounds.height() / lh;
		while (OfferedAnswers.next(&a)) {
			n++;
			if (AnswerSpots[n].line < top)
				continue;
			if (AnswerSpots[n].line > shown + top)
				break;
			if (Rect(bounds.x + AnswerSpots[n].x,
				bounds.y + (AnswerSpots[n].line - top) * lh - 2,
				bounds.x + AnswerSpots[n].x +
				YellowTextPrinter.textWidth(a->text) + YellowTextPrinter.charWidth(0x7f),
					bounds.y + (AnswerSpots[n].line - top + 1) * lh - 2).contains(MouseState_getX(m),
					m->y)) {
				if (strlen(a->text) >= ANSWER_SIZE) {
					CheatPrintfAtCoords(1, 1, GetGameText(3, 240), __FILE__, 416);
					a->text[ANSWER_SIZE - 1] = 0;
				}
				strncpy(ChosenAnswer, a->text, ANSWER_SIZE);
				while (!GameInput.isButtonReleased(1))
					plat_yield();
				return 8;
			}
		}
		while (!GameInput.isButtonReleased(1))
			plat_yield();
	}
	return 0;
}

void AnswerPane::draw(View *target)
{
	Answer *a;
	View *old;
	int16_t last;
	int16_t n;
	char s[33];

	if (!visible)
		return;
	a = 0;
	memset(s, 0, sizeof s);
	s[0] = 0x7f;
	old = YellowTextPrinter.target;
	YellowTextPrinter.target = target;
	n = 0;
	last = top + bounds.height() / YellowTextPrinter.charHeight(0);
	while (OfferedAnswers.next(&a)) {
		n++;
		if (AnswerSpots[n].line < top)
			continue;
		if (AnswerSpots[n].line > last) {
			CheatPrintfAtCoords(1, 1, GetGameText(3, 240), __FILE__, 455);
			break;
		}
		if (strlen(a->text) > 30) {
			CheatPrintfAtCoords(1, 1, GetGameText(3, 240), __FILE__, 463);
			continue;
		}
		strncpy(s + 1, a->text, 29);
		YellowTextPrinter.x = bounds.x + AnswerSpots[n].x;
		YellowTextPrinter.y = bounds.y +
			YellowTextPrinter.charHeight(0) * (AnswerSpots[n].line - top) + 8;
		YellowTextPrinter.printString(s);
	}
	YellowTextPrinter.target = old;
}

void AnswerPane::scrollUp()
{
	if (top >= 1)
		top--;
}

void AnswerPane::scrollDown()
{
	if (top + bounds.height() / YellowTextPrinter.charHeight(0) <= lines - 1)
		top++;
}

AnswerGump::AnswerGump(int16_t x0, int16_t y0, int16_t x1, int16_t y1) : options(x0, y0, x1, y1)
{
	unusedWord = 0;
	bounds.set(x0, y0, x1, y1);
	add(&options);
}

void AnswerGump::moveTo(int16_t x, int16_t y)
{
	bounds.moveTo(x, y);
	options.moveTo(x, y);
}

uint8_t AnswerGump::handle(MouseState *m)
{
	int8_t c;

	if (bounds.contains(MouseState_getX(m), m->y)) {
		if ((c = options.handle(m)) != 0)
			return c;
	}
	return 0;
}

void AnswerGump::draw(View *)
{
}

TextBox::TextBox(const Rect &box, int16_t w, int16_t h) : r(box)
{
	width = w;
	height = h;
	centered = 0;
	text = new char[w * h];
	if (text == 0)
		AssertFail(__FILE__, 615);
	used = 0;
}

TextBox::~TextBox()
{
	if (text) {
		delete text;
		text = 0;
	}
}

/* add a line, cut to the box's width; ignored when the box is full */
void TextBox::add(char *s)
{
	if (used < height) {
		_fstrncpy(text + used * width, s, width);
		text[used * width + width - 1] = 0;
		used++;
	}
}

void TextBox::moveTo(int16_t x, int16_t y)
{
	r.moveTo(x, y);
}

uint8_t TextBox::handle(MouseState *m)
{
	if (r.contains(MouseState_getX(m), m->y))
		return 14;
	return 0;
}

void TextBox::draw(View *target)
{
	View *old;
	int16_t x;
	int16_t lh;
	int16_t i;

	if (visible) {
		old = YellowTextPrinter.target;
		x = 0;
		YellowTextPrinter.target = target;
		lh = YellowTextPrinter.charHeight(0);
		for (i = 0; i < used; i++) {
			if (centered) {
				/* half the room left on the line */
				x = r.width() - YellowTextPrinter.textWidth(text + width * i);
				x >>= 1;
			}
			YellowTextPrinter.x = r.x + x;
			YellowTextPrinter.y = r.y + i * lh;
			YellowTextPrinter.printString(text + width * i);
		}
		YellowTextPrinter.target = old;
	}
}

/* how many lines the box shows */
int16_t TextBox::lines()
{
	int16_t lh = YellowTextPrinter.charHeight(0);

	return r.height() / lh;
}

extern "C" void ResetConvgumpGlobals(void)
{
	memset(AnswerSpots, 0, sizeof(AnswerSpots));
}
