/* Serpent Isle SI.EXE, overlay segment 313 (file offsets 0x089d60 to 0x08a223, 1219 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "itemrec.h"
#include "objref.h"
#include "type.h"
#include "npcref.h"
#include "u7npc.h"
#include "coord.h"
#include "gtimer.h"
#include "camera.h"
#include "ucvalue.h"
#include "uclist.h"
#include "actqueue.h"
#include "item.h"
#include "keyring.h"
#include "search.h"
#include "uccomm12.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define IS_NPC(rec) ((int8_t) ((CLASS_FLAGS(rec) & CLASS_NPC) != 0))
#define IS_DEAD(n) ((int8_t) (((n)->status & NPC_DEAD) != 0))
#define FLAG(v, m) ((uint8_t) ((v) & (m)))

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x5c: stop an item's scripts */
void UC_HaltScheduled(Value *args)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if ((int8_t)r.valid())
		HaltItemScripts(obj, (int8_t)(objref(AvatarRef.off).off == r.off) && IS_DEAD(GetNpcBufferForIbo(&r)) ? 0 : 1);
}

/* 0x3b: the time of day, in three-hour periods */
void UC_GetTime(Value *args, Value *ret)
{
	ret->appendInt((int16_t)GameTime.getHour() / 3);
}

/* 0x2f: whether an item is on screen, and for an NPC, conscious and visible */
void UC_IsNPCNear(Value *args, Value *ret)
{
	int16_t result = 1;
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref target = obj;

	if ((int8_t)target.valid()) {
		if ((int8_t)gCamera.isOnScreen(Item_getX(target), Item_getY(target))) {
			if (IS_NPC(ITEM(target.off)) && (IsNpcUnconscious(&NPCRef((objref &)target))
				|| FLAG(Item_getQualityFlags(&NPCRef((objref &)target)), QUALITY_INVISIBLE)))
				result = 0;
		} else
			result = 0;
	} else
		result = 0;
	ret->appendInt(result);
}

void UC_CanVisit(Value *args, Value *ret)
{
	int16_t result = 1;
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref target = obj;

	if (!target.valid() || !CanVisit(&target))
		result = 0;
	ret->appendInt(result);
}

void UC_KeyringContains(Value *args, Value *ret)
{
	uint8_t key = GetListNode(args - 1, 1)->toInt();

	ret->appendInt(KeyRing.contains(key));
}

void UC_KeyringAdd(Value *args)
{
	uint8_t key = GetListNode(args - 1, 1)->toInt();

	KeyRing.add(key);
}

void UC_RemoveItemsInArea(Value *args)
{
	int16_t type = GetListNode(args - 1, 1)->toInt();
	int16_t frame = GetListNode(args - 2, 1)->toInt();
	Coord x0 = GetListNode(args - 3, 1)->toInt();
	Coord y0 = GetListNode(args - 3, 2)->toInt();
	Coord x1 = GetListNode(args - 4, 1)->toInt();
	Coord y1 = GetListNode(args - 4, 2)->toInt();
	objref current;
	AreaSearch search;

	if (type == 275) {
		FindItemInArea(&search, x0, y0, x1, y1, 16, type, -1, frame);
		while ((uint8_t)(TYPE_CLASS(search.current.ptr()) == TYPE_CLASS_EGG)) {
			if (search.current.frame() == frame) {
				current = search.current;
				FindItem(&search);
				Item_detach(&current);
				ZapDetachedItem(&current);
			}
		}
	} else {
		FindItemInArea(&search, x0, y0, x1, y1, 0, type, -1, frame);
		while ((int8_t)search.current.valid()) {
			if (search.current.frame() == frame) {
				current = search.current;
				FindItem(&search);
				Item_detach(&current);
				ZapDetachedItem(&current);
			}
		}
	}
}
