/* Serpent Isle INTRO.EXE, resident segment 72 (file offsets 0x013e8a to 0x014042, 440 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "file.h"

long DosFile::read(void far *buffer, long at, long size)
{
	long count = -1;

	if (handle)
		count = DOSREAD(handle, at, size, buffer);
	return count;
}

long DosFile::write(void far *buffer, long at, long size)
{
	long count = -1;

	if (handle && DOSWRITE(handle, at, size, buffer))
		count = size;
	return count;
}

long DosFile::seek(long offset, char whence)
{
	long at = -1;

	if (handle)
		at = DOSSEEK(handle, offset, whence);
	return at;
}

unsigned char DosFile::open(char *name, char mode)
{
	unsigned char opened = 0;

	if (handle == 0) {
		if (mode == FILE_CREATE)
			handle = DOSCREATE(name);
		else
			handle = DOSOPEN(name);
		opened = handle != -1;
		if (!opened)
			handle = 0;
	}
	return opened;
}

unsigned char DosFile::close()
{
	if (handle) {
		DOSCLOSE(handle);
		handle = 0;
	}
	return !handle;
}

/* The file's size; the file is left at its start. */
long DosFile::length()
{
	long here;
	long size = 0;

	if (handle) {
		here = DOSSEEK(handle, 0L, FROM_START);
		size = DOSSEEK(handle, 0L, FROM_END);
		DOSSEEK(handle, here, FROM_START);
	}
	return size;
}

long DosFile::position()
{
	return seek(0L, FROM_HERE);
}
