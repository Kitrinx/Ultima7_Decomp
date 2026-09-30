/* Black Gate U7.EXE, resident segment 90 (file offsets 0x030eb8 to 0x032208, 4944 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "typefram.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "debug.h"
#include "itembuf.h"
#include "makemojo.h"
#include "combat.h"
#include "monsters.h"
#include "type.h"
#include "voolook.h"
#include "equip.h"
#include "voonpc.h"
#include "npcref.h"
#include <new>

#define ITEM_TYPE(i) ((i)->typeFrame & 0x3ff)
#define TYPE_CLASS(i) (gItemTypeInfo[ITEM_TYPE(i)].typeClass)
#define IS_NPC(i) ((uint8_t)((ItemTypeClassFlags[TYPE_CLASS(i)] & CLASS_NPC) != 0))
#define IS_CLASS(i, n) ((uint8_t)(TYPE_CLASS(i) == (n)))
#define NPC_FLAG(n, mask) ((uint8_t)(((n)->status & (mask)) != 0))
#define NPC_EXTRA(off) ((NpcExtra *)ItemAt(off))

/* the class flags of TYPE_CLASS_HUMAN */
#define HUMAN_CLASS_FLAGS 0x3ee

/* set in scheduleFlags when the NPC's target is a place, not an item */
#define TARGET_IS_PLACE 0x200

/* in typeFlagsHigh: might, which doubles a statistic and lifts a weak one to 10 */
#define NPC_MIGHT       0x10

/* an NPC's extra record: its number sits where other items keep flags */
struct NpcExtra {
	int16_t contents;
	uint8_t region, quality;
	uint8_t npcNumber;
};

extern int16_t NpcItemRefs[];

const int16_t FacingFrameOffsets[] = { 0, 48, 16, 32 };
objref AvatarRef;
int16_t FreeNpcNumbers = -1;
int16_t FreeMonsterNumbers = -1;
NpcBufferPool NpcPool;
NpcBuffer NullNpcBuffer = { 0 };

/* one 7-byte record of the NPC pose table */
void StoreNpcViewRecord(int32_t *address, int16_t index, NpcView *record)
{
	if (index >= 0 && index < NpcRecordCount + ExtraNpcRecordCount)
		CopyFarToLinear(*address + (uint16_t)(index * 7), record, INT32_C(7));
}

NpcBuffer * GetNpcBufferForIbo(objref *ref)
{
	if (ref->valid() && IS_NPC(ITEM(ref->off)))
		return GetCachedNpcBuffer(&NpcPool, Item_getNpcNumber(ref));
	return &NullNpcBuffer;
}

void Npc_allocateNumber(objref *ref)
{
	int16_t number = FreeNpcNumbers;

	if (number != -1) {
		FreeNpcNumbers = GetCachedNpcBuffer(&NpcPool, number)->nextFree;
		Npc_setNumber(ref, number);
	}
}

void Npc_freeNumber(objref *ref)
{
	int16_t number = Item_getNpcNumber(ref);

	NpcItemRefs[number] = 0;
	GetCachedNpcBuffer(&NpcPool, number)->nextFree = FreeNpcNumbers;
	FreeNpcNumbers = number;
}

void Npc_allocateMonsterNumber(objref *ref)
{
	int16_t number = FreeMonsterNumbers;

	if (number != -1) {
		FreeMonsterNumbers = GetCachedNpcBuffer(&NpcPool, number)->nextFree;
		Npc_setNumber(ref, number);
	}
}

void Npc_freeMonsterNumber(objref *ref)
{
	int16_t number = Item_getNpcNumber(ref);

	NpcItemRefs[number] = 0;
	GetCachedNpcBuffer(&NpcPool, number)->nextFree = FreeMonsterNumbers;
	FreeMonsterNumbers = number;
}

uint8_t Npc_getStrength(objref *ref)
{
	int16_t value = GetNpcBufferForIbo(ref)->strength;

	if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_CURSED))
		value -= 3;
	if (GetNpcBufferForIbo(ref)->marked(NPC_MIGHT)) {
		if (value < 5)
			value = 10;
		else
			value *= 2;
	}
	if (value < 1)
		value = 1;
	if (value > 30 && GetNpcBufferForIbo(ref)->strength <= 30)
		value = 30;
	return value;
}

