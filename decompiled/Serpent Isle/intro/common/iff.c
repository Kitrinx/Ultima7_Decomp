/* Serpent Isle INTRO.EXE, resident segment 73 (file offsets 0x014042 to 0x014bf6, 2996 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <string.h>
#include "easyfile.h"
#include "iff.h"

char FormId[] = "FORM";
char ListId[] = "CAT ";
char OpenError[] = "Unable to open '%s'.\n";
char FormatError[] = "File '%s' is not IFF format.\n";

static char formType[5];

inline IffChunk::IffChunk(IffChunk *outer, ChunkHeader *header)
{
	prev = outer;
	start = header->start;
	size = header->size;
	strcpy(id, header->id);
	end = header->end;
}

inline IffChunk::~IffChunk()
{
}

inline IffFile::IffFile(char *name, char mode)
{
	open(name, mode);
}

void IffFile::describe()
{
	Message text;

	text.format("Form=%4s\nChunk=%4s\n", form->id, chunk.id);
	message.append(text);
	File::describe();
}

/* Called before the file opens: forget any chunk and form. */
unsigned char IffFile::validate()
{
	offset = depth = 0;
	chunk.start = chunk.size = chunk.end = 0;
	chunk.id[4] = 0;
	form = 0;
	table = -1;
	entries = 0;
	return 1;
}

IffFile::~IffFile()
{
	while (depth > 0)
		leaveForm();
}

/* Leaves every form whose chunks have all been read. */
void IffFile::leaveFinished()
{
	while (depth > 0 && atFormEnd())
		leaveForm();
}

/* Back to the first chunk of the outermost form. */
void IffFile::rewind()
{
	while (form->prev)
		leaveForm();
	offset = FORM_HEADER;
}

/* Sizes are stored high byte first. */
long IffFile::readSize()
{
	char *p = (char *) &chunk.size;
	unsigned i;
	int t;

	File::read(p, 4, offset);
	offset += 4;
	for (i = 0; i < 2; i++) {
		t = p[i];
		p[i] = p[3 - i];
		p[3 - i] = t;
	}
	return chunk.size;
}

int IffFile::readForm()
{
	readHeader();
	if (isId(FormId)) {
		readId();
		return 1;
	} else
		return 0;
}

int IffFile::readList()
{
	readHeader();
	if (isId(ListId)) {
		readId();
		return 1;
	} else
		return 0;
}

int IffFile::findChunk(char *id)
{
	int found = 0;

	rewindForm();
	for (;;) {
		readHeader();
		if (isId(id)) {
			found = 1;
			break;
		} else
			skipChunk();
		if (atFormEnd())
			break;
	}
	return found;
}

int IffFile::findForm(char *id)
{
	int found = 0;

	rewindForm();
	for (;;) {
		if (readForm()) {
			if (isId(id)) {
				found = 1;
				enterForm();
				break;
			} else
				skipChunk();
		} else
			skipChunk();
		if (atFormEnd())
			break;
	}
	return found;
}

int IffFile::findList(char *id)
{
	int found = 0;

	rewindForm();
	for (;;) {
		if (readList()) {
			if (isId(id)) {
				found = 1;
				enterList();
				break;
			} else
				skipChunk();
		} else
			skipChunk();
		if (atFormEnd())
			break;
	}
	return found;
}

/* The chunk holds a table of offsets, one long each. */
void IffFile::startTable()
{
	table = offset;
	entries = chunk.size / 4;
}

void IffFile::startGrid(int width, int height)
{
	columns = width;
	rows = height;
	startTable();
}

/* Moves to the data a table entry points at; entrySize is the long stored there. */
int IffFile::findEntry(long index)
{
	long saved;
	long at;

	entrySize = 0;
	saved = offset;
	at = -1;
	if (entries > index) {
		offset = table + (index << 2);
		at = readLong();
	}
	if (at != -1) {
		offset = at;
		entrySize = readLong();
		return 1;
	} else {
		offset = saved;
		return 0;
	}
}

int IffFile::findCell(int column, int row)
{
	entrySize = 0;
	if (columns < column || rows < row)
		return 0;
	else
		return findEntry((long) (row * columns) + column);
}

/* Steps over a table of count offsets at the current place. */
void IffFile::skipTable(int count)
{
	mark = table = offset;
	entries = count;
	offset += entries << 2;
}

