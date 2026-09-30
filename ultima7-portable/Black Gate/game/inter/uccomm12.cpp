/* Black Gate U7.EXE, overlay segment 333 (file offsets 0x09ba70 to 0x09bbee, 382 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
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
	int16_t target = obj;

	if ((int8_t)gCamera.isOnScreen(Item_getX((objref &)target), Item_getY((objref &)target))) {
		if (IS_NPC(ITEM(target)) && (IsNpcUnconscious(&NPCRef((objref &)target))
			|| FLAG(Item_getQualityFlags(&NPCRef((objref &)target)), QUALITY_INVISIBLE)))
			result = 0;
	} else
		result = 0;
	ret->appendInt(result);
}
