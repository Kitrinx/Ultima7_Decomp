/* Black Gate U7.EXE, overlay segment 316 (file offsets 0x091c50 to 0x092861, 3089 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "ucvalue.h"
#include "uclist.h"
#include "item.h"
#include "u7npc.h"
#include "coord.h"
#include "debug.h"
#include "partymov.h"
#include "bogus.h"
#include "damage.h"
#include "death.h"
#include "makemojo.h"
#include "npcref.h"
#include "palctrl.h"
#include "wihh.h"
#include "voolook.h"
#include "search.h"
#include "type.h"
#include "mapview.h"
#include "weapons.h"

struct WeaponRef {
	int16_t index;
	WeaponRef() {}
	void operator=(int16_t n) { index = n; }
	uint8_t isNull() { return index == 0; }
	uint8_t uses() { return WeaponRecords.get(index)->uses; }
};

/* how a weapon is used: 2 and 3 throw or fire it */

/* NPC statistics usecode reads and changes */
#define STAT_STRENGTH       0
#define STAT_DEXTERITY      1
#define STAT_INTELLIGENCE   2
#define STAT_HIT_POINTS     3
#define STAT_COMBAT         4
#define STAT_MANA           5
#define STAT_MAGIC          6
#define STAT_TRAINING       7
#define STAT_EXPERIENCE     8
#define STAT_FOOD           9
#define STAT_FEMALE         10
#define STAT_RANGED         11      /* holding a weapon that is thrown or fired */

/* the most combat or magic training can give */
#define STAT_TRAINED_MAX    30

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* the item in element 1 of the usecode value at v */
#define ITEM_ARG(v) GetItemRef(GetListNode(v, 1))

#define TYPE_CLASS(ref) (gItemTypeInfo[(ref).type()].typeClass)

inline uint8_t IsContainer(objref &ref) { return ItemTypeClassFlags[TYPE_CLASS(ref)] & CLASS_CONTENTS; }

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x20: one of an NPC's statistics */
void UC_GetNpcProp(Value *args, Value *ret)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	uint8_t property = ARG(args - 2, 1);
	int16_t value;

	switch (property) {
	case STAT_STRENGTH:
		value = GetNpcBufferForIbo(&ref)->strength;
		break;
	case STAT_DEXTERITY:
		value = GetNpcBufferForIbo(&ref)->dexterity;
		break;
	case STAT_INTELLIGENCE:
		value = GetNpcBufferForIbo(&ref)->intelligence;
		break;
	case STAT_HIT_POINTS:
		value = (int8_t)Item_getHitPoints(&ref);
		break;
	case STAT_COMBAT:
		value = GetNpcBufferForIbo(&ref)->combat;
		break;
	case STAT_MANA:
		value = GetNpcBufferForIbo(&ref)->mana;
		break;
	case STAT_MAGIC:
		value = GetNpcBufferForIbo(&ref)->magic;
		break;
	case STAT_TRAINING:
		value = GetNpcBufferForIbo(&ref)->training;
		break;
	case STAT_EXPERIENCE:
		value = GetNpcBufferForIbo(&ref)->experience;
		break;
	case STAT_FOOD:
		value = GetNpcBufferForIbo(&ref)->food;
		break;
	case STAT_FEMALE:
		value = !Npc_isMale(&ref);
		break;
	case STAT_RANGED:
		value = 0;
		WeaponRef weapon;
		weapon = WeaponLookup.get(objref(GetItemInSlot(ref, 1)).type());
		if (!weapon.isNull()) {
			if (weapon.uses() == USES_MISSILE || weapon.uses() == USES_GOOD_THROWN)
				value = 1;
		}
	}
	ret->appendInt(value);
}

/* 0x21: add to one of an NPC's statistics */
void UC_SetNpcProp(Value *args, Value *ret)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	uint8_t property = ARG(args - 2, 1);
	int16_t delta = ARG(args - 3, 1);
	int16_t value;

	switch (property) {
	case STAT_STRENGTH:
		GetNpcBufferForIbo(&ref)->strength = GetNpcBufferForIbo(&ref)->strength + delta;
		break;
	case STAT_DEXTERITY:
		GetNpcBufferForIbo(&ref)->dexterity = GetNpcBufferForIbo(&ref)->dexterity + delta;
		break;
	case STAT_INTELLIGENCE:
		GetNpcBufferForIbo(&ref)->intelligence = GetNpcBufferForIbo(&ref)->intelligence + delta;
		break;
	case STAT_HIT_POINTS:
		Item_setHitPoints(&ref, (int8_t)Item_getHitPoints(&ref) + delta);
		break;
	case STAT_COMBAT:
		value = GetNpcBufferForIbo(&ref)->combat + delta;
		if (value > STAT_TRAINED_MAX)
			value = STAT_TRAINED_MAX;
		GetNpcBufferForIbo(&ref)->combat = value;
		break;
	case STAT_MANA:
		GetNpcBufferForIbo(&ref)->mana = GetNpcBufferForIbo(&ref)->mana + delta;
		break;
	case STAT_MAGIC:
		value = GetNpcBufferForIbo(&ref)->magic + delta;
		if (value > STAT_TRAINED_MAX)
			value = STAT_TRAINED_MAX;
		GetNpcBufferForIbo(&ref)->magic = value;
		break;
	case STAT_TRAINING:
		GetNpcBufferForIbo(&ref)->training = GetNpcBufferForIbo(&ref)->training + delta;
		break;
	case STAT_FOOD:
		Npc_changeFood(&ref, (uint8_t)delta);
		break;
	case STAT_EXPERIENCE:
		AwardExperience(ref, (int32_t)delta);
	}
	ret->appendInt(1);
}

