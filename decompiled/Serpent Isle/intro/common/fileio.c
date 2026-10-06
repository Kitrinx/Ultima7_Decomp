/* Serpent Isle INTRO.EXE, resident segment 56 (file offsets 0x011718 to 0x011812, 250 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "fileio.h"

void WriteWord(MemoryFile *file, int value, long at)
{
	file->write(&value, sizeof value, at);
}

void WriteByte(MemoryFile *file, char value, long at)
{
	file->write(&value, sizeof value, at);
}

void WriteLong(MemoryFile *file, long value, long at)
{
	file->write(&value, sizeof value, at);
}

int ReadWord(File *file, long at)
{
	int value;

	file->read(&value, sizeof value, at);
	return value;
}

char ReadByte(File *file, long at)
{
	char value;

	file->read(&value, sizeof value, at);
	return value;
}

long ReadLong(File *file, long at)
{
	long value;

	file->read(&value, sizeof value, at);
	return value;
}

void ReadAll(File *file, void far *buffer)
{
	long size = file->getLength();

	file->read(buffer, size, 0L);
}
