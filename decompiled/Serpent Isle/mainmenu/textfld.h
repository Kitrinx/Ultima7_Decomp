#ifndef TEXTFLD_H
#define TEXTFLD_H

#include "systimer.h"
#include "controls.h"

struct KeyQueue;
struct FontTextPrinter;

/* A line of text the player types, read from a key queue, with a blinking cursor. */
struct TextField {
	Timer cursorTimer;
	KeyQueue *keys;
	char *start;
	int length;
	char *text;
	int size;
	int cursor;
	unsigned char locked;
	unsigned char cursorVisible;
	unsigned char active;
	TextField();
	TextField(int bufferSize, KeyQueue *queue);
	virtual void reject() = 0;
	virtual void display() = 0;
	void reset(char *buffer, int bufferSize, KeyQueue *queue);
	void setText(char *s);
	char update();
	void activate() { active = 1; }
	void deactivate() { active = 0; }
	char isEmpty() { return !*text; }
	char *getText() { return text; }
};

char RemoveChar(char *at, int count);
void InsertChar(char *at, int count, char c);

/* A text field shown on a screen, printed with a font at (x, y). */
struct TextBox : TextField, Control {
	FontTextPrinter *printer;
	char cursorChar;
	int x, y;
	TextBox(FontTextPrinter *font, char cursor);
	TextBox(FontTextPrinter *font, int bufferSize, KeyQueue *queue, char cursor);
	void moveTo(int nx, int ny);
	void reject();
	void display();
	void saveUnder();
	void draw();
	void restoreUnder();
	unsigned char contains(int px, int py);
};

#endif
