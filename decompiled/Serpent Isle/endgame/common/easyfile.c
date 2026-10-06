/* Serpent Isle ENDGAME.EXE, resident segment 5 (file offsets 0x008be2 to 0x008e83, 673 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <io.h>
#include <string.h>
#include "dosio.h"
#include "errors.h"
#include "vooalloc.h"
#include "lowlevel.h"
#include "memsys.h"
#include "easyfile.h"

#define BUFSIZE     10000L

/* the transfer buffer between files and far blocks */
void far *FileTransferBuffer = 0;

/* Opens name, stopping the program when it cannot. */
int OpenFileOrFail(char *name)
{
	char msg[100];
	int fd;

	fd = DosOpen(name);
	if (fd == -1) {
		_fstrcpy(msg, name);
		FatalError("Failed to open file \"%s\"", msg);
	}
	return fd;
}

inline long ClampLong(long lo, long v, long hi)
{
	if (v < lo)
		return lo;
	else if (v > hi)
		return hi;
	else
		return v;
}

/* Reads size bytes at offset (-1: where the file stands) into the far block, allocating it when
 * *block is 0. Returns the bytes read. */
long ReadHandleToVoodoo(int fd, long offset, long size, long *block)
{
	long done = 0;
	MemHandle buffer(BUFSIZE, FAR_MEMORY, 0, 1);

	if (*block == 0)
		*block = AllocateVoodooMemory(&VoodooXmsBlock, size);
	if (*block == 0)
		FatalError("No voodoo.");
	if (offset != -1)
		DosSeek(fd, offset, 0);
	while (done < size) {
		long chunk = ClampLong(0, size - done, BUFSIZE);
		long got = DosRead(fd, -1L, chunk, buffer.pointer());
		if (got != chunk) {
			done += got;
			break;
		}
		CopyFarToLinear(*block + done, buffer.lock(), chunk);
		done += chunk;
	}
	return done;
}

/* Reads a whole file into a far block. */
long LoadFileToVoodoo(char *name, long block)
{
	int fd;
	long size;

	fd = OpenFileOrFail(name);
	size = filelength(fd);
	ReadHandleToVoodoo(fd, 0L, size, &block);
	DosClose(fd);
	return size;
}
