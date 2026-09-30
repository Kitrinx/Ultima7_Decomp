/* Black Gate ENDGAME.EXE, resident segment 57 (file offsets 0x01111c to 0x01120f, 243 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <alloc.h>
#include "strbuf.h"

#define WORK_BUFFER_SIZE 256

void String::free()
{
	if (text)
		delete text;
	text = 0;
	size = 0;
}

/* Replaces the buffer with one of n bytes, or as many as are left. */
char *String::allocate(unsigned n)
{
	free();
	if (coreleft() < n)
		n = coreleft();
	if (n > 0) {
		text = new char[n];
		*text = 0;
	} else
		text = 0;
	size = n;
	return text;
}

/* Adds s to the end of the text, as much of it as fits. */
void String::concat(char far *s)
{
	char *p;
	unsigned room;

	if (text) {
		p = text;
		room = size;
		while (*p) {
			p++;
			room--;
		}
		while (room && *s) {
			*p++ = *s++;
			room--;
		}
		if (room)
			*p = 0;
		else
			text[size - 1] = 0;
	}
}

void String::clear()
{
	if (text)
		*text = 0;
}

String WorkBuffer(WORK_BUFFER_SIZE);
