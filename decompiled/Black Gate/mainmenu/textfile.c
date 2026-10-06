/* Black Gate MAINMENU.EXE, resident segment 24 (file offsets 0x00f505 to 0x00f86f, 874 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "chkfile.h"
#include "flex.h"
#include "memapi.h"
#include "oops.h"
#include "textfile.h"

TextFile::TextFile(char *name)
{
	lines = 0;
	text = 0;
	load(name);
}

TextFile::TextFile(char *flexName, int entry)
{
	lines = 0;
	text = 0;
	load(flexName, entry);
}

TextFile::~TextFile()
{
	if (lines)
		FreeFarHeap(lines);
	if (text)
		FreeFarHeap(text);
}

void TextFile::allocate(long size)
{
	text = (char far *) AllocateFarHeap(size, 0);
	if (text == 0)
		ReportOutOfFarMemory();
}

/* Counts the lines, then ends each with a zero and records where it starts. */
void TextFile::findLines(long length)
{
	count = split(length, 0);
	lines = (char far * far *) AllocateFarHeap(count * sizeof(char far *), 0);
	if (lines == 0)
		ReportOutOfFarMemory();
	split(length, 1);
}

void TextFile::load(char *name)
{
	DataFile file;
	long size;

	if (file.open(name, FILE_OPEN) != 1)
		ReportFileNotFound(name);
	size = file.getLength();
	allocate(size);
	if (file.read(text, size) != size)
		ReportFileReadError(name);
	findLines(size);
	file.close();
}

void TextFile::load(char *flexName, int entry)
{
	FlexEntry where;
	long size;
	Flex flex;

	flex.open(flexName);
	flex.getEntry(entry, &where);
	size = where.size;
	allocate(size);
	flex.readEntry(&where, text, 0);
	flex.close();
	findLines(size);
}

char far *TextFile::getLine(int n)
{
	if (text && lines)
		return lines[n];
	return 0;
}

/*
 * Counts the lines of the text; with mark set, also ends each line with a zero and
 * records where it starts. A CR LF pair ends one line.
 */
int TextFile::split(long length, char mark)
{
	char far *p = text;
	char far *line = text;
	int n = 0;

	while (length--) {
		if (*p == '\r' || *p == '\n') {
			if (mark) {
				*p = 0;
				lines[n] = line;
			}
			n++;
			p++;
			if (length == 0)
				break;
			length--;
			line = p;
			if (*p == '\r' || *p == '\n')
				line++;
		}
		p++;
	}
	return n;
}
