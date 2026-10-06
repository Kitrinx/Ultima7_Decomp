/* Serpent Isle SI.EXE, overlay segment 249 (file offsets 0x06bd80 to 0x06ca3f, 3263 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <stdlib.h>
#include "lowlevel.h"
#include "activity.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "objref.h"
#include "u7npc.h"
#include "combmode.h"
#include "type.h"
#include "npcref.h"
#include "coord.h"
#include "combat.h"
#include "makemojo.h"
#include "memfree.h"
#include "text.h"
#include "gtimer.h"
#include "cast.h"
#include "colbuf.h"
#include "gumpmgr.h"
#include "cbattack.h"
#include "combatai.h"
#include "usehook.h"
#include "wihh.h"
#include "damage.h"
#include "partymov.h"
#include "slime.h"
#include "spell.h"
#include "crime.h"
#include "cheat.h"
#include "voolook.h"
#include "ready.h"
#include "weapons.h"
#include "item.h"

extern unsigned char far Item_getQuality(objref *ref);

inline unsigned char IsReady(NPCRef &npc) { return CombatGroups.movePoints[npc.number()] >= 24 ? 1 : 0; }
inline unsigned char CanAct(NPCRef &npc) { return Item_getQuality(&npc) != 9; }

struct SpellbookRef : objref {
	SpellbookRef(objref r) { off = r.off; }
	unsigned char selected();
};

inline unsigned char SpellbookRef::selected() { return GetSpellbookBookmark(this); }

struct WeaponRef {
	int index;
	WeaponRef() {}
	void operator=(int n) { index = n; }
	unsigned char isNull() { return index == 0; }
};

struct ArmorRef {
	unsigned index;
	ArmorRef(unsigned n) { index = n; }
	unsigned char isNull() { return index == 0; }
};

extern "C" int far Item_greatestDeltaToItem(objref &ref, objref other);
extern int far Item_greatestDeltaToCoords(objref &ref, CellCoord targetX, CellCoord targetY, unsigned targetZ);
extern unsigned char far Item_getQualityFlags(objref *);

inline char AttackWithWeapon(objref actor, objref target, int weapon)
{
	return AttackItemWithWeapon(actor, target, weapon);
}
inline char AttackWithWeapon(objref actor, Coord x, Coord y, int z, int weapon)
{
	return AttackCoordsWithWeapon(actor, x, y, z, weapon);
}

/* Whether the item opens as a container gump: a party member's body, or one of the container types
 * (a chest only below quality 250); refused when near memory is short. */
unsigned char far Item_canBeOpened(objref object)
{
	int available, required;
	unsigned char allowed;
	available = NearMemory.getNearFree();
	required = GetDialogMemoryNeeded();
	allowed = 0;
	if (object.isBody()) {
		if (NPCRef(object).isAvatar())
			allowed = 1;
		if (NPCRef(object).flag(NPC_IN_PARTY))
			allowed = 1;
		else
			allowed = 0;
	} else {
		switch (object.type()) {
		case 800:
			if (Item_getQuality(&object) > 249) {
				allowed = 0;
				break;
			}
		case 283: case 297: case 400: case 402: case 405: case 406: case 407: case 414: case 416:
		case 507: case 555: case 679: case 715: case 761: case 762: case 778: case 801: case 802:
		case 803: case 804: case 819: case 892:
			allowed = 1;
			break;
		default:
			allowed = 0;
		}
	}
	if (available < required && allowed != 0) {
		ReportNoCanDo(5);
		allowed = 0;
	}
	return allowed;
}

/* Double-click on an item or a spot: open, use or talk; in combat, attack it with the readied weapon
 * or cast the bookmarked spell. */