uint8_t Npc_getDexterity(objref *ref)
{
	int16_t value;

	value = GetNpcBufferForIbo(ref)->dexterity;
	if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_PARALYZED) || NPC_FLAG(GetNpcBufferForIbo(ref), NPC_DEAD)
		|| NPC_FLAG(GetNpcBufferForIbo(ref), NPC_ASLEEP) || (int8_t)Item_getHitPoints(ref) <= 0) {
		value = 0;
	} else {
		if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_CURSED))
			value -= 3;
		if (GetNpcBufferForIbo(ref)->marked(NPC_MIGHT)) {
			if (value < 5)
				value = 10;
			else
				value *= 2;
		}
	}
	if (value < 0)
		value = 0;
	if (value > 30)
		value = 30;
	return value;
}

uint8_t Npc_getIntelligence(objref *ref)
{
	int16_t value;

	value = GetNpcBufferForIbo(ref)->intelligence;
	if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_DEAD)) {
		value = 1;
	} else {
		if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_CURSED))
			value -= 3;
		if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_CHARMED))
			value--;
		if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_ASLEEP))
			value--;
		if (GetNpcBufferForIbo(ref)->marked(NPC_MIGHT)) {
			if (value < 5)
				value = 10;
			else
				value *= 2;
		}
	}
	if (value < 1)
		value = 1;
	if (value > 30)
		value = 30;
	return value;
}

uint8_t Npc_getCombat(objref *ref)
{
	int16_t value;

	value = GetNpcBufferForIbo(ref)->combat;
	if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_PARALYZED) || NPC_FLAG(GetNpcBufferForIbo(ref), NPC_DEAD)
		|| NPC_FLAG(GetNpcBufferForIbo(ref), NPC_ASLEEP) || (int8_t)Item_getHitPoints(ref) <= 0) {
		value = 1;
	} else {
		if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_CURSED))
			value -= 3;
		if (GetNpcBufferForIbo(ref)->marked(NPC_MIGHT)) {
			if (value < 5)
				value = 10;
			else
				value *= 2;
		}
	}
	if (value < 1)
		value = 1;
	if (value > 30 && GetNpcBufferForIbo(ref)->combat <= 30)
		value = 30;
	return value;
}

int16_t Npc_getLevel(objref *ref)
{
	int16_t experience;
	int16_t level = 1;

	for (experience = GetNpcBufferForIbo(ref)->experience / INT32_C(100);
		experience != 0; experience = experience >> 1)
		level++;
	return level;
}

int16_t Item_isAvatar(objref *ref)
{
	if (ref->off == AvatarRef.off)
		return 1;
	return 0;
}

/* Creates an NPC at x, y, z: a human takes a numbered NPC slot, anything else a monster slot. Its
 * equipment slots start empty and it moves as its MONSTERS.DAT record says. */
