/* Black Gate U7.EXE, overlay segment 228 (file offsets 0x069c10 to 0x06a55c, 2380 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include <dir.h>
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include "lowlevel.h"
#include "dosio.h"
#include "easyfile.h"
#include "debug.h"
#include "flex.h"
#include "oops.h"
#include "u7manage.h"
#include "savegame.h"

#define SAVE_SLOTS  37      /* files gathered before they are written out */
#define NAME_SIZE   13      /* an 8.3 name and its terminator */
#define NO_BLOCK    0x7fff

struct SaveGameFile : FlexWriter {
	int pending, next;
	int *slots;
	SaveGameFile();
	~SaveGameFile();
	void flush();
	void add(char *);
	void addMatching(char *);
	void save(char *, char *, char *, int, int);
	void extractPending(char *);
	void extract(char *, char *);
};

long GetArchivedFileSize(char *name)
{
	FlexEntry entry;
	long size;
	Flex archive;
	char filename[NAME_SIZE];
	int i;
	archive.open(BuildPath(StaticPath, InitGameFileName, 0));
	for (i = 0; i < (int)archive.hdr.count; ++i) {
		archive.getEntry(i, &entry);
		if (!entry.empty()) {
			size = entry.size - NAME_SIZE;
			entry.size = NAME_SIZE;
			archive.readEntry(&entry, filename, 0);
			if (stricmp(name, filename) == 0)
				break;
		}
	}
	archive.close();
	return size;
}

SaveGameFile::SaveGameFile()
{
	int i;
	slots = new int[SAVE_SLOTS];
	if (!slots)
		ReportOutOfNearMemory();
	for (i = 0; i < SAVE_SLOTS; ++i)
		slots[i] = NO_BLOCK;
}

SaveGameFile::~SaveGameFile()
{
	delete slots;
}

void SaveGameFile::flush()
{
	int unusedStart = pending;
	int unusedWritten = 0;
	int i;

	for (i = 0; i < SAVE_SLOTS; ++i) {
		if (slots[i] != NO_BLOCK) {
			++unusedWritten;
			writeBlock(next++, gShapeManager.get(slots[i]),
				gShapeManager.entry(slots[i])->size, 0);
			gShapeManager.releaseBlock(slots[i]);
			slots[i] = NO_BLOCK;
			--pending;
		}
	}
}

void SaveGameFile::add(char *name)
{
	int fd = OpenFileOrFail(name);
	long size = filelength(fd);
	if (gShapeManager.countFreeSlots(0) <= 0 ||
		!gShapeManager.hasRoomFor(size))
		flush();
	slots[pending] = gShapeManager.allocateBlock(size + NAME_SIZE, 0x7fff, 0);
	CacheEntry *block = gShapeManager.entry(slots[pending]);
	CopyFarToLinear(block->data, name, (long)NAME_SIZE);
	long data = block->data + NAME_SIZE;
	if (ReadHandleToVoodoo(fd, 0L, size, &data) != size)
		ReportInvalidSaveGame();
	DosClose(fd);
	++pending;
}

void SaveGameFile::addMatching(char *pattern)
{
	unsigned char done;
	ffblk found;
	done = findfirst(pattern, &found, 0);
	while (!done) {
		add(found.ff_name);
		done = findnext(&found);
	}
}

void ForceDeleteFile(char *name)
{
	_chmod(name, 1, 0);
	unlink(name);
}

void SaveGameFile::save(char *directory, char *name, char *title, int version, int count)
{
	char previous[80];
	if (!getcwd(previous, sizeof(previous)))
		ReportInvalidSaveGame();
	if (chdir(directory))
		ReportInvalidSaveGame();
	FlexHeader initial;
	initial.count = count;
	initial.addWaste(1, 0L);
	char filename[80];
	sprintf(filename, "%s\\%s", previous, name);
	ForceDeleteFile(filename);
	openForWrite(filename, &initial);
	if (title)
		hdr.setTitle(title);
	hdr.saveVersion = version;
	DebugPrintfAtCoords(1, 1, "");
	next = 0;
	pending = 0;
	addMatching("*.*");
	flush();
	close();
	if (chdir(previous))
		ReportInvalidSaveGame();
}

void SaveGameFile::extractPending(char *directory)
{
	int i;

	for (i = 0; i < SAVE_SLOTS; ++i) {
		if (slots[i] != NO_BLOCK) {
			CacheEntry *block = gShapeManager.entry(slots[i]);
			char filename[NAME_SIZE];
			CopyLinearToFar(filename, block->data, (long)NAME_SIZE);
			int fd = CreateFileOrFail(BuildPath(directory, filename, 0));
			if (!WriteHandleFromVoodoo(fd, -1L, block->size - NAME_SIZE, block->data + NAME_SIZE))
				ReportInvalidSaveGame();
			DosClose(fd);
			gShapeManager.releaseBlock(slots[i]);
			slots[i] = NO_BLOCK;
			--pending;
		}
	}
}

void SaveGameFile::extract(char *name, char *directory)
{
	FlexEntry entry;

	openOrFail(name);
	pending = 0;
	next = 0;
	while (next < (int)hdr.count) {
		if (getEntry(next, &entry)) {
			if (gShapeManager.countFreeSlots(0) <= 0 ||
				!gShapeManager.hasRoomFor(entry.size))
				extractPending(directory);
			slots[pending] = gShapeManager.allocateBlock(entry.size, 0x7fff, 0);
			readEntryToVoodoo(&entry, gShapeManager.get(slots[pending]), 0);
			++pending;
		}
		++next;
	}
	extractPending(directory);
	close();
}
