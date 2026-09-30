/* Black Gate U7.EXE, resident segment 10 (file offsets 0x00fe71 to 0x010492, 1569 bytes).
 * Borland C++ 2.0 -mm -O -G -P -b- rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include <string.h>
#include "init.h"
#include "oops.h"
#include "plat.h"
#include "dosio.h"
#include "chkfile.h"

#define NO_NAME     0x7000
#define NOT_OPEN    0x7001
#define READ_ONLY   0x7002

void DataFile::fail(int16_t code)
{
	FatalError("File %s,  Error %04x", name, code);
}

/* opens the named file; reports the name on failure */
DataFile::DataFile(char *path, int8_t how)
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
void DataFile::setName(char *path, int8_t how)
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
uint8_t DataFile::open(char *path, FileMode how)
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
int8_t DataFile::reopen(int8_t how)
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

int32_t DataFile::tell()
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosSeek(handle, INT32_C(0), SEEK_CUR);
}

int32_t DataFile::seek(int32_t offset)
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosSeek(handle, offset, SEEK_SET);
}

int32_t DataFile::skip(int32_t count)
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosSeek(handle, count, SEEK_CUR);
}

/* 1 at the end of the file */
int16_t DataFile::atEnd()
{
	if (handle == -1)
		fail(NOT_OPEN);
	return tell() == getLength();
}

int32_t DataFile::getLength()
{
	if (handle == -1)
		fail(NOT_OPEN);
	return plat_file_length(handle);
}

int32_t DataFile::seekEnd()
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosSeek(handle, INT32_C(0), SEEK_END);
}

int8_t DataFile::readByte()
{
	int8_t c;

	if (handle == -1)
		fail(NOT_OPEN);
	DosRead(handle, -INT32_C(1), INT32_C(1), &c);
	return c;
}

int16_t DataFile::readWord()
{
	int16_t v;

	read(&v, sizeof v);
	return v;
}

int32_t DataFile::readLong()
{
	int32_t v;

	read(&v, sizeof v);
	return v;
}

/* reads up to limit bytes into buf, stopping before the byte stop */
uint32_t DataFile::readUntil(int8_t stop, char *buf, uint32_t limit)
{
	int8_t ch;
	uint32_t count;
	int32_t n;

	if (handle == -1)
		fail(NOT_OPEN);
	count = 0;
	do {
		n = DosRead(handle, -INT32_C(1), INT32_C(1), &ch);
		if (n >= 1) {
			if (ch == stop)
				break;
			buf[count++] = ch;
		} else
			break;
	} while (count < limit);
	return count;
}

int32_t DataFile::read(void *buf, int32_t size)
{
	if (handle == -1)
		fail(NOT_OPEN);
	return DosRead(handle, -INT32_C(1), size, buf);
}

/* reads count records of size bytes each into buf; 1 when all arrive */
int8_t DataFile::readRecords(uint32_t count, char *buf, uint32_t size)
{
	uint32_t i;

	if (handle == -1)
		fail(NOT_OPEN);
	for (i = 0; i < count; i++)
		if (DosRead(handle, -INT32_C(1), size, buf + size * i) != size)
			return 0;
	return 1;
}

void DataFile::writeByte(int8_t value)
{
	if (handle == -1)
		fail(NOT_OPEN);
	if (mode == FILE_OPEN)
		fail(READ_ONLY);
	DosWrite(handle, -INT32_C(1), INT32_C(1), &value);
}

int16_t DataFile::write(void *buf, int32_t size)
{
	if (handle == -1)
		fail(NOT_OPEN);
	if (mode == FILE_OPEN)
		fail(READ_ONLY);
	return DosWrite(handle, -INT32_C(1), size, buf);
}

/* writes count records of size bytes each from buf */
void DataFile::writeRecords(uint32_t count, char *buf, uint32_t size)
{
	uint32_t i;

	if (handle == -1)
		fail(NOT_OPEN);
	if (mode == FILE_OPEN)
		fail(READ_ONLY);
	for (i = 0; i < count; i++)
		DosWrite(handle, -INT32_C(1), size, buf + size * i);
}

/* copies count bytes from source into this file */
void DataFile::copyFrom(DataFile *source, uint32_t count)
{
	int8_t c;
	uint32_t i;

	for (i = 0; i < count; i++) {
		c = source->readByte();
		writeByte(c);
	}
}
