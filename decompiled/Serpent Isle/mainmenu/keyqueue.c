/* Serpent Isle MAINMENU.EXE, resident segment 15 (file offsets 0x00e785 to 0x00e98d, 520 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <conio.h>
#include "keyqueue.h"

KeyQueue::KeyQueue()
{
	keys = 0;
	tail = 0;
	head = 0;
	size = 0;
	count = 0;
	translate = 0;
}

KeyQueue::KeyQueue(int length)
{
	keys = 0;
	tail = 0;
	head = 0;
	size = 0;
	count = 0;
	translate = 0;
	allocate(length);
}

KeyQueue::~KeyQueue()
{
	if (keys)
		delete keys;
}

void KeyQueue::allocate(int length)
{
	if (keys == 0) {
		keys = new int[length];
		if (keys)
			size = length;
	}
	reset();
}

/* Reads the oldest key without taking it; 0 when there is none. */
unsigned char KeyQueue::peek(int *key)
{
	*key = 0;
	if (count) {
		*key = *head;
		return 1;
	}
	return 0;
}

unsigned char KeyQueue::put(int key)
{
	if (count < size - 1) {
		*tail = key;
		if (++tail >= keys + size)
			tail = keys;
		count++;
		return 1;
	}
	return 0;
}

unsigned char KeyQueue::get(int *key)
{
	if (count) {
		*key = *head;
		if (++head >= keys + size)
			head = keys;
		count--;
		return 1;
	}
	return 0;
}

void KeyQueue::reset()
{
	tail = head = keys;
	count = 0;
}

/* Drains BIOS input if the queue has room when polling starts. */
void KeyQueue::poll()
{
	int key;

	if (count < size - 1) {
		while (kbhit() != 0) {
			key = getch();
			if (key == 0)
				key = getch() | KEY_EXTENDED;
			if (translate)
				key = translate(key);
			put(key);
		}
	}
}

void KeyQueue::setTranslate(int (far *f)(int key))
{
	translate = f;
}
