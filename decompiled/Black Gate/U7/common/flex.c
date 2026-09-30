/* Black Gate U7.EXE, resident segment 84 (file offsets 0x02ca19 to 0x02d073, 1626 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include <io.h>
#include <mem.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "init.h"
#include "flex.h"

void FlexHeader::setFlexMarks()
{
	magic = -1;
	eof = 0x1a;
	version = 0xcc;
}

void FlexHeader::clear()
{
	memset(this, 0, sizeof(FlexHeader));
	count = -1;
	checksum = computeChecksum();
}

/* the sum of the header's longs, less the checksum itself */
long FlexHeader::computeChecksum()
{
	long sum = 0;
	unsigned i;
	long *p = (long *) this;

	for (i = 0; i < sizeof(FlexHeader); i += sizeof(long), p++)
		sum += *p;
	sum -= checksum;
	return sum;
}

/* Reads the header from fd, keeping the count when the file has none. */
void FlexHeader::read(int fd)
{
	long savedCount;

	if (!valid()) {
		savedCount = count;
		DosRead(fd, 0L, sizeof(FlexHeader), this);
		if (!valid()) {
			clear();
			count = savedCount;
		}
		checksum = computeChecksum();
	}
}

/* Writes the header back when it has changed. */
void FlexHeader::writeIfChanged(int fd)
{
	if (valid() && computeChecksum() != checksum)
		DosWrite(fd, 0L, sizeof(FlexHeader), this);
}

/* A new header, its checksum one off so that it gets written. */
void FlexHeader::create(int entries, char *text)
{
	clear();
	count = entries;
	if (text)
		strncpy(title, text, 80);
	setFlexMarks();
	checksum = computeChecksum() - 1;
}

void FlexHeader::setTitle(char *text)
{
	unsigned i;

	strncpy(title, text, 80);
	for (i = 0; i < strlen(title); i++)
		if (title[i] == '_')
			title[i] = ' ';
}

char *FlexHeader::getTitle(char *buf)
{
	if (buf) {
		strcpy(buf, title);
		return buf;
	}
	return title;
}

void FlexHeader::addWaste(char flag, long bytes)
{
	wasteFlag = flag;
	if (wasteFlag)
		waste = 13;
	waste += bytes;
}

int FlexEntry::operator==(FlexEntry other)
{
	if (memcmp(this, &other, sizeof(FlexEntry)) == 0)
		return 1;
	return 0;
}

long Flex::getFileLength()
{
	if (isopen())
		return filelength(handle);
	return 0;
}

void Flex::fatalError(FlexEntry *entry, int code)
{
	FatalError("Flex error, \"%s\"\nat %ld, len %ld\n%04x", name, entry->offset, entry->size, code);
}

void Flex::setCount(int entries)
{
	if (entries != -1)
		hdr.count = entries;
}

void Flex::setName(char *path)
{
	if (name != path) {
		if (name)
			delete name;
		if (path == 0)
			name = 0;
		else {
			name = new char[strlen(path) + 1];
			strcpy(name, path);
		}
	}
}

void Flex::setError(int code)
{
	error = code;
}

/* the last error, cleared on request */
int Flex::getError(unsigned char clear)
{
	int code = error;

	if (clear)
		error = 0;
	return code;
}

void Flex::printError(FlexEntry *entry)
{
	printf("Flex error, \"%s\" at %ld, len %ld,  #%04X\n", name, entry->offset, entry->size, getError(1));
}

unsigned char Flex::open(char *path)
{
	setName(path);
	if (name == 0)
		ReportError(0xa901);
	handle = DosOpen(name);
	if (isopen())
		hdr.read(handle);
	return isopen();
}

void Flex::openOrFail(char *path)
{
	setName(path);
	if (name == 0)
		ReportError(0xa901);
	handle = OpenFileOrFail(name);
	if (isopen())
		hdr.read(handle);
}

void Flex::close()
{
	if (isopen()) {
		hdr.writeIfChanged(handle);
		DosClose(handle);
		handle = -1;
	}
}

/* Entry i of the table; past the end it is empty. */
unsigned char Flex::getEntry(int i, FlexEntry *entry)
{
	if (i >= hdr.count) {
		entry->size = 0;
		entry->offset = 0;
	} else if (DosRead(handle, (hdr.valid() ? sizeof(FlexHeader) : 0L) + i * sizeof(FlexEntry),
		sizeof(FlexEntry), entry) != sizeof(FlexEntry))
		ReportError(0xa904);
	return !entry->empty();
}

/* Reads an object into buf. A short read is fatal unless quiet, which only records it. */
unsigned char Flex::readEntry(FlexEntry *entry, void far *buf, unsigned char quiet)
{
	if (!entry->empty()) {
		if (DosRead(handle, entry->offset, entry->size, buf) != entry->size) {
			if (!quiet)
				fatalError(entry, 0xa905);
			else
				setError(0xa905);
		}
	}
	return !entry->empty();
}

/* the same, into a Voodoo memory block */
unsigned char Flex::readEntryToVoodoo(FlexEntry *entry, long block, unsigned char quiet)
{
	if (!entry->empty()) {
		if (ReadHandleToVoodoo(handle, entry->offset, entry->size, &block) != entry->size) {
			if (!quiet)
				fatalError(entry, 0xa906);
			else
				setError(0xa906);
		}
	}
	return !entry->empty();
}

unsigned char Flex::readRecord(int i, void far *buf, unsigned char quiet)
{
	FlexEntry entry;

	getEntry(i, &entry);
	return readEntry(&entry, buf, quiet);
}

unsigned char Flex::readRecordToVoodoo(int i, long block, unsigned char quiet)
{
	FlexEntry entry;

	getEntry(i, &entry);
	return readEntryToVoodoo(&entry, block, quiet);
}
