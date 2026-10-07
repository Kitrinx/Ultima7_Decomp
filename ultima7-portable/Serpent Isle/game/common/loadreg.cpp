/* Serpent Isle SI.EXE, resident segment 60 (file offsets 0x0292bd to 0x029661, 932 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z -d rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "item.h"
#include "debug.h"
#include "maps.h"
#include "oops.h"
#include "itemovr1.h"
#include "npcref.h"
#include "type.h"
#include "mapview.h"

#define ITEM_TYPE(p) ((p)->typeFrame & 0x3ff)
#define TYPE_CLASS(p) (gItemTypeInfo[ITEM_TYPE(p)].typeClass)
#define IS_NPC(r) ((int8_t) ((ItemTypeClassFlags[TYPE_CLASS(ITEM((r).off))] & CLASS_NPC) != 0))
#define IS_VALID(v) ((int8_t) ((v) != 0))

/* one bit per region: it has been written to a temp file */
#define SAVED(region) ((int8_t) (SavedRegionBits[(region) / 8] & (1 << (region) % 8)))

#define TEMP_EXT ".$$$"

uint8_t SavedRegionBits[32];
char *IregPathFormat;
char *MapPath;
int16_t UnusedRegionWord;                   /* never referenced */

uint8_t CurrentRegion = 255;

/* forget every saved region and empty the four region slots */
void ForgetSavedRegions(void)
{
	int16_t i;

	for (i = 0; i < 32; i++)
		SavedRegionBits[i] = 0;
	for (i = 0; i < 4; i++)
		LoadedRegions[i] = 255;
}

void EmptyRegionStub(void)
{
}

/* write the items of region slot to the region's temp file, then free them if asked */
void SaveRegion(uint8_t *region, int16_t slot, int8_t unload)
{
	int16_t fd;
	objref r;
	objref next;
	objref npc;
	int16_t i, j;
	char name[80];

	sprintf(name, IregPathFormat, *region);
	strcat(name, TEMP_EXT);
	fd = DosCreate(name);
	if (fd == -1)
		ReportInvalidSaveGame();
	else {
		for (i = 0; i < 16; i++)
			for (j = 0; j < 16; j++) {
				r = ChunkItemLists[slot][i][j];
				WriteItemTree(&r, fd);
			}
		DosClose(fd);
		if (unload)
			for (i = 0; i < 16; i++)
				for (j = 0; j < 16; j++) {
					r = ChunkItemLists[slot][i][j];
					while (IS_VALID(r.off)) {
						next = r.next();
						if (IS_NPC(r)) {
							npc = r;
							Item_moveOffMap(&npc);
						} else {
							Item_deleteContents(&r);
							Item_delete(&r);
						}
						r = next;
					}
				}
		SavedRegionBits[*region / 8] |= 1 << *region % 8;
	}
}

/* read the region's items into region slot, from its temp file if it was saved */
void LoadRegion(uint8_t *region, int16_t slot)
{
	objref list;
	int16_t fd;
	int16_t x, y;
	int16_t i, j;
	char name[80];

	sprintf(name, IregPathFormat, *region);
	if (SAVED(*region))
		strcat(name, TEMP_EXT);
	/* the region's origin, read but not used */
	x = RegionX[*region];
	y = RegionY[*region];
	fd = DosOpen(name);
	if (fd == -1 && SAVED(*region)) {
		CheatPrintfWait("Can't find temp file %s", name);
		sprintf(name, IregPathFormat, *region);
		fd = DosOpen(name);
		SavedRegionBits[*region / 8] &= ~(1 << *region % 8);
	}
	if (fd >= 0) {
		for (i = 0; i < 16; i++)
			for (j = 0; j < 16; j++) {
				list.off = 0;
				ReadItemTree(&list, fd, slot);
				list.off = 0;
				ChunkItemLists[slot][i][j] = ITEM(list.off)->next;
				ITEM(list.off)->next = 0;
			}
		DosClose(fd);
	}
}

void EmptyRegionHook(uint8_t *region, void *buf)
{
}

/* read the region's 512 bytes of chunk numbers from the map file */
void LoadRegionMap(uint8_t *region, void *buf)
{
	char name[80];
	int16_t fd;

	strcpy(name, MapPath);
	fd = DosOpen(name);
	if (fd == -1)
		ReportInvalidSaveGame();
	else {
		DosRead(fd, (int32_t) *region << 9, INT32_C(512), buf);
		DosClose(fd);
	}
}

extern "C" void ResetLoadregGlobals(void)
{
	memset(SavedRegionBits, 0, sizeof SavedRegionBits);
	IregPathFormat = 0;
	MapPath = 0;
	CurrentRegion = 255;
	UnusedRegionWord = 0;
}
