/* Black Gate U7.EXE, resident segment 59 (file offsets 0x022730 to 0x022e85, 1877 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include <stdio.h>
#include <string.h>
#include "plat.h"
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
	uint16_t id;
	TypeNumber(uint16_t type) { id = type; }
};

#define INFO(s) (gItemTypeInfo[(s).id & 0x3ff])
#define TYPE_CLASS(s) (INFO(s).typeClass)
#define CLASS_FLAGS(s) (ItemTypeClassFlags[TYPE_CLASS(s)])
#define IS_ANIMATED(s) ((int8_t) INFO(s).animated)
#define HEIGHT(s) (INFO(s).height)
#define IS_CLASS(s, c) ((int8_t) (TYPE_CLASS(s) == (c)))
#define IS_NPC(s) ((int8_t) ((CLASS_FLAGS(s) & CLASS_NPC) != 0))

int16_t DeadPartyMembers[12];
ItemNodesSaver ItemNodesFile;
ItemBufferSaver ItemBufferFile;
OstiaSaver OstiaFile;
char *ItemNodeFileName = "ITEMNODE.DAT";
char *U7IBufFileName = "U7IBUF.DAT";
char *U7OstiaFileName = "U7_OSTIA.DAT";
uint8_t AvatarDontMove = 1;
uint8_t ArmageddonDone = 0;
int16_t ActiveSailor = 0;
int16_t CurrentVehicle = 0;
int16_t ActiveBarge = 0;
int16_t DeadPartyCount = 0;
uint8_t OinkMode = 0;

/* One bit per region: its item file has a newer copy waiting under .$$$. */
inline uint8_t IsRegionPending(uint8_t n)
{
	return SavedRegionBits[n / 8] & (1 << n % 8);
}

inline void ClearRegionPending(uint8_t n)
{
	SavedRegionBits[n / 8] &= ~(1 << n % 8);
}

/* An item's type: the low 10 bits of its typeFrame. */
inline uint16_t ItemType(objref r)
{
	return ITEM(r.off)->typeFrame & 0x3ff;
}

char *ItemNodesSaver::name()
{
	return ItemNodeFileName;
}

void ItemNodesSaver::save(char *dir)
{
	int16_t fd;
	int16_t i;
	uint8_t region[2];
	char name[80];
	char tmp[80];

	fd = CreateFileOrFail(BuildPath(dir, ItemNodeFileName, 0));
	DosWrite(fd, -INT32_C(1), sizeof ItemFreeList, &ItemFreeList);
	DosWrite(fd, -INT32_C(1), sizeof ItemFreeCount, &ItemFreeCount);
	DosWrite(fd, -INT32_C(1), sizeof DetachedItems.off, &DetachedItems.off);
	DosWrite(fd, -INT32_C(1), sizeof ChunkItemLists, ChunkItemLists);
	DosWrite(fd, -INT32_C(1), sizeof LoadedRegions, LoadedRegions);
	DosWrite(fd, -INT32_C(1), sizeof CurrentRegion, &CurrentRegion);
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
			plat_file_remove(name);
			plat_file_rename(tmp, name);
			ClearRegionPending(i);
		}
	}
}

void ItemNodesSaver::load(char *dir)
{
	int16_t fd;
	int16_t i;

	fd = OpenFileOrFail(BuildPath(dir, ItemNodeFileName, 0));
	DosRead(fd, -INT32_C(1), sizeof ItemFreeList, &ItemFreeList);
	DosRead(fd, -INT32_C(1), sizeof ItemFreeCount, &ItemFreeCount);
	DosRead(fd, -INT32_C(1), sizeof DetachedItems.off, &DetachedItems.off);
	DosRead(fd, -INT32_C(1), sizeof ChunkItemLists, ChunkItemLists);
	DosRead(fd, -INT32_C(1), sizeof LoadedRegions, LoadedRegions);
	DosRead(fd, -INT32_C(1), sizeof CurrentRegion, &CurrentRegion);
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
	int16_t fd;

	fd = CreateFileOrFail(BuildPath(dir, U7IBufFileName, 0));
	DosWrite(fd, -INT32_C(1), ItemBufferBytes, ItemBuffer);
	DosClose(fd);
}

