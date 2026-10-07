/* Serpent Isle MAINMENU.EXE, resident segment 24 (file offsets 0x00fe80 to 0x0101e1, 865 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "chkfile.h"
#include "flex.h"
#include "memapi.h"
#include "oops.h"
#include "textfile.h"

namespace Shared {

TextFile::TextFile(char *name)
{
	lines = 0;
	text = 0;
	load(name);
}

TextFile::TextFile(char *flexName, int16_t entry)
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

void TextFile::allocate(int32_t size)
{
	text = (char *) AllocateFarHeap(size, 0);
	if (text == 0)
		ReportOutOfFarMemory();
}

/* Counts the lines, then ends each with a zero and records where it starts. */
void TextFile::findLines(int32_t length)
{
	count = split(length, 0);
	lines = (char * *) AllocateFarHeap(count * sizeof(char *), 0);
	if (lines == 0)
		ReportOutOfFarMemory();
	split(length, 1);
}

void TextFile::load(char *name)
{
	DataFile file;
	int32_t size;

	if (file.open(name, FILE_OPEN) != 1)
		ReportFileNotFound(name);
	size = file.getLength();
	allocate(size);
	if (file.read(text, size) != size)
		ReportFileReadError(name);
	findLines(size);
	file.close();
}

void TextFile::load(char *flexName, int16_t entry)
{
	Flex flex;

	flex.open(flexName);
	FlexEntry where;
	int32_t size;
	flex.getEntry(entry, &where);
	size = where.size;
	allocate(size);
	flex.readEntry(&where, text, 0);
	flex.close();
	findLines(size);
}

char *TextFile::getLine(int16_t n)
{
	if (text && lines)
		return lines[n];
	return 0;
}

/*
 * Counts the lines of the text; with mark set, also ends each line with a zero and
 * records where it starts. A CR LF pair ends one line.
 */
int16_t TextFile::split(int32_t length, int8_t mark)
{
	char *p = text;
	char *line = text;
	int16_t n = 0;

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

}
