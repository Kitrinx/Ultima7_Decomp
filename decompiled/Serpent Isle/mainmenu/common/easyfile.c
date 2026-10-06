/* Serpent Isle MAINMENU.EXE, resident segment 28 (file offsets 0x010c12 to 0x011060, 1102 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- -d rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <string.h>
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "oops.h"
#include "memapi.h"
#include "easyfile.h"
#include "fileutil.h"

#define NPATHS      6
#define PATHSIZE    50
#define BUFSIZE     10240L

char PathBuffers[NPATHS][PATHSIZE];

extern "C" char *FormatWorkstring(char *, ...);

/* the transfer buffer between files and far blocks, made on first use */
void far *FileTransferBuffer = 0;
static int PathBufferIndex = 0;

/* Copies name into the next of the rotating path buffers. */
char *StorePath(char far *name)
{
	PathBufferIndex = (PathBufferIndex + 1) % NPATHS;
	_fstrncpy(PathBuffers[PathBufferIndex], name, PATHSIZE - 1);
	PathBuffers[PathBufferIndex][PATHSIZE - 1] = 0;
	return PathBuffers[PathBufferIndex];
}

/* dir\name, adding ext when name has none; no directory for "" or ".". */
char *BuildPath(char *dir, char *name, char *ext)
{
	if (ext != 0 && strchr(name, '.') == 0) {
		if (strcmp(dir, "") == 0 || strcmp(dir, ".") == 0)
			FormatWorkstring("%s%s", name, ext);
		else
			FormatWorkstring("%s\\%s%s", dir, name, ext);
	} else {
		if (strcmp(dir, "") == 0 || strcmp(dir, ".") == 0)
			FormatWorkstring("%s", name);
		else
			FormatWorkstring("%s\\%s", dir, name);
	}
	return StorePath(GetWorkString(&WorkString));
}

/* The path of a numbered file: fmt formats n into the name. */
char *BuildNumberedPath(char *dir, char *fmt, int n, char *ext)
{
	char name[80];

	sprintf(name, fmt, n);
	return BuildPath(dir, name, ext);
}

/* The same, with the temporary extension when temp is set. */
char *BuildNumberedTempPath(char *dir, char *fmt, int n, char temp)
{
	char name[80];

	sprintf(name, fmt, n);
	if (temp)
		return BuildPath(dir, name, ".$$$");
	else
		return BuildPath(dir, name, 0);
}

/* Replaces the string at *p with a new copy of s. */
void ReplaceString(char **p, char *s)
{
	if (*p != 0)
		delete *p;
	*p = new char[strlen(s) + 1];
	strcpy(*p, s);
}

/* Opens name, stopping the game when it cannot. */
int OpenFileOrFail(char *name)
{
	char msg[100];
	int fd;

	fd = DosOpen(name);
	if (fd == -1) {
		_fstrcpy(msg, name);
		ReportFileNotFound(msg);
	}
	return fd;
}

/* Creates name, stopping the game when it cannot. */
int CreateFileOrFail(char *name)
{
	char msg[100];
	int fd;

	fd = DosCreate(name);
	if (fd == -1) {
		_fstrcpy(msg, name);
		ReportFileNotFound(msg);
	}
	return fd;
}

/* Reads size bytes at offset (-1: where the file stands) into the far block, allocating it when
 * *block is 0. Returns the bytes read. */
long ReadHandleToVoodoo(int fd, long offset, long size, long *block)
{
	long done = 0;
	long chunk;
	long got;

	if (FileTransferBuffer == 0)
		FileTransferBuffer = AllocateFarHeap(BUFSIZE, 0);
	if (*block == 0)
		*block = AllocateVoodooMemory(&VoodooXmsBlock, size);
	if (*block == 0)
		ReportOutOfVoodooMemory();
	if (offset != -1)
		DosSeek(fd, offset, 0);
	while (done < size) {
		chunk = ClampLong(0L, size - done, BUFSIZE);
		got = DosRead(fd, -1L, chunk, FileTransferBuffer);
		if (got != chunk) {
			done += got;
			break;
		}
		CopyFarToLinear(*block + done, FileTransferBuffer, chunk);
		done += chunk;
	}
	return done;
}

/* Writes size bytes of the far block at offset (-1: where the file stands). */
unsigned char WriteHandleFromVoodoo(int fd, long offset, long size, long block)
{
	long done;
	long chunk;

	if (FileTransferBuffer == 0)
		FileTransferBuffer = AllocateFarHeap(BUFSIZE, 0);
	done = 0;
	if (offset != -1)
		DosSeek(fd, offset, 0);
	while (done < size) {
		chunk = ClampLong(0L, size - done, BUFSIZE);
		CopyLinearToFar(FileTransferBuffer, block + done, chunk);
		if (!DosWrite(fd, -1L, chunk, FileTransferBuffer))
			return 0;
		done += chunk;
	}
	return 1;
}
