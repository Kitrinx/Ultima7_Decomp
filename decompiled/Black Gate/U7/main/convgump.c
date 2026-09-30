/* Black Gate U7.EXE, overlay segment 336 (file offsets 0x09c600 to 0x09d380, 3456 bytes).
 * Borland C++ 2.0 -mm -O -P -d rebuilds it byte for byte as C++.
 */

/* path: convgump.c */
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

struct View;

/* a screen rectangle, corners included */
struct Rect : Point {
	int x1, y1;
	Rect() { x1 = 0; y1 = 0; }
	Rect(int a, int b, int c, int d) : Point(a, b) { x1 = c; y1 = d; }
	void set(int a, int b) { x = a; y = b; x1 = -1; y1 = -1; }
	void set(int a, int b, int c, int d) { x = a; y = b; x1 = c; y1 = d; }
	int width() { return x1 - x + 1; }
	int height() { return y1 - y + 1; }
	void moveTo(int a, int b) { x1 = a + (x1 - x); y1 = b + (y1 - y); x = a; y = b; }
	int contains(int a, int b) { return (x <= a) & (x1 >= a) & (y <= b) & (y1 >= b); }
};

/* a face, drawn at a corner of `where` */
struct FaceGump : Control {
	int shape;
	Rect where;
	int frame;
	FaceGump(int x, int y);
	FaceGump(Rect &r);
	void set(int shape, int frame, char absolute);
	void moveTo(int x, int y);
	unsigned char handle(MouseState *);
	void draw(View *);
};

/* the answers, laid out in lines; `top` is the first line shown */
struct AnswerPane : Control {
	Rect bounds;
	unsigned char lines;
	int top;
	AnswerPane(int x0, int y0, int x1, int y1);
	void layout();
	void moveTo(int x, int y);
	unsigned char handle(MouseState *);
	void draw(View *);
	void scrollUp();
	void scrollDown();
};

/* the arrows that scroll the answers */
struct AnswerScroller : Control {
	int x;
	int y;
	int height;
	SizedSprite up;
	SizedSprite down;
	AnswerScroller(int x, int y, int height);
	void moveTo(int x, int y);
	unsigned char handle(MouseState *);
	void draw(View *);
};

/* the gump that lists the answers */
struct AnswerGump : Control {
	AnswerPane options;
	Rect bounds;
	int unusedWord;
	AnswerGump(int x0, int y0, int x1, int y1);
	void moveTo(int x, int y);
	unsigned char handle(MouseState *);
	void draw(View *);
};

/* a box of text lines, `width` characters each */
struct TextBox : Control {
	Rect r;
	int used;
	char *text;
	int width;
	int height;
	char centered;
	TextBox(Rect &r, int width, int height);
	~TextBox();
	void add(char far *s);
	void moveTo(int x, int y);
	unsigned char handle(MouseState *);
	void draw(View *);
	int lines();
};

/* where each answer sits: its x in the line and its line, from 1 */
struct AnswerSpot {
	unsigned char x;
	unsigned char line;
};

AnswerSpot AnswerSpots[25] = { 0 };

FaceGump::FaceGump(int x, int y)
{
	where.set(x, y);
}

FaceGump::FaceGump(Rect &r)
{
	shape = -1;
	where = r;
}

void FaceGump::set(int s, int f, char absolute)
{
	if (absolute)
		shape = s;
	else
		shape = s + 1059;
	frame = f;
}

unsigned char FaceGump::handle(MouseState *)
{
	return 0;
}

void FaceGump::draw(View *target)
{
	int w, h;

	if (visible) {
		gShapeManager.getFrameSize(&w, &h, shape, frame);
		if (where.x1 == -1)
			ShapeManager_draw(&gShapeManager, target, where.x, where.y, shape, frame, 0, 0);
		else
			ShapeManager_draw(&gShapeManager, target, where.x1, where.y + h - 10, shape, frame, 0, 0);
	}
}

void FaceGump::moveTo(int x, int y)
{
	if (where.x1 != -1) {
		int w = where.width();
		int h = where.height();
		where.moveTo(x - w - 1, y - h - 1);
	} else
		where.set(x, y);
}

/* how many times c appears in s; loops forever unless s is "" */
int CountChar(char *s, char c)
{
	int n = 0;

	while (*s)
		if (*s == c)
			n++;
	return n;
}

