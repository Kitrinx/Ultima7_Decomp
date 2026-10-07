/* Serpent Isle SI.EXE, overlay segment 316 (file offsets 0x08ad50 to 0x08ba98, 3400 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
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

/* a statistic shares its byte with three flag bits: the stat in the low five, flags above */
#define STAT_BITS(n)        ((uint8_t)((n) & 0x1f))
#define FLAG_BITS(n)        ((uint8_t)((n) & 0xe0))
#define ADD_STAT(n, delta)  ((uint8_t)(((n) & 0x1f) + (delta)))

/* the most combat or magic training can give */
#define STAT_TRAINED_MAX    30

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* the item in element 1 of the usecode value at v */
#define ITEM_ARG(v) GetItemRef(GetListNode(v, 1))

#define NPC(p)  GetNpcBufferForIbo(p)

#define TYPE_CLASS(ref) (gItemTypeInfo[(ref).type()].typeClass)

inline uint8_t IsContainer(objref &ref) { return ItemTypeClassFlags[TYPE_CLASS(ref)] & CLASS_CONTENTS; }

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x28: one of an NPC's statistics, 0 for no item */
void UC_GetNpcProp(Value *args, Value *ret)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	uint8_t property = ARG(args - 2, 1);
	int16_t value;

	if (ref.valid()) {
		switch (property) {
		case STAT_STRENGTH:
			value = STAT_BITS(GetNpcBufferForIbo(&ref)->strength);
			break;
		case STAT_DEXTERITY:
			value = GetNpcBufferForIbo(&ref)->dexterity;
			break;
		case STAT_INTELLIGENCE:
			value = STAT_BITS(GetNpcBufferForIbo(&ref)->intelligence);
			break;
		case STAT_HIT_POINTS:
			value = (int8_t)Item_getHitPoints(&ref);
			break;
		case STAT_COMBAT:
			value = STAT_BITS(GetNpcBufferForIbo(&ref)->combat);
			break;
		case STAT_MANA:
			value = STAT_BITS(GetNpcBufferForIbo(&ref)->mana);
			break;
		case STAT_MAGIC:
			value = STAT_BITS(GetNpcBufferForIbo(&ref)->magic);
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
	} else
		value = 0;
	ret->appendInt(value);
}

/* 0x29: add to one of an NPC's statistics; only the avatar's mana and magic change */
void UC_SetNpcProp(Value *args, Value *ret)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	uint8_t property = ARG(args - 2, 1);
	int16_t delta = ARG(args - 3, 1);
	int16_t value;

	if (ref.valid()) {
		switch (property) {
		case STAT_STRENGTH:
			NPC(&ref)->strength = FLAG_BITS(NPC(&ref)->strength) | ADD_STAT(NPC(&ref)->strength, delta);
			break;
		case STAT_DEXTERITY:
			NPC(&ref)->dexterity = NPC(&ref)->dexterity + delta;
			break;
		case STAT_INTELLIGENCE:
			NPC(&ref)->intelligence = ADD_STAT(NPC(&ref)->intelligence, delta) | FLAG_BITS(NPC(&ref)->intelligence);
			break;
		case STAT_HIT_POINTS:
			Item_setHitPoints(&ref, (int8_t)Item_getHitPoints(&ref) + delta);
			break;
		case STAT_COMBAT:
			value = STAT_BITS(NPC(&ref)->combat) + delta;
			if (value > STAT_TRAINED_MAX)
				value = STAT_TRAINED_MAX;
			NPC(&ref)->combat = (uint8_t)value | FLAG_BITS(NPC(&ref)->combat);
			break;
		case STAT_MANA:
			if ((int8_t)Item_isAvatar(&ref))
				NPC(&ref)->mana = ADD_STAT(NPC(&ref)->mana, delta) | FLAG_BITS(NPC(&ref)->mana);
			break;
		case STAT_MAGIC:
			if ((int8_t)Item_isAvatar(&ref)) {
				value = STAT_BITS(NPC(&ref)->magic) + delta;
				if (value > STAT_TRAINED_MAX)
					value = STAT_TRAINED_MAX;
				NPC(&ref)->magic = (uint8_t)value | FLAG_BITS(NPC(&ref)->magic);
			}
			break;
		case STAT_TRAINING:
			NPC(&ref)->training = NPC(&ref)->training + delta;
			break;
		case STAT_FOOD:
			Npc_changeFood(&ref, (uint8_t)delta);
			break;
		case STAT_FEMALE:
			if (delta)
				Npc_setFemale(&ref);
			else
				Npc_setMale(&ref);
			break;
		case STAT_EXPERIENCE:
			AwardExperience(ref, (int32_t)delta);
		}
	}
	ret->appendInt(1);
}

/* 0x31: how many items of a type an NPC holds, or the party for UC_PARTY */
void UC_CountObjects(Value *args, Value *ret)
{
	int16_t count = 0;
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	int16_t scope = ARG(args - 1, 1);
	int16_t type = ARG(args - 2, 1);
	int16_t quality = ARG(args - 3, 1);
	int16_t frame = ARG(args - 4, 1);
	int16_t party = 0;

	if (type == UC_ALL)
		type = -1;
	if (quality == UC_ALL)
		quality = 255;
	if (frame == UC_ALL)
		frame = 255;
	if (scope == UC_PARTY)
		party = 1;
	count = CountHeldItems(party, ref, type, quality, frame);
	ret->appendInt(count);
}

/* 0x32: the first item of a type at a position, in view, in the party or in a container */
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

/* 0x87 */
void UC_ReduceHealth(Value *args)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;
	int8_t damage = ARG(args - 2, 1);

	ReduceHealth(ref, damage, ARG(args - 3, 1), objref(0));
}

/* 0x44: whether an NPC is dead; no item counts as dead */
void UC_IsDead(Value *args, Value *ret)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;

	if (ref.valid())
		ret->appendInt((uint8_t)((GetNpcBufferForIbo(&ref)->status & NPC_DEAD) != 0));
	else
		ret->appendInt(1);
}

/* 0x76 */
void UC_ApplyDamage(Value *args, Value *ret)
{
	int8_t damage = ARG(args - 1, 1);
	int16_t amount = ARG(args - 2, 1);
	int16_t mode = ARG(args - 3, 1);
	int16_t object = ITEM_ARG(args - 4);
	objref ref = object;

	ret->appendInt(DealDamage(damage, amount, mode, ref, objref(0)));
}

/* 0x88: whether an NPC holds a type and frame in a slot; a 0 always follows the answer */
void UC_IsReadied(Value *args, Value *ret)
{
	int16_t object = ITEM_ARG(args - 1);
	objref ref = object;

	if (ref.valid()) {
		uint16_t type = ARG(args - 3, 1);
		uint16_t frame = ARG(args - 4, 1);
		objref held = GetItemInSlot(ref, ARG(args - 2, 1));

		ret->appendInt(held.type() == type && (held.frame() == frame || frame == (uint16_t)UC_ALL));
	}
	ret->appendInt(0);
}

/* 0x9d */
void UC_ResetPalette()
{
	SetFixedPalette(1);
}

/* 0x9e */
void UC_SetTimePalette()
{
	SetTimePalette();
}

/* 0xb2: the article for a word */
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
