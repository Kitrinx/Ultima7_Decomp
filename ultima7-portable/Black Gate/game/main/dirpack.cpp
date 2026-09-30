/* Black Gate U7.EXE, overlay segment 228 (file offsets 0x069c10 to 0x06a55c, 2380 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdio.h>
#include <string.h>
#include "lowlevel.h"
#include "plat.h"
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
	int16_t pending, next;
	int16_t *slots;
	SaveGameFile();
	~SaveGameFile();
	void flush();
	void add(char *, char *);
	void addMatching(char *);
	void save(char *, char *, char *, int16_t, int16_t);
	void extractPending(char *);
	void extract(char *, char *);
};

int32_t GetArchivedFileSize(char *name)
{
	FlexEntry entry;
	int32_t size;
	Flex archive;
	char filename[NAME_SIZE];
	int16_t i;
	archive.open(BuildPath(StaticPath, InitGameFileName, 0));
	for (i = 0; i < (int16_t)archive.hdr.count; ++i) {
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
	int16_t i;
	slots = new int16_t[SAVE_SLOTS];
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
	int16_t unusedStart = pending;
	int16_t unusedWritten = 0;
	int16_t i;

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

void SaveGameFile::add(char *directory, char *name)
{
	int16_t fd = OpenFileOrFail(BuildPath(directory, name, 0));
	int32_t size = plat_file_length(fd);
	if (gShapeManager.countFreeSlots(0) <= 0 ||
		!gShapeManager.hasRoomFor(size))
		flush();
	slots[pending] = gShapeManager.allocateBlock(size + NAME_SIZE, 0x7fff, 0);
	CacheEntry *block = gShapeManager.entry(slots[pending]);
	CopyFarToLinear(block->data, name, (int32_t)NAME_SIZE);
	int32_t data = block->data + NAME_SIZE;
	if (ReadHandleToVoodoo(fd, INT32_C(0), size, &data) != size)
		ReportInvalidSaveGame();
	DosClose(fd);
	++pending;
}

void SaveGameFile::addMatching(char *directory)
{
	uint8_t done;
	plat_find found;
	done = !plat_find_first(BuildPath(directory, "*.*", 0), &found);
	while (!done) {
		add(directory, found.name);
		done = !plat_find_next(&found);
	}
}

void ForceDeleteFile(char *name)
{
	plat_file_remove(name);
}

void SaveGameFile::save(char *directory, char *name, char *title, int16_t version, int16_t count)
{
	if (!plat_dir_exists(directory))
		ReportInvalidSaveGame();
	FlexHeader initial;
	initial.count = count;
	initial.addWaste(1, INT32_C(0));
	ForceDeleteFile(name);
	openForWrite(name, &initial);
	if (title)
		hdr.setTitle(title);
	hdr.saveVersion = version;
	DebugPrintfAtCoords(1, 1, "");
	next = 0;
	pending = 0;
	addMatching(directory);
	flush();
	close();
}

void SaveGameFile::extractPending(char *directory)
{
	int16_t i;

	for (i = 0; i < SAVE_SLOTS; ++i) {
		if (slots[i] != NO_BLOCK) {
			CacheEntry *block = gShapeManager.entry(slots[i]);
			char filename[NAME_SIZE];
			CopyLinearToFar(filename, block->data, (int32_t)NAME_SIZE);
			int16_t fd = CreateFileOrFail(BuildPath(directory, filename, 0));
			if (!WriteHandleFromVoodoo(fd, -INT32_C(1), block->size - NAME_SIZE, block->data + NAME_SIZE))
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
	while (next < (int16_t)hdr.count) {
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
