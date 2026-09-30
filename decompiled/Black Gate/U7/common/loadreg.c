/* Black Gate U7.EXE, overlay segment 242 (file offsets 0x06f800 to 0x06fbbd, 957 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z -d rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

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
#define IS_NPC(r) ((char) ((ItemTypeClassFlags[TYPE_CLASS(ITEM((r).off))] & CLASS_NPC) != 0))
#define IS_VALID(v) ((char) ((v) != 0))

/* one bit per region: it has been written to a temp file */
#define SAVED(region) ((char) (SavedRegionBits[(region) / 8] & (1 << (region) % 8)))

#define TEMP_EXT ".$$$"

unsigned char SavedRegionBits[32];
char *IregPathFormat;
char *MapPath;

unsigned char CurrentRegion = 255;

/* forget every saved region and empty the four region slots */
void far ForgetSavedRegions(void)
{
	int i;

	for (i = 0; i < 32; i++)
		SavedRegionBits[i] = 0;
	for (i = 0; i < 4; i++)
		LoadedRegions[i] = 255;
}

void far EmptyRegionStub(void)
{
}

/* write the items of region slot to the region's temp file, then free them if asked */
void far SaveRegion(unsigned char *region, int slot, char unload)
{
	int fd;
	objref r;
	objref next;
	objref npc;
	int i, j;
	char name[80];

	if (*region > 143)
		DebugPrintfWait("Error: Saving Region %X\n", *region);
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
void far LoadRegion(unsigned char *region, int slot)
{
	objref list;
	int fd;
	int x, y;
	int i, j;
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

void far EmptyRegionHook(unsigned char *region, void far *buf)
{
}

/* read the region's 512 bytes of chunk numbers from the map file */
void far LoadRegionMap(unsigned char *region, void far *buf)
{
	char name[80];
	int fd;

	strcpy(name, MapPath);
	fd = DosOpen(name);
	if (fd == -1)
		ReportInvalidSaveGame();
	else {
		DosRead(fd, (long) *region << 9, 512L, buf);
		DosClose(fd);
	}
}
