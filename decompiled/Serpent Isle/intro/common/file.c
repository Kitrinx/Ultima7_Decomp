/* Serpent Isle INTRO.EXE, resident segment 71 (file offsets 0x0134cd to 0x013e8a, 2493 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "file.h"

static char *ModeNames[] = { "CREATE", "READ", "APPEND" };

ErrorHandler File::defaultHandler = FatalMessage;

void File::describe()
{
	Message text;

	text.format("\nFilename=%s\nLength  =%ld\nPosition=%ld\nMode    =%s\n",
		name.get(), length, position, ModeNames[mode]);
	message.append(text);
	ErrorReporter::describe();
}

void File::setName(char *s)
{
	if (!isOpen())
		name.assign(s);
	else
		errorCode(0x1912);
}

void File::setMode(char m)
{
	if (!isOpen())
		mode = m;
	else
		errorCode(0x1913);
}

void File::setHandle(DosFile *d)
{
	if (dos == 0)
		dos = d;
	else
		errorCode(0x1990);
}

void File::set(char *s, char m)
{
	setName(s);
	setMode(m);
}

void File::clear()
{
	position = 0;
	start = 0;
	length = 0;
	parent = 0;
	name.assign(0);
	dos = 0;
}

File::File()
{
	setHandler(defaultHandler);
	clear();
}

File::File(DosFile *d)
{
	clear();
	setHandle(d);
}

File::File(char *s, char m)
{
	clear();
	open(s, m);
}

/* Opens the part of whole from one offset up to another as a file of its own. */
void File::openPart(File *whole, long from, long to)
{
	clear();
	parent = whole;
	setWindow(whole->dos, from, to);
}

File::~File()
{
	if (isOpen())
		close();
	if (parent == 0)
		delete dos;
}

DosFile *File::createHandle()
{
	return new DosFile;
}

void File::makeHandle()
{
	if (dos == 0) {
		dos = createHandle();
		if (dos == 0)
			errorCode(0x19e0);
	} else
		errorCode(0x1990);
}

unsigned char File::open()
{
	unsigned char ok = 0;
	unsigned char opened;

	if (validate()) {
		if (dos == 0)
			makeHandle();
		if (dos) {
			opened = dos->open(name, mode);
			if (opened) {
				ok = 1;
				position = 0;
				start = 0;
				length = dos->length();
				load();
			} else
				errorCode(0x1910);
		} else
			errorCode(0x19e0);
	}
	return ok;
}

unsigned char File::open(char *s, char m)
{
	set(s, m);
	return open();
}

unsigned char File::setWindow(DosFile *d, long from, long to)
{
	position = 0;
	start = from;
	length = to - from;
	setHandle(d);
	if (parent)
		return 1;
	else
		return open();
}

unsigned char File::close()
{
	unsigned char ok = 0;

	if (parent == 0) {
		if (dos->close())
			ok = 1;
		else
			errorCode(0x1980);
	}
	return ok;
}

unsigned char File::isOpen()
{
	unsigned char open = 0;

	if (dos)
		open = dos->isOpen();
	return open;
}

unsigned char File::atEnd()
{
	return position >= length;
}

void File::seek(long offset)
{
	long at;

	if (offset < 0 || offset > length)
		errorCode(0x1941);
	else {
		at = dos->seek(offset + start, FROM_START);
		if (at != -1)
			position = at - start;
		else {
			checkOpen();
			errorCode(0x1914);
		}
	}
}

void File::skip(long delta)
{
	long at = position + delta;

	if (at < 0 || at > length)
		errorCode(0x1943);
	else {
		at = dos->seek(delta, FROM_HERE);
		if (at != -1)
			position = at - start;
		else {
			checkOpen();
			errorCode(0x1943);
		}
	}
}

void File::seekEnd()
{
	long at = dos->seek(start + length, FROM_START);

	if (at != -1)
		position = at - start;
	else {
		checkOpen();
		errorCode(0x1940);
	}
}

void File::rewind()
{
	long at = dos->seek(start, FROM_START);

	if (at != -1)
		position = at - start;
	else {
		checkOpen();
		errorCode(0x1940);
	}
}

/* Reads size bytes at the offset given, or where the file is when that is -1. */
long File::read(void far *buffer, long size, long at)
{
	long count;
	long from = at;

	if (at == -1)
		from = position;
	count = dos->read(buffer, from + start, size);
	if (count != -1) {
		position = from + count;
		if (atEnd())
			seekEnd();
	} else {
		checkOpen();
		errorCode(0x1920);
	}
	return count;
}

unsigned char File::validate()
{
	return 1;
}

void File::load()
{
	return;
}

void File::notOpen()
{
	errorCode(0x1911);
}

long File::getLength()
{
	return length;
}

long File::getPosition()
{
	return position;
}

char *File::getName()
{
	return name;
}

void File::setDefaultHandler(ErrorHandler handler)
{
	defaultHandler = handler;
}

unsigned char File::reopen()
{
	return open();
}
