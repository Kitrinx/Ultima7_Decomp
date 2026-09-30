/* Black Gate U7.EXE, overlay segment 269 (file offsets 0x07fc30 to 0x08007f, 1103 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dosio.h"
#include "vstring.h"

/* What operator+ returns. */
String TempString;
char EmptyString[] = "";

/* c if s holds it, else 0. */
uint8_t FindCharInSet(int8_t c, char *s)
{
	while (*s && *s != c)
		s++;
	return *s;
}

/* Where c first occurs in s, or -1. */
int16_t FindCharIndex(int8_t c, char *s)
{
	char *p;

	for (p = s; *p && *p != c; p++)
		;
	if (*p)
		return p - s;
	return -1;
}

/* 1 when one string begins with the other. */
int16_t MatchPrefix(char *a, char *b)
{
	while (*a && *b)
		if (*a++ != *b++)
			return 0;
	return 1;
}

char *String::allocate(int16_t n)
{
	if (n <= 1)
		return EmptyString;
	return (char *) malloc(n);
}

void String::release(char *p)
{
	if (p && p != EmptyString)
		free(p);
}

void String::assign(int16_t n, char *s)
{
	len = n;
	if (len > 0) {
		str = allocate(len + 1);
		if (str == 0)
			ReportError(0x7301);
		_fstrcpy(str, s);
	} else {
		str = EmptyString;
	}
}

/* n spaces. */
String::String(int16_t n)
{
	int16_t i;

	assign(n, "");
	for (i = 0; i < len; i++)
		str[i] = ' ';
	str[i] = 0;
}

String::~String()
{
	if (str)
		release(str);
}

/* s up to its first character from stops. */
void String::assignUntil(char *s, char *stops)
{
	char *p;
	int8_t c;

	for (p = s; *p && !FindCharInSet(*p, stops); p++)
		;
	c = *p;
	*p = 0;
	release(str);
	assign(_fstrlen(s), s);
	*p = c;
}

String &String::operator+=(char *s)
{
	append(s);
	return *this;
}

String &String::operator+=(int8_t c)
{
	char s[2];

	s[0] = c;
	s[1] = 0;
	append(s);
	return *this;
}

String &String::operator+=(String &s)
{
	append(s.str);
	return *this;
}

String &String::operator+(char *s)
{
	TempString = str;
	TempString += s;
	return TempString;
}

String &String::operator+(String &s)
{
	TempString = str;
	TempString += s;
	return TempString;
}

/* The old text goes last, so a string can be assigned to itself. */
String &String::operator=(String &s)
{
	char *old = str;
	char *src = s.str;

	assign(_fstrlen(s.str), src);
	release(old);
	return *this;
}

String &String::operator=(char *s)
{
	/* A null string read from the start of the data segment on DOS, which begins with a zero:
	 * an empty string. Usecode comparisons pass one (Node::compare, with a number operand). */
	if (s == 0)
		s = "";
	release(str);
	len = _fstrlen(s);
	str = allocate(len + 1);
	if (str == 0)
		ReportError(0x7302);
	_fstrcpy(str, s);
	return *this;
}

/* Prints into WorkString, which fmt may already be, and takes the result. */
String &String::format(char *fmt, ...)
{
	va_list ap;

	if (fmt != WorkString) {
		va_start(ap, fmt);
		vsprintf(WorkString, fmt, ap);
	}
	release(str);
	assign(_fstrlen(WorkString), WorkString);
	return *this;
}

void String::append(char *s)
{
	char *p, *q;
	int16_t n;

	n = _fstrlen(s);
	p = allocate(len + n + 1);
	if (p == 0)
		ReportError(0x7303);
	q = p + len;
	_fstrcpy(p, str);
	len += n;
	do
		*q++ = *s;
	while (*s++);
	release(str);
	str = p;
}

/* Character n, or the terminator when the string is shorter. */
int8_t String::charAt(int16_t n)
{
	int16_t i;

	for (i = 0; str[i] && i < n; i++)
		;
	return str[i];
}

void String::clear()
{
	if (len > 0)
		*this = "";
}

extern "C" void ResetVstringGlobals(void)
{
	memset((void *)&TempString, 0, sizeof(TempString));
	memset(EmptyString, 0, sizeof(EmptyString));
}

extern "C" void ConstructVstringGlobals(void)
{
	new (&TempString) String();
}