uint8_t CreateNpc(objref *ref, TypeFrame &shape, CellCoord x, CellCoord y, int16_t z)
{
	int16_t classFlags;
	int16_t monsterIndex = MonsterLookup.get(shape.type());
	int16_t slots[12];

	for (int16_t i = 0; i < 12; i++)
		slots[i] = 0;
	classFlags = ItemTypeClassFlags[gItemTypeInfo[shape.bits & 0x3ff].typeClass];
	if (classFlags == HUMAN_CLASS_FLAGS) {
		if (FreeNpcNumbers != -1) {
			if (CreateItem(ref, TypeFrame(shape.bits), x, y, z)) {
				Npc_allocateNumber(ref);
				GetNpcBufferForIbo(ref)->status = 0;
				GetNpcBufferForIbo(ref)->typeFlagsHigh = 0;
				GetNpcBufferForIbo(ref)->setType(shape);
				CopyFarToLinear(EquipList + Item_getNpcNumber(ref) * (int32_t)sizeof slots, slots, (int32_t)sizeof slots);
				if ((uint8_t)MonsterRecords.get(monsterIndex)->walk) {
					GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_WALK;
				}
				if ((uint8_t)MonsterRecords.get(monsterIndex)->swim) {
					GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_SWIM;
				}
				if ((uint8_t)MonsterRecords.get(monsterIndex)->fly) {
					GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_FLY;
				}
				if ((uint8_t)MonsterRecords.get(monsterIndex)->ethereal) {
					if ((uint8_t)gItemTypeInfo[ITEM_TYPE(ITEM(ref->off))].solid) {
						DebugPrintfWait("\nYO! NPC %d is ethereal but solid! Fix it in the WE!\n",
							Item_getNpcNumber(ref));
						GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_FLY;
					} else {
						GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_ETHEREAL;
					}
				}
				GetNpcBufferForIbo(ref)->typeFlagsHigh = GetNpcBufferForIbo(ref)->typeFlagsHigh & ~0x40;
				GetNpcBufferForIbo(ref)->typeFlagsHigh = GetNpcBufferForIbo(ref)->typeFlagsHigh & ~0x80;
				GetNpcBufferForIbo(ref)->typeFlagsHigh = GetNpcBufferForIbo(ref)->typeFlagsHigh & ~4;
				Item_setQualityFlags(ref, 0);
				CombatGroups.movePoints[Item_getNpcNumber(ref)] = 0;
				return 1;
			}
		}
	} else {
		if (FreeMonsterNumbers != -1) {
			if (CreateItem(ref, TypeFrame(shape.bits), x, y, z)) {
				Npc_allocateMonsterNumber(ref);
				GetNpcBufferForIbo(ref)->status = 0;
				GetNpcBufferForIbo(ref)->typeFlagsHigh = 0;
				GetNpcBufferForIbo(ref)->setType(shape);
				CopyFarToLinear(EquipList + Item_getNpcNumber(ref) * (int32_t)sizeof slots, slots, (int32_t)sizeof slots);
				if ((uint8_t)MonsterRecords.get(monsterIndex)->walk) {
					GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_WALK;
				}
				if ((uint8_t)MonsterRecords.get(monsterIndex)->swim) {
					GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_SWIM;
				}
				if ((uint8_t)MonsterRecords.get(monsterIndex)->fly) {
					GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_FLY;
				}
				if ((uint8_t)MonsterRecords.get(monsterIndex)->ethereal) {
					GetNpcBufferForIbo(ref)->typeFlags = GetNpcBufferForIbo(ref)->typeFlags | NPC_ETHEREAL;
				}
				GetNpcBufferForIbo(ref)->typeFlagsHigh = GetNpcBufferForIbo(ref)->typeFlagsHigh & ~0x40;
				GetNpcBufferForIbo(ref)->typeFlagsHigh = GetNpcBufferForIbo(ref)->typeFlagsHigh & ~0x80;
				GetNpcBufferForIbo(ref)->typeFlagsHigh = GetNpcBufferForIbo(ref)->typeFlagsHigh & ~4;
				Item_setQualityFlags(ref, 0);
				CombatGroups.movePoints[Item_getNpcNumber(ref)] = 0;
				return 1;
			}
		}
	}
	return 0;
}

void Item_moveOffMap(objref *ref)
{
	if (Item_detach(ref) != 0)
		PlaceItemOffMap(ref);
}

void GetNpcIbo(objref *ref, int16_t number)
{
	if ((uint16_t)(NpcRecordCount + ExtraNpcRecordCount) > (uint16_t)number)
		ref->off = NpcItemRefs[number];
	else
		ref->off = 0;
}

int16_t Item_getNpcNumber(objref *ref)
{
	int16_t extra;

	if (IS_NPC(ITEM(ref->off))) {
		extra = ITEM(ref->off)->data.extra;
		if (IS_CLASS(ITEM(ref->off), TYPE_CLASS_MONSTER))
			return NPC_EXTRA(extra)->npcNumber + NpcRecordCount;
		else
			return NPC_EXTRA(extra)->npcNumber;
	}
	return -1;
}

void Npc_setNumber(objref *ref, uint16_t number)
{
	int16_t extra;

	if (IS_NPC(ITEM(ref->off)) && number < (uint16_t)(NpcRecordCount + ExtraNpcRecordCount)) {
		extra = ITEM(ref->off)->data.extra;
		if (IS_CLASS(ITEM(ref->off), TYPE_CLASS_MONSTER))
			NPC_EXTRA(extra)->npcNumber = number - NpcRecordCount;
		else
			NPC_EXTRA(extra)->npcNumber = number;
		NpcItemRefs[number] = ref->off;
	}
}

/* An NPC's target is an item, or with TARGET_IS_PLACE a place: x and y offsets from the
 * avatar in schedulePosition's two bytes and z in scheduleFlags' top nibble. */
uint8_t Npc_hasItemTarget(objref *ref)
{
	return !((GetNpcBufferForIbo(ref)->scheduleFlags & TARGET_IS_PLACE) >> 9);
}

