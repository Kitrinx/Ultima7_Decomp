/* Serpent Isle ENDGAME.EXE, resident segment 51 (file offsets 0x0106fc to 0x0108c7, 459 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <alloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "strbuf.h"

inline int Larger(int a, int b)
{
	return a > b ? a : b;
}

/* Makes the text a copy of s, growing the buffer to fit. */
char *String::assign(char far *s)
{
	unsigned n;

	if (s == 0) {
		free();
		allocate(0);
	} else if (text == s)
		return text;
	n = _fstrlen(s) + 1;
	if (text == 0 || size < n) {
		free();
		allocate(n);
	}
	if (text) {
		clear();
		concat(s);
		text[size - 1] = 0;
	}
	return text;
}

/* Adds s to the end, moving to a larger buffer when there is memory for one. */
char *String::append(char far *s)
{
	int length;
	unsigned n;

	if (s) {
		length = _fstrlen(s);
		n = Larger(size, strlen(text) + length + 1);
		if (size < n && coreleft() > size) {
			String longer(n);

			longer.concat(text);
			longer.concat(s);
			swap(&longer);
		} else
			concat(s);
	}
	return text;
}

void String::swap(String *other)
{
	char *oldText = text;
	unsigned oldSize = size;

	text = other->text;
	size = other->size;
	other->text = oldText;
	other->size = oldSize;
}

/* printf into the string. */
char *String::format(char *fmt, ...)
{
	va_list args;

	if (fmt != WorkBuffer) {
		va_start(args, fmt);
		vsprintf(WorkBuffer, fmt, args);
	}
	clear();
	assign(WorkBuffer);
	return text;
}

/* Drops the last character. */
char *String::chop()
{
	size--;
	text[size - 1] = 0;
	return text;
}
