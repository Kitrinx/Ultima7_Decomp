/* Serpent Isle SI.EXE, overlay segment 213 (file offsets 0x054160 to 0x054b00, 2464 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "typefram.h"
#include "iteminfo.h"
#include "itemrec.h"
#include "objref.h"
#include "u7npc.h"
#include "barge.h"
#include "cheat.h"
#include "search.h"
#include "usehook.h"
#include "wihh.h"
#include "missile.h"
#include "npcpath.h"
#include "type.h"
#include "npcref.h"
#include "coord.h"
#include "collide.h"
#include "mapview.h"
#include "spell.h"
#include "cast.h"

#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))
#define ITEM(ref) ((struct ItemRecord far *)MK_FP(ItemBufferSegment, (ref).off))
#define TYPE(ref) (ITEM(ref)->typeFrame & 0x3ff)
#define TYPE_CLASS(ref) (gItemTypeInfo[TYPE(ref)].typeClass)
#define IS_CLASS(ref, n) ((unsigned char)(TYPE_CLASS(ref) == (n)))

/* A copy of a reference, made so its address can be passed on */
struct RefCopy {
	int value;
	RefCopy(objref item) { value = item.off; }
};

extern unsigned char far IsItemDetached(objref *ref);
extern char far Item_delete(objref *ref);
extern void far Item_storeQuantity(objref *, unsigned char);
extern void far Item_setFrame(objref *, int);
extern unsigned char far Item_getQuantity(objref *ref);
extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern unsigned far SpellReagents[];


int SpellCost;
objref ReagentItems[11];

inline int NeedsReagent(int spell, int reagent)
{
	return (1 << reagent) & SpellReagents[spell];
}

inline unsigned char GetMana(objref *npc)
{
	return GetNpcBufferForIbo(npc)->mana & 0x1f;
}

inline void SetMana(objref *npc, unsigned char mana)
{
	GetNpcBufferForIbo(npc)->mana = mana | (GetNpcBufferForIbo(npc)->mana & 0xe0);
}

unsigned char HasQuantityFrames(unsigned type)
{
	switch (type) {
	case 417:   /* magic bolt */
	case 554:   /* burst arrow */
	case 556:   /* magic arrow */
	case 558:   /* lucky arrow */
	case 568:   /* Tseramed arrow */
	case 591:
	case 627:   /* lockpick */
	case 644:   /* gold coin */
	case 722:   /* arrow */
	case 723:   /* bolt */
	case 948:
	case 951:
	case 952:
		return 1;
	default:
		return 0;
	}
}

void Item_setQuantity(objref item, int quantity, unsigned char keepEmpty)
{
	unsigned type = TYPE(item);
	if (TYPE_CLASS(item) != TYPE_CLASS_QUANTITY) {
		return;
	}
	int frame = 0;
	if (quantity == 0 && !keepEmpty) {
		if (!IsItemDetached(&item)) {
			Item_delete(&item);
		}
		return;
	}
	Item_storeQuantity(&item, quantity);
	switch (type) {
	case 417:   /* magic bolt */
	case 554:   /* burst arrow */
	case 556:   /* magic arrow */
	case 558:   /* lucky arrow */
	case 568:   /* Tseramed arrow */
	case 591:
	case 722:   /* arrow */
	case 723:   /* bolt */
		frame = 24;
		/* fall through */
	case 948:
	case 951:
	case 952:
	case 627:   /* lockpick */
	case 644:   /* gold coin */
		break;
	default:
		return;
	}
	if (quantity <= 6) {
		frame += quantity - 1;
	} else if (quantity > 6 && quantity <= 12) {
		frame += 6;
	} else {
		frame += 7;
	}
	Item_setFrame(&item, frame);
}

unsigned char CanCastSpell(objref caster, int spell, unsigned char useReagents, unsigned char useMana)
{
	int mana;
	AreaSearch found;
	if (HackMoverEnabled == 0) {
		ReagentCounter supply;
		supply.countReagentsInPossession(NPCRef(caster));
		if (useMana) {
			int circle = spell / 8;
			SpellCost = circle + 1;
			mana = GetMana((objref *)&RefCopy(caster));
			if (SpellCost > mana) {
				return 0;
			}
			if (Npc_getLevel((objref *)&RefCopy(caster)) < circle) {
				return 0;
			}
		}
		if (useReagents && !HasReagentRing()) {
			for (int reagent = 0; reagent < 11; reagent++) {
				ReagentItems[reagent].off = 0;
				if (NeedsReagent(spell, reagent)) {
					if (FindItemInContainer(&found, caster, 0, 842, 255, reagent)) {    /* reagent */
						ReagentItems[reagent] = found.current;
					} else {
						return 0;
					}
				}
			}
		}
	}
	return 1;
}

