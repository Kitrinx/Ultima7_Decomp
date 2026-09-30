/* Black Gate U7.EXE, resident segment 59 (file offsets 0x022730 to 0x022e85, 1877 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "datanode.h"
#include "item.h"
#include "easyfile.h"
#include "bogus.h"
#include "itembuf.h"
#include "loadreg.h"
#include "missile.h"
#include "text.h"
#include "u7sound.h"
#include "type.h"
#include "mapview.h"
#include "u7ibuf.h"

/* ITEMNODE.DAT: the item buffer's free list and the loaded regions' item lists. */
struct ItemNodesSaver : DataNode {
	char *name();
	void load(char *dir);
	void save(char *dir);
};

/* U7IBUF.DAT: the item buffer. */
struct ItemBufferSaver : DataNode {
	char *name();
	void load(char *dir);
	void save(char *dir);
};

/* U7_OSTIA.DAT: loose game state. */
struct OstiaSaver : DataNode {
	char *name();
	void load(char *dir);
	void save(char *dir);
};

struct TypeNumber {
	unsigned id;
	TypeNumber(unsigned type) { id = type; }
};

#define INFO(s) (gItemTypeInfo[(s).id & 0x3ff])
#define TYPE_CLASS(s) (INFO(s).typeClass)
#define CLASS_FLAGS(s) (ItemTypeClassFlags[TYPE_CLASS(s)])
#define IS_ANIMATED(s) ((char) INFO(s).animated)
#define HEIGHT(s) (INFO(s).height)
#define IS_CLASS(s, c) ((char) (TYPE_CLASS(s) == (c)))
#define IS_NPC(s) ((char) ((CLASS_FLAGS(s) & CLASS_NPC) != 0))

int DeadPartyMembers[12];
ItemNodesSaver ItemNodesFile;
ItemBufferSaver ItemBufferFile;
OstiaSaver OstiaFile;
char *ItemNodeFileName = "ITEMNODE.DAT";
char *U7IBufFileName = "U7IBUF.DAT";
char *U7OstiaFileName = "U7_OSTIA.DAT";
unsigned char AvatarDontMove = 1;
unsigned char ArmageddonDone = 0;
int ActiveSailor = 0;
int CurrentVehicle = 0;
int ActiveBarge = 0;
int DeadPartyCount = 0;
unsigned char OinkMode = 0;

/* One bit per region: its item file has a newer copy waiting under .$$$. */
inline unsigned char IsRegionPending(unsigned char n)
{
	return SavedRegionBits[n / 8] & (1 << n % 8);
}

inline void ClearRegionPending(unsigned char n)
{
	SavedRegionBits[n / 8] &= ~(1 << n % 8);
}

/* An item's type: the low 10 bits of its typeFrame. */
inline unsigned ItemType(objref r)
{
	return ITEM(r.off)->typeFrame & 0x3ff;
}

char *ItemNodesSaver::name()
{
	return ItemNodeFileName;
}

void ItemNodesSaver::save(char *dir)
{
	int fd;
	int i;
	unsigned char region[2];
	char name[80];
	char tmp[80];

	fd = CreateFileOrFail(BuildPath(dir, ItemNodeFileName, 0));
	DosWrite(fd, -1L, sizeof ItemFreeList, &ItemFreeList);
	DosWrite(fd, -1L, sizeof ItemFreeCount, &ItemFreeCount);
	DosWrite(fd, -1L, sizeof DetachedItems.off, &DetachedItems.off);
	DosWrite(fd, -1L, sizeof ChunkItemLists, ChunkItemLists);
	DosWrite(fd, -1L, sizeof LoadedRegions, LoadedRegions);
	DosWrite(fd, -1L, sizeof CurrentRegion, &CurrentRegion);
	DosClose(fd);
	for (i = 0; i < 4; i++) {
		if (LoadedRegions[i] != 255) {
			region[0] = LoadedRegions[i];
			SaveRegion(region, i, 0);
		}
	}
	for (i = 0; i < 256; i++) {
		if (IsRegionPending(i)) {
			sprintf(name, IregPathFormat, i);
			strncpy(tmp, name, 80);
			strcat(tmp, ".$$$");
			unlink(name);
			rename(tmp, name);
			ClearRegionPending(i);
		}
	}
}