/* 0x28: how many items of a type an NPC holds, or the party for UC_PARTY */
void UC_CountObjects(Value *args, Value *ret)
{
	int16_t count = 0;
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	int16_t scope = ARG(args - 1, 1);
	int16_t type = ARG(args - 2, 1);
	int16_t quality = ARG(args - 3, 1);
	int16_t frame = ARG(args - 4, 1);

	if (type == UC_ALL)
		type = -1;
	if (quality == UC_ALL)
		quality = 255;
	if (frame == UC_ALL)
		frame = 255;
	int16_t party;
	if (scope == UC_PARTY)
		party = 1;
	count = CountHeldItems(party, ref, type, quality, frame);
	ret->appendInt(count);
}

/* 0x29: the first item of a type at a position, in view, in the party or in a container */
void UC_FindObject(Value *args, Value *ret)
{
	int16_t type = ARG(args - 2, 1);
	int16_t quality = ARG(args - 3, 1);
	int16_t frame = ARG(args - 4, 1);
	AreaSearch items;

	if (type == UC_ALL)
		type = -1;
	if (quality == UC_ALL)
		quality = 255;
	if (frame == UC_ALL)
		frame = 255;
	int16_t count = LinkList_count(args - 1);
	if (count == 3) {
		Coord x = ARG(args - 1, 1);
		Coord y = ARG(args - 1, 2);

		FindItemInArea(&items, x, y, 0, type, quality, frame);
		ret->appendInt(items.current.off);
		return;
	} else if (count != 1) {
		CheatPrintf("Incorrect number of params for FindItem");
		ret->appendInt(0);
		return;
	} else {
		int16_t object = ITEM_ARG(args - 1);

		if (object == UC_ALL) {
			FindItemInArea(&items, CellWindowX, CellWindowY,
				CellWindowX + (CELL_WINDOW - 1), CellWindowY + (CELL_WINDOW - 1),
				0, type, quality, frame);
			ret->appendInt(items.current.off);
			return;
		} else if (object == UC_PARTY) {
			for (int16_t i = 0; i < PartySize; i++) {
				FindItemInContainer(&items, PartyMembers[i], 0, type, quality, frame);
				if ((uint8_t)(items.current.off != 0)) {
					ret->appendInt(items.current.off);
					return;
				}
			}
			ret->appendInt(0);
			return;
		} else {
			objref container = object;

			if (!IsContainer(container) && !container.isNpc())
				CheatPrintf("Using FindItem to look inside something that is not an npc or container.");
			FindItemInContainer(&items, container, 0, type, quality, frame);
			ret->appendInt(items.current.off);
		}
	}
}

void UC_ReduceHealthPlain(Value *args)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	int8_t damage = ARG(args - 2, 1);

	ReduceHealth(ref, damage, 0, objref(0));
}

/* 0x71 */
void UC_ReduceHealth(Value *args)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	int8_t damage = ARG(args - 2, 1);

	ReduceHealth(ref, damage, ARG(args - 3, 1), objref(0));
}

/* 0x37 */
void UC_IsDead(Value *args, Value *ret)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;

	ret->appendInt((uint8_t)((GetNpcBufferForIbo(&ref)->status & NPC_DEAD) != 0));
}

/* 0x61 */
void UC_ApplyDamage(Value *args, Value *ret)
{
	int8_t damage = ARG(args - 1, 1);
	int16_t amount = ARG(args - 2, 1);
	int16_t mode = ARG(args - 3, 1);
	int16_t object = ITEM_ARG(args - 4);
	objref ref = object;

	ret->appendInt(DealDamage(damage, amount, mode, ref, objref(0)));
}

/* 0x72: whether an NPC holds a type and frame in a slot */
void UC_IsReadied(Value *args, Value *ret)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	uint16_t type = ARG(args - 3, 1);
	uint16_t frame = ARG(args - 4, 1);
	objref held = GetItemInSlot(ref, ARG(args - 2, 1));

	ret->appendInt(held.type() == type && (held.frame() == frame || frame == (uint16_t)UC_ALL));
}

/* 0x83 */
void UC_ResetPalette()
{
	SetFixedPalette(1);
}

/* 0x84 */
void UC_SetTimePalette()
{
	SetTimePalette();
}

/* 0x96: the article for a word */
void UC_AOrAn(Value *args, Value *ret)
{
	int8_t first = GetListNode(args - 1, 1)->text.str[0];

	switch (first) {
	case 'A': case 'E': case 'I': case 'O': case 'U':
	case 'a': case 'e': case 'i': case 'o': case 'u':
		ret->appendFarString("an");
		break;
	default:
		ret->appendFarString("a");
	}
}