AnswerPane::AnswerPane(int x0, int y0, int x1, int y1)
{
	top = 0;
	bounds.set(x0 - 1, y0 + 1, x1 - 1, y1 - 1);
}

/* place each answer after the last, starting a line when it will not fit */
void AnswerPane::layout()
{
	Answer *a = 0;
	Answer *prev = 0;
	unsigned char x = 0;
	int n;

	lines = 0;
	n = 0;
	while (OfferedAnswers.next(&a)) {
		n++;
		int w = YellowTextPrinter.charWidth(0x7f) + YellowTextPrinter.textWidth(a->text) + 9;
		char full = bounds.width() < x + w;
		char both = prev && CountChar(prev->text, ' ') && CountChar(a->text, ' ');
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

void AnswerPane::moveTo(int x, int y)
{
	bounds.moveTo(x, y);
}

/* 8 when an answer is clicked; its text is copied out for the conversation */
unsigned char AnswerPane::handle(MouseState *m)
{
	Answer *a = 0;

	if (bounds.contains(MouseState_getX(m), m->y)) {
		int lh = YellowTextPrinter.charHeight(0);
		int n = 0;
		int shown = bounds.height() / lh;
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
					CheatPrintfAtCoords(1, 1, "%s %d", __FILE__, 246);
					a->text[ANSWER_SIZE - 1] = 0;
				}
				strncpy(ChosenAnswer, a->text, ANSWER_SIZE);
				while (!GameInput.isButtonReleased(1))
					;
				return 8;
			}
		}
		while (!GameInput.isButtonReleased(1))
			;
	}
	return 0;
}

void AnswerPane::draw(View *target)
{
	Answer *a;
	View *old;
	int last;
	int n;
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
			CheatPrintfAtCoords(1, 1, "%s %d", __FILE__, 285);
			break;
		}
		if (strlen(a->text) > 30) {
			CheatPrintfAtCoords(1, 1, "%s %d", __FILE__, 293);
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

AnswerGump::AnswerGump(int x0, int y0, int x1, int y1) : options(x0, y0, x1, y1)
{
	unusedWord = 0;
	bounds.set(x0, y0, x1, y1);
	add(&options);
}

void AnswerGump::moveTo(int x, int y)
{
	bounds.moveTo(x, y);
	options.moveTo(x, y);
}

unsigned char AnswerGump::handle(MouseState *m)
{
	char c;

	if (bounds.contains(MouseState_getX(m), m->y)) {
		if ((c = options.handle(m)) != 0)
			return c;
	}
	return 0;
}

void AnswerGump::draw(View *)
{
}

AnswerScroller::AnswerScroller(int px, int py, int h) : up(19), down(18)
{
	x = px;
	y = py;
	height = h;
	add(&up);
	add(&down);
	moveTo(x, y);
}

void AnswerScroller::moveTo(int px, int py)
{
	int w, h;

	up.moveTo(px, py);
	down.size(&w, &h);
	down.moveTo(px, py + height - h + 1);
}

/* 9 or 10 when an arrow is pressed; otherwise whatever the down arrow said */
unsigned char AnswerScroller::handle(MouseState *m)
{
	if (up.handle(m) == 18)
		return 9;
	if (down.handle(m) == 18)
		return 10;
}

void AnswerScroller::draw(View *)
{
}

TextBox::TextBox(Rect &box, int w, int h) : r(box)
{
	width = w;
	height = h;
	centered = 0;
	text = new char[w * h];
	if (text == 0)
		AssertFail(__FILE__, 445);
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
void TextBox::add(char far *s)
{
	if (used < height) {
		_fstrncpy(text + used * width, s, width);
		text[used * width + width - 1] = 0;
		used++;
	}
}

void TextBox::moveTo(int x, int y)
{
	r.moveTo(x, y);
}

unsigned char TextBox::handle(MouseState *m)
{
	if (r.contains(MouseState_getX(m), m->y))
		return 14;
	return 0;
}

void TextBox::draw(View *target)
{
	View *old;
	int x;
	int lh;
	int i;

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
int TextBox::lines()
{
	int lh = YellowTextPrinter.charHeight(0);

	return r.height() / lh;
}