void ItemBufferSaver::load(char *dir)
{
	int16_t fd;

	fd = OpenFileOrFail(BuildPath(dir, U7IBufFileName, 0));
	DosRead(fd, -INT32_C(1), ItemBufferBytes, ItemBuffer);
	DosClose(fd);
}

char *OstiaSaver::name()
{
	return U7OstiaFileName;
}

void OstiaSaver::save(char *dir)
{
	int16_t fd;

	fd = CreateFileOrFail(BuildPath(dir, U7OstiaFileName, 0));
	DosWrite(fd, -INT32_C(1), sizeof AvatarDontMove, &AvatarDontMove);
	DosWrite(fd, -INT32_C(1), sizeof CurrentVehicle, &CurrentVehicle);
	DosWrite(fd, -INT32_C(1), sizeof ArmageddonDone, &ArmageddonDone);
	DosWrite(fd, -INT32_C(1), sizeof ActiveSailor, &ActiveSailor);
	DosWrite(fd, -INT32_C(1), sizeof OinkMode, &OinkMode);
	DosWrite(fd, -INT32_C(1), sizeof CurrentMusic, &CurrentMusic);
	DosWrite(fd, -INT32_C(1), sizeof SpecialMusicPlaying, &SpecialMusicPlaying);
	DosWrite(fd, -INT32_C(1), sizeof MusicResume, &MusicResume);
	DosWrite(fd, -INT32_C(1), sizeof DeadPartyMembers, DeadPartyMembers);
	DosWrite(fd, -INT32_C(1), sizeof DeadPartyCount, &DeadPartyCount);
	DosWrite(fd, -INT32_C(1), sizeof ActiveBarge, &ActiveBarge);
	DosWrite(fd, -INT32_C(1), sizeof EarthquakeCount, &EarthquakeCount);
	DosWrite(fd, -INT32_C(1), sizeof ActiveMissiles, &ActiveMissiles);
	DosClose(fd);
}

void OstiaSaver::load(char *dir)
{
	int16_t fd;

	fd = OpenFileOrFail(BuildPath(dir, U7OstiaFileName, 0));
	DosRead(fd, -INT32_C(1), sizeof AvatarDontMove, &AvatarDontMove);
	DosRead(fd, -INT32_C(1), sizeof CurrentVehicle, &CurrentVehicle);
	DosRead(fd, -INT32_C(1), sizeof ArmageddonDone, &ArmageddonDone);
	DosRead(fd, -INT32_C(1), sizeof ActiveSailor, &ActiveSailor);
	DosRead(fd, -INT32_C(1), sizeof OinkMode, &OinkMode);
	DosRead(fd, -INT32_C(1), sizeof CurrentMusic, &CurrentMusic);
	DosRead(fd, -INT32_C(1), sizeof SpecialMusicPlaying, &SpecialMusicPlaying);
	DosRead(fd, -INT32_C(1), sizeof MusicResume, &MusicResume);
	DosRead(fd, -INT32_C(1), sizeof DeadPartyMembers, DeadPartyMembers);
	DosRead(fd, -INT32_C(1), sizeof DeadPartyCount, &DeadPartyCount);
	DosRead(fd, -INT32_C(1), sizeof ActiveBarge, &ActiveBarge);
	DosRead(fd, -INT32_C(1), sizeof EarthquakeCount, &EarthquakeCount);
	DosRead(fd, -INT32_C(1), sizeof ActiveMissiles, &ActiveMissiles);
	DosClose(fd);
}

/* 1 for animated types and class-14 types of height 1, 2 for NPC types, else 0. */
uint8_t GetShapeFrameKind(uint16_t type)
{
	TypeNumber s = type;

	if (IS_ANIMATED(s) || (IS_CLASS(s, TYPE_CLASS_BUILDING) && HEIGHT(s) == 1))
		return 1;
	if (IS_NPC(s))
		return 2;
	return 0;
}

/* The name of an item's type. */
char * GetItemName(uint16_t ref)
{
	return GetGameText(0, ItemType(ref));
}
