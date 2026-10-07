/* Serpent Isle SI.EXE, resident segment 20 (file offsets 0x01363a to 0x01383c, 514 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "iteminfo.h"
#include "itemrec.h"
#include "u7npc.h"
#include "npcref.h"
#include "coord.h"
#include "debug.h"
#include "sprite.h"
#include "actqueue.h"
#include "missile.h"
#include "misstrac.h"
#include "random.h"
#include "type.h"
#include "objref.h"
#include "combatai.h"

#define ITEM(off) ((struct ItemRecord *) ItemAt((off)))

extern int8_t Item_delete(objref *ref);
extern uint8_t Item_move(objref *, Loc, Loc, int16_t);

#define TYPE(off) (ITEM(off)->typeFrame & 0x3ff)
#define TYPE_CLASS(off) (gItemTypeInfo[TYPE(off)].typeClass)
#define IS_CLASS(off, f) ((uint8_t) (TYPE_CLASS(off) == (f)))
#define HAS_TYPE_CLASS(off) ((uint8_t) !IS_CLASS(off, TYPE_CLASS_NONE))
#define IS_NPC(off) ((uint8_t) ((ItemTypeClassFlags[TYPE_CLASS(off)] & CLASS_NPC) != 0))
#define NPC_FLAG(n, bit) ((uint8_t) (((n)->status & (bit)) != 0))
#define INFO_FLAG(v, bit) ((uint8_t) ((v).flags & (bit)))

void DetachItem(int16_t item)
{
	objref r = item;

	if (HAS_TYPE_CLASS(r.off) && !IS_CLASS(r.off, TYPE_CLASS_BUILDING)) {
		ActionQueue.remove(item, 0, 1);
		if (ItemSpritesChanged)
			SpriteManager_stopItemSprites(&gSpriteManager, item, 0);
		if (ActiveMissiles)
			CheckMissilesForItem(r);
		if (IS_NPC(r.off) && !NPC_FLAG(GetNpcBufferForIbo(&NPCRef(r.off)), NPC_DEAD))
			RemoveFromCombat(NPCRef(r.off));
	}
}

void SendNPCToLunch(NPCRef npc)
{
	Coord x, y;
	ItemInfo info;

	if (!IS_NPC(npc.off)) {
		CheatPrintfWait("ERROR: lunch non-sentient");
		return;
	}
	DetachItem(npc.off);
	info = GetItemZAndStuff(&npc);
	if (INFO_FLAG(info, 8))  /* temporary */
		Item_delete(&npc);
	else {
		x = GenerateRandomIntegerInRange(44) + 130;
		y = GenerateRandomIntegerInRange(27) + 35;
		Item_move(&npc, x, y, 0);
	}
}