/* Called once the file is open: read the outer FORM. */
void IffFile::load()
{
	loaded = 1;
	if (readForm())
		formEnd = chunk.end;
}

void IffFile::setOffset(long at)
{
	offset = at;
}

void IffFile::read(long size, void far *buffer)
{
	File::read(buffer, size, offset);
	offset += size;
}

int IffFile::readWord()
{
	int value;

	File::read(&value, sizeof value, offset);
	offset += sizeof value;
	return value;
}

long IffFile::readLong()
{
	long value;

	File::read(&value, sizeof value, offset);
	offset += sizeof value;
	return value;
}

long IffFile::peekLong()
{
	long value;

	File::read(&value, sizeof value, offset);
	return value;
}

long IffFile::readBytes(void far *buffer, long size)
{
	long count;

	count = File::read(buffer, size, offset);
	offset += size;
	return count;
}

long IffFile::readChunk(void far *buffer)
{
	long count;

	count = File::read(buffer, chunk.size, offset);
	offset = chunk.end;
	return count;
}

/* Reads the chunk straight into a Voodoo block. */
long IffFile::loadChunk(long block)
{
	long count;

	if (dos->handle == 0)
		error("you're fucked");
	count = ReadHandleToVoodoo(dos->handle, offset, chunk.size, &block);
	offset = chunk.end;
	return count;
}

int IffFile::readByte()
{
	unsigned char value;

	File::read(&value, sizeof value, offset++);
	return value;
}

long IffFile::readAt(void far *buffer, long at, long size)
{
	return File::read(buffer, size, at);
}

void IffFile::readId()
{
	File::read(chunk.id, 4, offset);
	offset += 4;
}

/* Chunks are padded to an even length. */
void IffFile::readHeader()
{
	chunk.start = offset;
	readId();
	readSize();
	chunk.end = offset + chunk.size + (chunk.size & 1);
}

void IffFile::skipChunk()
{
	offset = chunk.end;
}

void IffFile::rewindForm()
{
	if (form == 0)
		offset = 0;
	else
		offset = form->start + FORM_HEADER;
}

void IffFile::enterForm()
{
	form = new IffChunk(form, &chunk);
	if (form == 0)
		errorCode(0xb0e0);
	depth++;
}

void IffFile::enterList()
{
	form = new IffChunk(form, &chunk);
	if (form == 0)
		errorCode(0xb0e0);
	depth++;
}

void IffFile::skipForm()
{
	offset = form->end;
}

void IffFile::leaveForm()
{
	IffChunk *outer = form;

	offset = form->end;
	form = form->prev;
	delete outer;
	depth--;
}

void IffFile::leaveList()
{
	leaveForm();
}

int IffFile::isId(char *id)
{
	return strcmp(chunk.id, id) == 0;
}

long IffFile::setRecordSize(long size)
{
	recordSize = size;
	return records = chunk.size / size;
}

void IffFile::readRecords(void *buffer)
{
	File::read(buffer, records * recordSize, offset);
}

int IffFile::atChunkEnd()
{
	return offset >= chunk.end;
}

int IffFile::atFormEnd()
{
	return offset >= form->end;
}

int IffFile::atFileEnd()
{
	return getLength() <= offset;
}

int IffFile::atEnd()
{
	char c;

	return File::read(&c, 1) != 1;
}

/* The type of the file's outer FORM, or 0 if it will not open. */
char *GetFormType(char *name)
{
	IffFile file;

	if (file.open(name, FILE_READ) != -1) {
		strcpy(formType, file.form->id);
		file.close();
		return formType;
	} else
		return 0;
}

/* Prints the file's forms and chunks, indented by depth. */
void DumpIff(char *name)
{
	IffFile file(name, FILE_READ);
	int i;

	file.rewind();
	file.enterForm();
	printf("FORM:%s(%ld)\n", file.form->id, file.form->size);
	while (file.depth > 0) {
		for (i = 0; i < file.depth; i++)
			printf(" ");
		if (file.readForm()) {
			file.enterForm();
			printf("FORM:%s(%ld)\n", file.form->id, file.form->size);
		} else {
			printf("CHUNK:%s(%ld)\n", file.chunk.id, file.chunk.size);
			file.skipChunk();
			if (file.atFormEnd())
				file.leaveFinished();
		}
	}
}
