/* Black Gate MAINMENU.EXE module KEYQUEUE: a ring of keys read from the keyboard. */

#include "u7port.h"
#include "../shared/keys.h"
#include "keyqueue.h"

namespace MainMenu {

KeyQueue::KeyQueue()
{
	keys = 0;
	tail = 0;
	head = 0;
	size = 0;
	count = 0;
	translate = 0;
}

KeyQueue::KeyQueue(int16_t length)
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
	delete[] keys;
}

void KeyQueue::allocate(int16_t length)
{
	if (keys == 0) {
		keys = new int16_t[length];
		size = length;
	}
	reset();
}

/* Reads the oldest key without taking it; 0 when there is none. */
uint8_t KeyQueue::peek(int16_t *key)
{
	*key = 0;
	if (count) {
		*key = *head;
		return 1;
	}
	return 0;
}

uint8_t KeyQueue::put(int16_t key)
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

uint8_t KeyQueue::get(int16_t *key)
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

/* Moves every key waiting into the queue while it has room. */
void KeyQueue::poll()
{
	int16_t key;

	if (count < size - 1) {
		while (Shared::KeyHit() != 0) {
			key = Shared::GetKey();
			if (key == 0)
				key = Shared::GetKey() | KEY_EXTENDED;
			if (translate)
				key = translate(key);
			put(key);
		}
	}
}

void KeyQueue::setTranslate(int16_t (*f)(int16_t key))
{
	translate = f;
}

}