int16_t Npc_getItemTarget(objref *ref)
{
	if (GetNpcBufferForIbo(ref)->scheduleFlags & TARGET_IS_PLACE)
		return 0;
	return GetNpcBufferForIbo(ref)->schedulePosition;
}

void Npc_getTargetCoords(objref *ref, int16_t *x, int16_t *y, int16_t *z)
{
	int16_t d;

	if (GetNpcBufferForIbo(ref)->scheduleFlags & TARGET_IS_PLACE) {
		d = GetNpcBufferForIbo(ref)->schedulePosition & 0xff;
		if (d > 127)
			d |= 0xff00;
		*x = (int16_t)Item_getX(AvatarRef) + d;
		d = (GetNpcBufferForIbo(ref)->schedulePosition & 0xff00) >> 8;
		if (d > 127)
			d |= 0xff00;
		*y = (int16_t)Item_getY(AvatarRef) + d;
		d = GetNpcBufferForIbo(ref)->scheduleFlags >> 12 & 0xf;
		*z = d;
	} else {
		objref target = GetNpcBufferForIbo(ref)->schedulePosition;

		*x = Item_getX(target);
		*y = Item_getY(target);
		*z = GetItemZAndStuff(&target).z();
	}
}

int16_t Npc_getTargetWeapon(objref *ref)
{
	int16_t value = GetNpcBufferForIbo(ref)->scheduleFlags & 0x1ff;

	if (value & 0x100)
		value |= 0xfe00;
	return value;
}

void Npc_setItemTarget(objref *ref, int16_t target)
{
	GetNpcBufferForIbo(ref)->schedulePosition = target;
	GetNpcBufferForIbo(ref)->scheduleFlags &= ~TARGET_IS_PLACE;
}

void Npc_setCoordTarget(objref *ref, Coord x, Coord y, int16_t z)
{
	int16_t dx = x.value - Item_getX(AvatarRef);
	int16_t dy = y.value - Item_getY(AvatarRef);

	GetNpcBufferForIbo(ref)->schedulePosition = (dx & 0xff) | (dy << 8 & 0xff00);
	GetNpcBufferForIbo(ref)->scheduleFlags = (GetNpcBufferForIbo(ref)->scheduleFlags & 0x1ff) |
		(z << 12 & 0xf000) | TARGET_IS_PLACE;
}

void Npc_setTargetWeapon(objref *ref, uint16_t value)
{
	GetNpcBufferForIbo(ref)->scheduleFlags = (GetNpcBufferForIbo(ref)->scheduleFlags & 0xfe00) | (value & 0x1ff);
}

void Npc_setMale(objref *ref)
{
	GetNpcBufferForIbo(ref)->typeFlagsHigh |= 2;
}

void Npc_setFemale(objref *ref)
{
	uint8_t flags = GetNpcBufferForIbo(ref)->typeFlagsHigh;

	GetNpcBufferForIbo(ref)->typeFlagsHigh = flags & ~2;
}

uint8_t Npc_isMale(objref *ref)
{
	return GetNpcBufferForIbo(ref)->typeFlagsHigh & 2;
}

uint8_t IsNpcUnconscious(objref *ref)
{
	if (NPC_FLAG(GetNpcBufferForIbo(ref), NPC_PARALYZED)
		|| NPC_FLAG(GetNpcBufferForIbo(ref), NPC_ASLEEP)
		|| (uint8_t)((int8_t)Item_getHitPoints(ref) <= 0)
		|| NPC_FLAG(GetNpcBufferForIbo(ref), NPC_DEAD))
		return 1;
	return 0;
}

void Npc_changeFood(objref *ref, int8_t amount)
{
	int16_t food = GetNpcBufferForIbo(ref)->food + amount;

	if (food >= 31)
		GetNpcBufferForIbo(ref)->food = 31;
	else if (food < 0)
		GetNpcBufferForIbo(ref)->food = 0;
	else
		GetNpcBufferForIbo(ref)->food = food;
}

extern "C" void ResetNpcrefGlobals(void)
{
	AvatarRef = 0;
	FreeNpcNumbers = -1;
	FreeMonsterNumbers = -1;
	memset(&NullNpcBuffer, 0, sizeof NullNpcBuffer);
}

extern "C" void ConstructNpcrefGlobals(void)
{
	new (&NpcPool) NpcBufferPool();
}
