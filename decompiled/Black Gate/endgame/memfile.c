/* Black Gate ENDGAME.EXE, resident segment 67 (file offsets 0x01179a to 0x011c9f, 1285 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "memsys.h"
#include "memfile.h"

/* Takes as large a work buffer as far memory allows, up to bufferSize. */
void MemoryFile::load()
{
	File::load();
	while (Memory.available(FAR_MEMORY) <= bufferSize) {
		bufferSize >>= 1;
		if (bufferSize == 0) {
			errorCode(0x1ae0);
			break;
		}
	}
	buffer = Memory.allocate(bufferSize, FAR_MEMORY, 0, 3);
}

MemoryFile::MemoryFile(DosFile *d, long size) : File(d)
{
	bufferSize = size;
	buffer = 0;
}

MemoryFile::~MemoryFile()
{
	if (buffer)
		Memory.release(&buffer, FAR_MEMORY, 0x1a04);
}

/* Writes at an offset; bytes past the end are first made room for. at == -1 writes at the position. */
long MemoryFile::write(void far *buf, long size, long at)
{
	long written = 0;
	long offset;
	long end;
	long extra;

	if (parent == 0) {
		offset = at;
		if (at == -1)
			offset = position;
		extra = 0;
		end = 0;
		if (offset + size > length) {
			end = offset + size;
			extra = end - length;
			written = insert(buf, extra, length);
		}
		if (written == end) {
			written = dos->write(buf, offset + start, size);
			if (written != -1) {
				position = offset + written;
			} else {
				checkOpen();
				errorCode(0x1a05);
			}
		} else {
			checkOpen();
			errorCode(0x1a01);
		}
	}
	return position;
}

/* Inserts size bytes at an offset, moving everything after it up one buffer at a time. */
long MemoryFile::insert(void far *buf, long size, long at)
{
	long remaining;
	long oldLength;
	long from;
	long to;
	long moved;
	long got;
	long chunk;
	unsigned char failed = 0;

	if (at == -1)
		at = position;
	oldLength = dos->length();
	if (at < oldLength) {
		remaining = oldLength - at;
		moved = dos->write(buf, oldLength, size);
		if (moved == -1) {
			checkOpen();
			errorCode(0x1a06);
			failed = 1;
		} else {
			chunk = bufferSize;
			if (chunk > remaining)
				chunk = remaining;
			to = dos->length() - chunk;
			from = oldLength - chunk;
			while (remaining) {
				got = dos->read(buffer, from, chunk);
				if (got != -1) {
					moved = dos->write(buffer, to, chunk);
					if (moved != -1) {
						remaining -= moved;
					} else {
						checkOpen();
						errorCode(0x1a08);
						failed = 1;
						break;
					}
				} else {
					checkOpen();
					errorCode(0x1a09);
					failed = 1;
					break;
				}
				if (chunk > remaining)
					chunk = remaining;
				from -= chunk;
				to -= chunk;
			}
		}
	}
	if (!failed) {
		moved = dos->write(buf, at, size);
		if (moved != -1) {
			position = at + size;
			length += size;
		} else {
			checkOpen();
			errorCode(0x1a07);
		}
	}
	return position;
}

DosFile *MemoryFile::createHandle()
{
	return new DosFile;
}

void MemoryFile::describe()
{
	File::describe();
}