void ItemNodesSaver::load(char *dir)
{
	int fd;
	int i;

	fd = OpenFileOrFail(BuildPath(dir, ItemNodeFileName, 0));
	DosRead(fd, -1L, sizeof ItemFreeList, &ItemFreeList);
	DosRead(fd, -1L, sizeof ItemFreeCount, &ItemFreeCount);
	DosRead(fd, -1L, sizeof DetachedItems.off, &DetachedItems.off);
	DosRead(fd, -1L, sizeof ChunkItemLists, ChunkItemLists);
	DosRead(fd, -1L, sizeof LoadedRegions, LoadedRegions);
	DosRead(fd, -1L, sizeof CurrentRegion, &CurrentRegion);
	DosClose(fd);
	for (i = 0; i < 256; i++)
		ClearRegionPending(i);
}

char *ItemBufferSaver::name()
{
	return U7IBufFileName;
}

void ItemBufferSaver::save(char *dir)
{
	int fd;

	fd = CreateFileOrFail(BuildPath(dir, U7IBufFileName, 0));
	DosWrite(fd, -1L, ItemBufferBytes, ItemBuffer);
	DosClose(fd);
}

void ItemBufferSaver::load(char *dir)
{
	int fd;

	fd = OpenFileOrFail(BuildPath(dir, U7IBufFileName, 0));
	DosRead(fd, -1L, ItemBufferBytes, ItemBuffer);
	DosClose(fd);
}

char *OstiaSaver::name()
{
	return U7OstiaFileName;
}

void OstiaSaver::save(char *dir)
{
	int fd;

	fd = CreateFileOrFail(BuildPath(dir, U7OstiaFileName, 0));
	DosWrite(fd, -1L, sizeof AvatarDontMove, &AvatarDontMove);
	DosWrite(fd, -1L, sizeof CurrentVehicle, &CurrentVehicle);
	DosWrite(fd, -1L, sizeof ArmageddonDone, &ArmageddonDone);
	DosWrite(fd, -1L, sizeof ActiveSailor, &ActiveSailor);
	DosWrite(fd, -1L, sizeof OinkMode, &OinkMode);
	DosWrite(fd, -1L, sizeof CurrentMusic, &CurrentMusic);
	DosWrite(fd, -1L, sizeof SpecialMusicPlaying, &SpecialMusicPlaying);
	DosWrite(fd, -1L, sizeof MusicResume, &MusicResume);
	DosWrite(fd, -1L, sizeof DeadPartyMembers, DeadPartyMembers);
	DosWrite(fd, -1L, sizeof DeadPartyCount, &DeadPartyCount);
	DosWrite(fd, -1L, sizeof ActiveBarge, &ActiveBarge);
	DosWrite(fd, -1L, sizeof EarthquakeCount, &EarthquakeCount);
	DosWrite(fd, -1L, sizeof ActiveMissiles, &ActiveMissiles);
	DosClose(fd);
}

void OstiaSaver::load(char *dir)
{
	int fd;

	fd = OpenFileOrFail(BuildPath(dir, U7OstiaFileName, 0));
	DosRead(fd, -1L, sizeof AvatarDontMove, &AvatarDontMove);
	DosRead(fd, -1L, sizeof CurrentVehicle, &CurrentVehicle);
	DosRead(fd, -1L, sizeof ArmageddonDone, &ArmageddonDone);
	DosRead(fd, -1L, sizeof ActiveSailor, &ActiveSailor);
	DosRead(fd, -1L, sizeof OinkMode, &OinkMode);
	DosRead(fd, -1L, sizeof CurrentMusic, &CurrentMusic);
	DosRead(fd, -1L, sizeof SpecialMusicPlaying, &SpecialMusicPlaying);
	DosRead(fd, -1L, sizeof MusicResume, &MusicResume);
	DosRead(fd, -1L, sizeof DeadPartyMembers, DeadPartyMembers);
	DosRead(fd, -1L, sizeof DeadPartyCount, &DeadPartyCount);
	DosRead(fd, -1L, sizeof ActiveBarge, &ActiveBarge);
	DosRead(fd, -1L, sizeof EarthquakeCount, &EarthquakeCount);
	DosRead(fd, -1L, sizeof ActiveMissiles, &ActiveMissiles);
	DosClose(fd);
}

/* 1 for animated types and class-14 types of height 1, 2 for NPC types, else 0. */
unsigned char far GetShapeFrameKind(unsigned type)
{
	TypeNumber s = type;

	if (IS_ANIMATED(s) || (IS_CLASS(s, TYPE_CLASS_BUILDING) && HEIGHT(s) == 1))
		return 1;
	if (IS_NPC(s))
		return 2;
	return 0;
}

/* The name of an item's type. */
char *far GetItemName(unsigned ref)
{
	return GetGameText(0, ItemType(ref));
}
