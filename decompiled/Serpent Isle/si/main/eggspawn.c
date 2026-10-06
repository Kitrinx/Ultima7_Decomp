/* Serpent Isle SI.EXE, overlay segment 218 (file offsets 0x05be20 to 0x05c7f2, 2514 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: eggspawn.c */
#include "lowlevel.h"
#include "activity.h"
#include "dosio.h"
#include "iteminfo.h"
#include "item.h"
#include "voolook.h"
#include "u7npc.h"
#include "combmode.h"
#include "combpick.h"
#include "legalmov.h"
#include "missile.h"
#include "equip.h"
#include "slime.h"
#include "random.h"
#include "makemojo.h"
#include "monsters.h"
#include "script.h"
#include "type.h"
#include "coord.h"
#include "actqueue.h"
#include "trigger.h"

#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))
#define INFO(typeFrame) (gItemTypeInfo[(typeFrame).type()])
#define CURRENT(r) (GetNpcBufferForIbo(r)->schedules[GetNpcBufferForIbo(r)->currentSchedule])

struct ScriptBuffer { unsigned char length, data[127]; };

/* per monster category: attack modes to try, and the odds of stopping at each */
unsigned char CategoryAttackModes[5][4] = {
	{ 0, 8, 7, 0 }, { 1, 0, 0, 0 }, { 0, 8, 0, 0 }, { 6, 5, 1, 2 }, { 3, 0, 0, 0 }
};
unsigned char CategoryAttackOdds[5][4] = {
	{ 5, 2, 1, 0 }, { 1, 0, 0, 0 }, { 2, 1, 0, 0 }, { 2, 2, 2, 1 }, { 1, 0, 0, 0 }
};
int SpawnGroupLeader = -1;

unsigned char far SpawnMonster(int source, TypeFrame far &typeFrame, Coord &x, Coord &y, int z,
	unsigned char alignment, char workType, int *count, int)
{
	if ((unsigned char)INFO(typeFrame).strangeMovement && HasEvenCellPlacement(typeFrame.type())) {
		if ((int)x & 1)
			++x;
		if ((int)y & 1)
			++y;
	}
	int type = MonsterLookup.get(typeFrame.type());
	unsigned char unrestricted = !(unsigned char)MonsterRecords.get(type)->fly &&
		!(unsigned char)MonsterRecords.get(type)->walk &&
		!(unsigned char)MonsterRecords.get(type)->swim &&
		!(unsigned char)MonsterRecords.get(type)->ethereal;
	unsigned char clear = 1;
	if (unrestricted)
		*count = 1;
	if (!unrestricted)
		if (!CanTypeMoveTo(CellCoord(x), CellCoord(y), z, TypeFrame(typeFrame.type()))
			&& (unsigned char)INFO(typeFrame).solid)
			clear = 0;
	if (source)
		if (!HasLineOfFireToCoords(objref(source), x, y, z + INFO(typeFrame).height - 1))
			clear = 0;
	if (!clear)
		return 0;
	objref created;
	CreateNpc(&created, TypeFrame(typeFrame.bits), x, y, z);
	if (!created.valid())
		return 0;
	RandomizeCreature(created, TypeFrame(typeFrame.bits));
	if (alignment == 0)
		alignment = MonsterRecords.get(type)->alignment;
	GetNpcBufferForIbo(&created)->setAlignment(alignment);
	GetNpcBufferForIbo(&created)->setInitialAlignment(alignment);
	Npc_setSchedule(&created, workType);
	CURRENT(&created).state = 1;
	ChooseNearbyTarget(&created);
	if (workType == WORK_COMBAT) {
		int i;
		for (i = 0; i < 3; i++) {
			if (GenerateRandomIntegerInRange(
				CategoryAttackOdds[(unsigned char)(MonsterRecords.get(type)->category - 1)][i]) == 0)
				break;
		}
		SetAttackModeByRef(&created, CategoryAttackModes[(unsigned char)(MonsterRecords.get(type)->category - 1)][i]);
	}
	return 1;
}

unsigned char far SpawnObject(int source, TypeFrame far &typeFrame, Coord &x, Coord &y, int z, unsigned char force)
{
	objref created;
	if ((unsigned char)INFO(typeFrame).solid && !force) {
		unsigned char clear = 1;
		if (!CanTypeMoveTo(CellCoord(x), CellCoord(y), z, TypeFrame(typeFrame.bits)))
			clear = 0;
		if (source) {
			if (!HasLineOfFireToCoords(objref(source), x, y, z + INFO(typeFrame).height))
				clear = 0;
		}
		if (!clear)
			return 0;
	}
	CreateItem(&created, TypeFrame(typeFrame.bits), x, y, z);
	if (!created.valid())
		return 0;
	Item_setTemporary(&created);
	Item_setOkayToTake(&created);
	Item_storeQuantity(&created, 1);
	if ((char)IsActionQueueRoom() && ((ITEM(created.off)->typeFrame & 0x3ff) == 895 ||    /* fire field */
		(ITEM(created.off)->typeFrame & 0x3ff) == 561 ||
		(ITEM(created.off)->typeFrame & 0x3ff) == 900 ||    /* poison field */
		(ITEM(created.off)->typeFrame & 0x3ff) == 768)) /* energy field */
		ActionQueue.add(created.off, (char *)MakeScript(SCRIPT_NO_HALT, SCRIPT_SFX, 40, SCRIPT_END));
	return 1;
}

void far SpawnFromEgg(EggRecord far *spawn, objref source, Coord x, Coord y)
{
	int count = LargerOf((unsigned char)(spawn->data1.bytes.first >> 2), 1);
	if (count > 1)
		count = GenerateRandomIntegerInRange(count) + 1;
	SpawnGroupLeader = -1;
	int placed = 0;
	int attempts = 100;
	unsigned char result;
	while (placed < count && attempts > 0) {
		if (count > 1) {
			x += GenerateRandomIntegerInRange(8) + GenerateRandomIntegerInRange(8) - 7;
			y += GenerateRandomIntegerInRange(8) + GenerateRandomIntegerInRange(8) - 7;
		}
		if (ItemTypeClassFlags[gItemTypeInfo[spawn->data2.word & 0x3ff].typeClass] & CLASS_NPC)
			result = SpawnMonster(source.off, TypeFrame(spawn->data2.word), x, y, Item_getZ(&source),
				spawn->data1.bytes.first & 3, spawn->data1.bytes.second, &count, placed);
		else
			result = SpawnObject(source.off, TypeFrame(spawn->data2.word), x, y, Item_getZ(&source), count == 1);
		if (result)
			placed++;
		else
			attempts--;
	}
}
