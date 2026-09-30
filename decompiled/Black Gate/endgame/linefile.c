/* Black Gate ENDGAME.EXE, resident segment 65 (file offsets 0x0115f0 to 0x01175c, 364 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "file.h"

#define LINE_BUFFER     160

LineFile::LineFile()
{
	buffer = new char[LINE_BUFFER];
	if (buffer == 0)
		errorCode(0x33e0);
	next = buffer;
	end = buffer;
}

LineFile::~LineFile()
{
	if (buffer)
		delete buffer;
	if (isOpen())
		close();
}

/* Refills the buffer once it is used up; true while characters remain. */
unsigned char LineFile::fill()
{
	int count;

	if (next >= end) {
		count = getLength() - getPosition();
		if (count > LINE_BUFFER)
			count = LINE_BUFFER;
		if (count)
			read(buffer, count);
		next = buffer;
		end = buffer + count;
	}
	return next < end;
}

/* Copies the next line, without its CR and LF, into size bytes and returns its length. */
int LineFile::readLine(char *line, unsigned size)
{
	int count = 0;
	char c;

	if (size)
		while (--size && fill()) {
			c = *next++;
			if (c == '\n')
				break;
			if (c == '\r')
				continue;
			*line++ = c;
			count++;
		}
	*line = 0;
	return count;
}
