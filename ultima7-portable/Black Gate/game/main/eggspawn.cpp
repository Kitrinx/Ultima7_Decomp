/* Black Gate U7.EXE, overlay segment 229 (file offsets 0x06a5e0 to 0x06afa6, 2502 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
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

#define INFO(typeFrame) (gItemTypeInfo[(typeFrame).type()])
#define CURRENT(r) (GetNpcBufferForIbo(r)->schedules[GetNpcBufferForIbo(r)->currentSchedule])

struct ScriptBuffer { uint8_t length, data[127]; };

/* per monster category: attack modes to try, and the odds of stopping at each */
extern const uint8_t CategoryAttackModes[5][4] = {
	{ 0, 8, 7, 0 }, { 1, 0, 0, 0 }, { 0, 8, 0, 0 }, { 6, 5, 1, 2 }, { 3, 0, 0, 0 }
};
extern const uint8_t CategoryAttackOdds[5][4] = {
	{ 5, 2, 1, 0 }, { 1, 0, 0, 0 }, { 2, 1, 0, 0 }, { 2, 2, 2, 1 }, { 1, 0, 0, 0 }
};
int16_t SpawnGroupLeader = -1;

uint8_t SpawnMonster(int16_t source, TypeFrame &typeFrame, Coord &x, Coord &y, int16_t z,
	uint8_t alignment, int8_t workType, int16_t *count, int16_t)
{
	if ((uint8_t)INFO(typeFrame).strangeMovement && HasEvenCellPlacement(typeFrame.type())) {
		if ((int16_t)x & 1)
			++x;
		if ((int16_t)y & 1)
			++y;
	}
	int16_t type = MonsterLookup.get(typeFrame.type());
	uint8_t unrestricted = !(uint8_t)MonsterRecords.get(type)->fly &&
		!(uint8_t)MonsterRecords.get(type)->walk &&
		!(uint8_t)MonsterRecords.get(type)->swim &&
		!(uint8_t)MonsterRecords.get(type)->ethereal;
	uint8_t clear = 1;
	if (unrestricted)
		*count = 1;
	if (!unrestricted)
		if (!CanTypeMoveTo(CellCoord(x), CellCoord(y), z, TypeFrame(typeFrame.type()))
			&& (uint8_t)INFO(typeFrame).solid)
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
		int16_t i;
		for (i = 0; i < 3; i++) {
			if (GenerateRandomIntegerInRange(
				CategoryAttackOdds[(uint8_t)(MonsterRecords.get(type)->category - 1)][i]) == 0)
				break;
		}
		SetAttackModeByRef(&created, CategoryAttackModes[(uint8_t)(MonsterRecords.get(type)->category - 1)][i]);
	}
	return 1;
}

uint8_t SpawnObject(int16_t source, TypeFrame &typeFrame, Coord &x, Coord &y, int16_t z, uint8_t force)
{
	objref created;
	if ((uint8_t)INFO(typeFrame).solid && !force) {
		uint8_t clear = 1;
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
	if ((int8_t)IsActionQueueRoom() && ((ITEM(created.off)->typeFrame & 0x3ff) == 895 ||    /* fire field */
		(ITEM(created.off)->typeFrame & 0x3ff) == 900 ||    /* poison field */
		(ITEM(created.off)->typeFrame & 0x3ff) == 768)) /* energy field */
		ActionQueue.add(created.off, (char *)MakeScript(SCRIPT_NO_HALT, SCRIPT_SFX, 7, SCRIPT_END));
	return 1;
}

void SpawnFromEgg(EggRecord *spawn, objref source, Coord x, Coord y)
{
	int16_t count = LargerOf((uint8_t)(spawn->data1.bytes.first >> 2), 1);
	if (count > 1)
		count = GenerateRandomIntegerInRange(count) + 1;
	SpawnGroupLeader = -1;
	int16_t placed = 0;
	int16_t attempts = 100;
	uint8_t result;
	while (placed < count && attempts > 0) {
		if (count > 1) {
			x += GenerateRandomIntegerInRange(8) + GenerateRandomIntegerInRange(8) - 7;
			y += GenerateRandomIntegerInRange(8) + GenerateRandomIntegerInRange(8) - 7;
		}
		TypeFrame spawnType(spawn->data2.word);
		if (ItemTypeClassFlags[gItemTypeInfo[spawn->data2.word & 0x3ff].typeClass] & CLASS_NPC)
			result = SpawnMonster(source.off, spawnType, x, y, Item_getZ(&source),
				spawn->data1.bytes.first & 3, spawn->data1.bytes.second, &count, placed);
		else
			result = SpawnObject(source.off, spawnType, x, y, Item_getZ(&source), count == 1);
		if (result)
			placed++;
		else
			attempts--;
	}
}

extern "C" void ResetEggspawnGlobals(void)
{
	SpawnGroupLeader = -1;
}
