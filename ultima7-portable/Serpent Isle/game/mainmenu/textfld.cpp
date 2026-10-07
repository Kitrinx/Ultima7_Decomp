/* Serpent Isle MAINMENU.EXE, resident segment 10 (file offsets 0x00df66 to 0x00e240, 730 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <string.h>
#include "oops.h"
#include "keyqueue.h"
#include "textfld.h"

namespace MainMenu {

#define CURSOR_BLINK_TICKS  INT32_C(20)

#define KEY_BACKSPACE   8
#define KEY_ENTER       13
#define KEY_HOME        (KEY_EXTENDED + 0x47)
#define KEY_LEFT        (KEY_EXTENDED + 0x4b)
#define KEY_RIGHT       (KEY_EXTENDED + 0x4d)
#define KEY_END         (KEY_EXTENDED + 0x4f)
#define KEY_INSERT      (KEY_EXTENDED + 0x52)
#define KEY_DELETE      (KEY_EXTENDED + 0x53)

/* Removes the character at `at`, closing up the next count - 1; returns the one removed. */
int8_t RemoveChar(char *at, int16_t count)
{
	int8_t removed = *at;

	memmove(at, at + 1, count - 1);
	at[count - 1] = 0;
	return removed;
}

/* Opens a gap at `at` by moving the next count - 1 characters up one, and puts c in it. */
void InsertChar(char *at, int16_t count, int8_t c)
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
}

TextField::TextField(int16_t bufferSize, KeyQueue *queue)
{
	char *buffer = new char[bufferSize + 1];

	if (buffer == 0)
		ReportOutOfNearMemory();
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
int8_t TextField::update()
{
	int8_t entered = 0;
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
			if (locked == 0)
				locked = 0;
			else
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

}
