/* Serpent Isle SI.EXE, resident segment 20 (file offsets 0x01363a to 0x01383c, 514 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

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

#define MK_FP(seg, ofs) ((void _seg *) (seg) + (void near *) (ofs))
#define ITEM(off) ((struct ItemRecord far *) MK_FP(ItemBufferSegment, (off)))

extern char far Item_delete(objref *ref);
extern unsigned char far Item_move(objref *, Loc, Loc, int);

#define TYPE(off) (ITEM(off)->typeFrame & 0x3ff)
#define TYPE_CLASS(off) (gItemTypeInfo[TYPE(off)].typeClass)
#define IS_CLASS(off, f) ((unsigned char) (TYPE_CLASS(off) == (f)))
#define HAS_TYPE_CLASS(off) ((unsigned char) !IS_CLASS(off, TYPE_CLASS_NONE))
#define IS_NPC(off) ((unsigned char) ((ItemTypeClassFlags[TYPE_CLASS(off)] & CLASS_NPC) != 0))
#define NPC_FLAG(n, bit) ((unsigned char) (((n)->status & (bit)) != 0))
#define INFO_FLAG(v, bit) ((unsigned char) ((v).flags & (bit)))

void far DetachItem(int item)
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

void far SendNPCToLunch(NPCRef npc)
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