unsigned char TryToCastSpell(objref caster, int spell, unsigned char useReagents, unsigned char useMana,
	unsigned char event)
{
	unsigned char savedHack;
	if ((useReagents || useMana) && !HackMoverEnabled) {
		if (!CanCastSpell(caster, spell, useReagents, useMana)) {
			return 0;
		}
		if (useReagents && !HasReagentRing()) {
			for (int reagent = 0; reagent < 11; reagent++) {
				if (ReagentItems[reagent].valid()) {
					int quantity = Item_getQuantity(&ReagentItems[reagent]);
					Item_setQuantity(ReagentItems[reagent], quantity - 1, 0);
				}
			}
		}
		if (useMana)
			SetMana((objref *)&RefCopy(caster), GetMana((objref *)&RefCopy(caster)) - SpellCost);
	}
	LeaveVehicle();
	savedHack = HackMoverEnabled;
	HackMoverEnabled = 1;
	RunUsable(event, caster.off, spell + 0x640);
	HackMoverEnabled = savedHack;
	CastingFramesLeft = 14;
	return 1;
}

unsigned char HasReagentRing()
{
	ItemId right = GetItemInSlot(AvatarRef, 8);
	ItemId left = GetItemInSlot(AvatarRef, 7);
	int rightType = TYPE(right);
	int leftType = TYPE(left);
	int rightFrame = (ITEM(right)->typeFrame & 0x7c00) >> 10;
	int leftFrame = (ITEM(left)->typeFrame & 0x7c00) >> 10;
	if (rightType == 296 && rightFrame == 3) {
		return 1;
	}
	if (leftType == 296 && leftFrame == 3) {
		return 1;
	}
	return 0;
}

unsigned char CanAvatarReach(objref target, unsigned char ignoreRange)
{
	int x, y;
	int distance;
	int failures;
	int minX, maxX, minY, maxY;
	int z, minZ, maxZ;
	int stepX, stepY, stepZ;
	int targetZ, avatarZ;

	if (HackMoverEnabled != 0 || ignoreRange != 0 || IS_CLASS(target, TYPE_CLASS_HUMAN)) {
		return 1;
	}
	failures = 0;
	stepX = 1;
	stepY = 1;
	stepZ = 1;
	targetZ = Item_getZ(&target);
	avatarZ = Item_getZ(&AvatarRef);
	minZ = targetZ - gItemTypeInfo[TYPE(AvatarRef)].height - 1;
	if (minZ < 0) {
		minZ = 0;
	}
	maxZ = targetZ + gItemTypeInfo[TYPE(target)].height + 1;
	if (maxZ > CeilingZ) {
		maxZ = CeilingZ;
	}
	if (avatarZ > targetZ) {
		int swap = minZ;
		minZ = maxZ;
		maxZ = swap;
		stepZ = -1;
	}
	minX = Item_getX(target) - GetFootprintX(ITEM(target)->asTypeFrame()) - 3;
	maxX = Item_getX(target) + 3;
	if (Item_getX(AvatarRef) >= Item_getX(target)) {
		int swap = minX;
		minX = maxX;
		maxX = swap;
		stepX = -1;
	}
	minY = Item_getY(target) - GetFootprintY(ITEM(target)->asTypeFrame()) - 3;
	maxY = Item_getY(target) + 3;
	if (Item_getY(AvatarRef) >= Item_getY(target)) {
		int swap = minY;
		minY = maxY;
		maxY = swap;
		stepY = -1;
	}
	for (x = minX; stepX == 1 ? x <= maxX : x >= maxX; x += stepX) {
		for (y = minY; stepY == 1 ? y <= maxY : y >= maxY; y += stepY) {
			for (z = minZ; stepZ == 1 ? z <= maxZ : z >= maxZ; z += stepZ) {
				if (!IsTypeSupportedAt(x, y, z, ITEM(AvatarRef)->asTypeFrame())) {
					continue;
				}
				if (IsTypeBlockedAt(x, y, z, ITEM(AvatarRef)->asTypeFrame())) {
					continue;
				}
				if (HasLineOfFireToCoords(target, x, y, z + 2)) {
					distance = 999;
					if (CanFindPath(AvatarRef, x, y, z, &distance, 30)) {
						return 1;
					}
					if (distance < 999) {
						failures++;
					}
				}
				if (failures > 14) {
					return 0;
				}
			}
		}
	}
	return 0;
}
