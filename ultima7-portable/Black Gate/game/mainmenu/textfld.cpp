/* Black Gate MAINMENU.EXE modules TEXTFLD and TEXTBOX: a line of typed text, shown on a screen
 * with a blinking cursor.
 */

#include "u7port.h"
#include "oops.h"
#include "keyqueue.h"
#include "../shared/itable.h"
#include "textfld.h"

namespace MainMenu {

#define CURSOR_BLINK_TICKS  20

#define KEY_BACKSPACE   8
#define KEY_ENTER       13
#define KEY_HOME        (KEY_EXTENDED + 0x47)
#define KEY_LEFT        (KEY_EXTENDED + 0x4b)
#define KEY_RIGHT       (KEY_EXTENDED + 0x4d)
#define KEY_END         (KEY_EXTENDED + 0x4f)
#define KEY_INSERT      (KEY_EXTENDED + 0x52)
#define KEY_DELETE      (KEY_EXTENDED + 0x53)

/* Removes the character at `at`, closing up the next count - 1 and zeroing the last; returns
 * the one removed. */
char RemoveChar(char *at, int16_t count)
{
	char removed = *at;

	memmove(at, at + 1, count - 1);
	at[count - 1] = 0;
	return removed;
}

/* Opens a gap at `at` by moving the next count - 1 characters up one, and puts c in it. */
void InsertChar(char *at, int16_t count, char c)
{
	memmove(at + 1, at, count - 1);
	*at = c;
}

TextField::TextField()
{
	text = 0;
	start = 0;
	locked = 0;
	size = 0;
	cursorVisible = 0;
	cursor = 0;
	length = 0;
	keys = 0;
	active = 0;
}

TextField::TextField(int16_t bufferSize, KeyQueue *queue)
{
	char *buffer = new char[bufferSize + 1];

	*buffer = 0;
	reset(buffer, bufferSize, queue);
}

void TextField::reset(char *buffer, int16_t bufferSize, KeyQueue *queue)
{
	text = buffer;
	size = bufferSize;
	start = buffer;
	cursor = 0;
	length = 0;
	locked = 0;
	cursorVisible = 1;
	active = 0;
	keys = queue;
	Timer_set(&cursorTimer, CURSOR_BLINK_TICKS);
}

void TextField::setText(char *s)
{
	strncpy(text, s, size);
	text[size] = 0;
	length = strlen(text);
	cursor = length;
	if (cursor)
		cursor--;
}

/* Handles one waiting key and blinks the cursor; returns 1 once Enter is pressed. */
char TextField::update()
{
	char entered = 0;
	int16_t key;

	if (keys->peek(&key)) {
		keys->get(&key);
		switch (key) {
		case KEY_LEFT:
			if (cursor)
				cursor--;
			else
				reject();
			break;
		case KEY_BACKSPACE:
			if (cursor) {
				cursor--;
				RemoveChar(text + cursor, size - cursor);
				length--;
			} else
				reject();
			break;
		case KEY_RIGHT:
			if (cursor < size) {
				if (text[cursor])
					cursor++;
				else
					reject();
			} else
				reject();
			break;
		case KEY_HOME:
			cursor = 0;
			break;
		case KEY_END:
			cursor = strlen(text);
			break;
		case KEY_INSERT:
			/* insert mode is not finished: the key always turns it off */
			locked = 0;
			break;
		case KEY_ENTER:
			entered = 1;
			break;
		case KEY_DELETE:
			break;
		default:
			if (cursor >= size || key < ' ' || key > 'z')
				reject();
			else if (locked == 0) {
				text[cursor++] = key;
				text[++length] = 0;
			}
			break;
		}
	}
	if (Timer_hasFinished(&cursorTimer)) {
		cursorVisible = !cursorVisible;
		Timer_set(&cursorTimer, CURSOR_BLINK_TICKS);
	}
	return entered;
}

TextBox::TextBox(FontTextPrinter *font, char cursor) : TextField(), Control()
{
	printer = font;
	cursorChar = cursor;
	x = 0;
	y = 0;
}

TextBox::TextBox(FontTextPrinter *font, int16_t bufferSize, KeyQueue *queue, char cursor) :
	TextField(bufferSize, queue), Control()
{
	printer = font;
	cursorChar = cursor;
	x = 0;
	y = 0;
}

void TextBox::moveTo(int16_t nx, int16_t ny)
{
	x = nx;
	y = ny;
}

void TextBox::saveUnder()
{
}

void TextBox::draw()
{
	display();
}

void TextBox::restoreUnder()
{
}

/* Prints the text, and the cursor after it while it blinks on, into this box's view. The text
 * went through printf as a format; printed as is here, so a '%' in a name shows as typed. */
void TextBox::display()
{
	View *saved;
	int16_t width;

	saved = printer->target;
	printer->target = this;
	printer->print(x, y, text);
	if (cursorVisible && active) {
		width = printer->measure(text, cursor);
		FontTextPrinter *p = printer;
		p->move(x + width, y + 1);
		p->printChar(cursorChar);
	}
	printer->target = saved;
}

void TextBox::reject()
{
}

uint8_t TextBox::contains(int16_t px, int16_t py)
{
	return 0;
}

}
