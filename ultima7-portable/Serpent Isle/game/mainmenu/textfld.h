#ifndef TEXTFLD_H
#define TEXTFLD_H

#include "systimer.h"
#include "controls.h"

namespace Shared {
class FontTextPrinter;
}

namespace MainMenu {

using Shared::FontTextPrinter;

struct KeyQueue;

/* A line of text the player types, read from a key queue, with a blinking cursor. */
struct TextField {
	Timer cursorTimer;
	KeyQueue *keys;
	char *start;
	int16_t length;
	char *text;
	int16_t size;
	int16_t cursor;
	uint8_t locked;
	uint8_t cursorVisible;
	uint8_t active;
	TextField();
	TextField(int16_t bufferSize, KeyQueue *queue);
	virtual void reject() = 0;
	virtual void display() = 0;
	void reset(char *buffer, int16_t bufferSize, KeyQueue *queue);
	void setText(char *s);
	int8_t update();
	void activate() { active = 1; }
	void deactivate() { active = 0; }
	int8_t isEmpty() { return !*text; }
	char *getText() { return text; }
};

int8_t RemoveChar(char *at, int16_t count);
void InsertChar(char *at, int16_t count, int8_t c);

/* A text field shown on a screen, printed with a font at (x, y). */
struct TextBox : TextField, Control {
	FontTextPrinter *printer;
	int8_t cursorChar;
	int16_t x, y;
	TextBox(FontTextPrinter *font, int8_t cursor);
	TextBox(FontTextPrinter *font, int16_t bufferSize, KeyQueue *queue, int8_t cursor);
	void moveTo(int16_t nx, int16_t ny);
	void reject();
	void display();
	void saveUnder();
	void draw();
	void restoreUnder();
	uint8_t contains(int16_t px, int16_t py);
};

}

#endif