void far Use(objref *object, Coord x, Coord y, int z)
{
	unsigned char combat = IsAvatarInCombat();
	if (object->valid()) {
		if (object->isBody() && NPCRef(*object).flag(NPC_IN_PARTY) && !NPCRef(*object).isAvatar()) {
			if (!HackMoverEnabled) {
				if (!CanAvatarReach(*object, 0)) {
					ReportNoCanDo(7);
					return;
				}
			}
			if (combat)
				OpenAndLoopItemDialog(*object);
			else
				RunUsable(1, object->off, -1);
			return;
		}
		if (Item_canBeOpened(*object)) {
			if (!HackMoverEnabled) {
				if (!CanAvatarReach(*object, 0)) {
					ReportNoCanDo(7);
					return;
				}
			}
			OpenAndLoopItemDialog(*object);
			return;
		}
		if (!IsAvatarInCombat()) {
			if (!HackMoverEnabled) {
				if (!object->isNpc() && !CanAvatarReach(*object, 0)) {
					ReportNoCanDo(7);
					return;
				}
			}
			if (object->isNpc()) {
				if (NPCRef(*object).buffer()->workType == WORK_COMBAT)
					return;
				if (abs((unsigned)object->z() - AvatarRef.z()) > 3)
					return;
				if (GameTime.running())
					RunUsable(1, object->off, -1);
			} else {
				RunUsable(1, object->off, -1);
			}
			return;
		}
	}
	if (IsAvatarInCombat()) {
		objref target;
		objref held;
		WeaponRef weaponIndex;
		unsigned char needsAmmo, thrown;
		WeaponRecord details;
		held = objref(GetItemInSlot(AvatarRef, 1));
		if (!held.valid()) {
			weaponIndex = -1;
			needsAmmo = 0;
		} else if (held.type() == 761) {   /* spellbook */
			if (object->valid()) {
				if (Item_getQuality(&NPCRef(AvatarRef)) != 9 && object->isNpc())
					Npc_setTarget(&NPCRef(AvatarRef), NPCRef(*object).number(), 1);
				Npc_setItemTarget(&AvatarRef, object->off);
			} else {
				Npc_setCoordTarget(&AvatarRef, x, y, z);
			}
			if (!TryToCastSpell(AvatarRef, SpellbookRef(held).selected(), 1, 1, 4)) {
				ReportNoCanDo(0);
				return;
			}
			return;
		} else {
			weaponIndex = GetWeaponNumber(held);
			if (weaponIndex.isNull()) {
				unsigned char slot = ReadyRecords.get(ReadyLookup.get(held.type()))->slot;
				ArmorRef armor = ArmorLookup.get(held.type());
				if (armor.isNull() && (slot == 1 || slot == 20) && HasUsable(held.type())) {
					if (object->valid()) {
						Npc_setItemTarget(&AvatarRef, object->off);
					} else {
						Npc_setCoordTarget(&AvatarRef, x, y, z);
					}
					RunUsable(4, object->off, held.type());
					return;
				}
			}
			WeaponRecords.read(weaponIndex.index, &details);
			thrown = details.uses == 3;
			needsAmmo = details.ammo != -1 && (thrown || (details.uses != 2 && details.uses != 1));
		}
		if (object->multipart()) {
			target = objref(GetStrangeMoverTarget(*object));
		} else {
			target = *object;
		}
		if (!needsAmmo || (needsAmmo && CountWeaponAmmo(AvatarRef, weaponIndex.index))) {
			if ((object->valid() && !target.isNpc() &&
				(unsigned)Item_greatestDeltaToItem(target, AvatarRef) > GetWeaponRange(AvatarRef, weaponIndex.index)) ||
				(!object->valid() &&
					Item_greatestDeltaToCoords(AvatarRef, x, y, z) > GetWeaponRange(AvatarRef, weaponIndex.index))) {
				ReportNoCanDo(2);
			} else if (!CanAct(NPCRef(AvatarRef)) || !object->valid() || !object->isNpc() || target != *object) {
				if (!(Item_getQualityFlags(&AvatarRef) & QUALITY_BUSY) && IsReady(NPCRef(AvatarRef))) {
					if ((object->valid() && AttackWithWeapon(AvatarRef, *object, weaponIndex.index)) ||
						(!object->valid() && AttackWithWeapon(AvatarRef, x, y, z, weaponIndex.index))) {
						CombatGroups.movePoints[Item_getNpcNumber(&AvatarRef)] = 0;
					} else if (!CanAct(NPCRef(AvatarRef))) {
						ReportNoCanDo(1);
					}
				}
			}
			if (CanAct(NPCRef(AvatarRef)) && target.isNpc()) {
				Npc_setTarget(&NPCRef(AvatarRef), NPCRef(target).number(), 1);
				CombatGroups.movePoints[Item_getNpcNumber(&AvatarRef)] = 0;
				if ((unsigned char)(Item_getQuality(&NPCRef(target)) == 7) || NPCRef(target).unconscious()) {
					KillNpcMode = 1;
				} else {
					KillNpcMode = 0;
				}
			}
		} else {
			ReportNoCanDo(3);
		}
	}
}

void far Item_setDefaultAttackMode(objref object, unsigned char mode)
{
	NPCRef(object).buffer()->defaultAttackMode = mode;
}

unsigned far Item_getDefaultAttackMode(objref object)
{
	return NPCRef(object).buffer()->defaultAttackMode;
}

void far Item_setAttackMode(objref object, unsigned char mode)
{
	SetAttackMode(NPCRef(object), mode);
}

unsigned far Item_getAttackMode(objref object)
{
	return Item_getQuality(&NPCRef(object));
}

unsigned char far Item_isPartyMember(objref object)
{
	int index;
	for (index = 0; index < PartySize; index++) {
		if (PartyMembers[index] == object)
			return 1;
	}
	return 0;
}
