/* Black Gate U7.EXE, resident segment 10 (file offsets 0x00fe71 to 0x010492, 1569 bytes).
 * Borland C++ 2.0 -mm -O -G -P -b- rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <io.h>
#include <string.h>
#include "init.h"
#include "oops.h"
#include "chkfile.h"

#define NO_NAME     0x7000
#define NOT_OPEN    0x7001
#define READ_ONLY   0x7002

/* The DOS calls, declared here because this file uses the write result as an int. */
extern "C" {
extern int far pascal DosWrite(int, long, long, void far *);
extern long far pascal DosRead(int, long, long, void far *);
extern int far pascal DosOpen(const char far *);
extern void far pascal DosClose(int);
extern int far pascal DosCreate(const char far *);
extern long far pascal DosSeek(int, long, char);
}

void DataFile::fail(int code)
{
	FatalError("File %s,  Error %04x", name, code);
}

/* opens the named file; reports the name on failure */
DataFile::DataFile(char far *path, char how)
{
	handle = -1;
	name = 0;
	if (open(path, how) == 0)
		ReportFileNotFound(name);
}

DataFile::~DataFile()
{
	if (handle != -1)
		close();
	if (name != 0)
		delete name;
}

/* keeps a private copy of the name, and the mode */
void DataFile::setName(char far *path, char how)
{
	if (name != 0)
		delete name;
	name = new char[_fstrlen(path) + 1];
	if (name == 0)
		ReportOutOfNearMemory();
	_fstrcpy(name, path);
	mode = how;
}

/* opens under a new name, creating the file for FILE_CREATE; 1 on success */
unsigned char DataFile::open(char far *path, FileMode how)
{
	if (handle != -1)
		close();
	setName(path, how);
	if (how == FILE_CREATE)
		handle = DosCreate(name);
	else
		handle = DosOpen(name);
	if (handle < 0) {
		handle = -1;
		return 0;
	}
	return 1;
}

/* reopens the same name in the given mode; 1 on success */
char DataFile::reopen(char how)
{
	if (handle != -1)
		close();
	if (name == 0)
		fail(NO_NAME);
	mode = how;
	if (mode == FILE_CREATE)
		handle = DosCreate(name);
	else
		handle = DosOpen(name);
	if (handle < 0) {
		handle = -1;
		return 0;
	}
	return 1;
}

void DataFile::close()
{
	if (handle != -1) {
		DosClose(handle);
		handle = -1;
	}
}

long DataFile::tell()
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosSeek(handle, 0L, SEEK_CUR);
}

long DataFile::seek(long offset)
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosSeek(handle, offset, SEEK_SET);
}

long DataFile::skip(long count)
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosSeek(handle, count, SEEK_CUR);
}

/* 1 at the end of the file */
int DataFile::atEnd()
{
	if (handle == -1)
		fail(NOT_OPEN);
	return tell() == getLength();
}

long DataFile::getLength()
{
	if (handle == -1)
		fail(NOT_OPEN);
	return filelength(handle);
}

long DataFile::seekEnd()
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosSeek(handle, 0L, SEEK_END);
}

char DataFile::readByte()
{
	char c;

	if (handle == -1)
		fail(NOT_OPEN);
	DosRead(handle, -1L, 1L, &c);
	return c;
}

int DataFile::readWord()
{
	int v;

	read(&v, sizeof v);
	return v;
}

long DataFile::readLong()
{
	long v;

	read(&v, sizeof v);
	return v;
}

/* reads up to limit bytes into buf, stopping before the byte stop */
unsigned long DataFile::readUntil(char stop, char far *buf, unsigned long limit)
{
	char ch;
	unsigned long count;
	long n;

	if (handle == -1)
		fail(NOT_OPEN);
	count = 0;
	do {
		n = DosRead(handle, -1L, 1L, &ch);
		if (n >= 1) {
			if (ch == stop)
				break;
			buf[count++] = ch;
		} else
			break;
	} while (count < limit);
	return count;
}

long DataFile::read(void far *buf, long size)
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosRead(handle, -1L, size, buf);
}

/* reads count records of size bytes each into buf; 1 when all arrive */
char DataFile::readRecords(unsigned long count, char far *buf, unsigned long size)
{
	unsigned long i;

	if (handle == -1)
		fail(NOT_OPEN);
	for (i = 0; i < count; i++)
		if (DosRead(handle, -1L, size, buf + size * i) != size)
			return 0;
	return 1;
}

void DataFile::writeByte(char value)
{
	if (handle == -1)
		fail(NOT_OPEN);
	if (mode == FILE_OPEN)
		fail(READ_ONLY);
	DosWrite(handle, -1L, 1L, &value);
}

int DataFile::write(void far *buf, long size)
{
	if (handle == -1)
		fail(NOT_OPEN);
	if (mode == FILE_OPEN)
		fail(READ_ONLY);
	return DosWrite(handle, -1L, size, buf);
}

/* writes count records of size bytes each from buf */
void DataFile::writeRecords(unsigned long count, char far *buf, unsigned long size)
{
	unsigned long i;

	if (handle == -1)
		fail(NOT_OPEN);
	if (mode == FILE_OPEN)
		fail(READ_ONLY);
	for (i = 0; i < count; i++)
		DosWrite(handle, -1L, size, buf + size * i);
}

/* copies count bytes from source into this file */
void DataFile::copyFrom(DataFile *source, unsigned long count)
{
	char c;
	unsigned long i;

	for (i = 0; i < count; i++) {
		c = source->readByte();
		writeByte(c);
	}
}
