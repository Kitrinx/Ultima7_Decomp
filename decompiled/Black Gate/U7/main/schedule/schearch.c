/* Black Gate U7.EXE, overlay segment 307 (file offsets 0x08e700 to 0x08f18f, 2703 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "item.h"
#include "coord.h"
#include "sprite.h"
#include "actitem.h"
#include "actmove.h"
#include "sortitem.h"
#include "wihh.h"
#include "actutil.h"
#include "random.h"
#include "equip.h"
#include "legalmov.h"
#include "itable.h"
#include "combatai.h"
#include "damage.h"
#include "missile.h"
#include "search.h"
#include "text.h"
#include "script.h"
#include "cbattack.h"

inline char HasClearShot(ItemId attacker, Loc x, Loc y, int z)
{
	return HasLineOfFireToCoords(attacker, x, y, z);
}

inline char HasClearShot(ItemId attacker, ItemId target)
{
	return HasLineOfFire(attacker, target);
}

inline char AttackTarget(objref attacker, Loc x, Loc y, int z, int weapon)
{
	return AttackCoordsWithWeapon(attacker, x, y, z, weapon);
}

inline char AttackTarget(objref attacker, objref target, int weapon)
{
	return AttackItemWithWeapon(attacker, target, weapon);
}

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)

/* Archery practice: clear the target, take up a bow and arrows, pace fourteen steps back and shoot. */
void far DoWorkArchery(objref *npc)
{
	int roll;
	objref item;
	objref held;
	int weapon;
	unsigned move;
	int count;
	AreaSearch target;

	switch (CUR_SCHED(npc).state) {
	case 0:
		StowHeldItems(npc);
		CUR_SCHED(npc).state++;
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 735 /* archery target */, -1);
		break;
	case 2:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else
			CUR_SCHED(npc).state++;
		break;
	case 3:
		FindNearestItem(&target, Item_getX(*npc), Item_getY(*npc), 4, 0, 735 /* archery target */, 0xff, 0xff);
		if (target.found() && FRAME(ITEM(target.current.off)) != 0) {
			if (FRAME(ITEM(target.current.off)) % 3 == 1)
				NPC(npc)->scheduleToggle = 1;
			else
				NPC(npc)->scheduleToggle = 0;
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_RAISE1_FRAME, SCRIPT_WAIT, 1,
				SCRIPT_EXTEND1_FRAME, SCRIPT_WAIT, 2, SCRIPT_THRUST1_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
		} else
			CUR_SCHED(npc).state = 5;
		break;
	case 4:
		FindNearestItem(&target, Item_getX(*npc), Item_getY(*npc), 4, 0, 735 /* archery target */, 0xff, 0xff);
		Item_setFrame(&target.current, NPC(npc)->scheduleToggle ? 0 : FRAME(ITEM(target.current.off)) - 1);
		if (FRAME(ITEM(target.current.off)) == 0)
			CUR_SCHED(npc).state++;
		else
			CUR_SCHED(npc).state = 3;
		break;
	case 5:
		Npc_pushSchedule(npc, WORK_READY_HAND, 597 /* bow */, 1);
		break;
	case 6:
		held = objref(GetItemInSlot(*npc, 8));
		if (held.valid()) {
			if (held.type() == 722 /* arrow */) {
				CUR_SCHED(npc).state++;
				break;
			}
			Item_moveIntoContainer(&held, *npc);
		}
		item = CreateCarriedItem(MakeTypeFrame(722 /* arrow */, 0), *npc);
		if (item.valid()) {
			roll = GenerateRandomIntegerInRange(10);
			if (roll < 2)
				count = 1;
			else if (roll < 5)
				count = 2;
			else
				count = 3;
			Item_storeQuantity(&item, count);
			/* frames 24-26 show one to three arrows */
			Item_setFrame(&item, count + 23);
			EquipItem(item, *npc, 8, 0);
		}
		CUR_SCHED(npc).state++;
		break;
	case 7:
		NPC(npc)->scheduleValue = 0;
		CUR_SCHED(npc).state++;
		break;
	case 8:
		if (NPC(npc)->scheduleValue == 14)
			CUR_SCHED(npc).state++;
		else {
			move = CheckMove(Item_getX(*npc), Item_getY(*npc), Item_getZ(npc),
				ITEM(npc->off)->typeFrame, 2, 1, NPC(npc)->typeFlags);
			if ((move & 0x8000) && !(move & 0x2000)) {
				StepItem(*npc, 2, (char) move, 1);
				NPC(npc)->scheduleValue++;
			} else if (RollChance(5))
				Npc_popSchedule(npc, -1);
			else {
				FindItemInArea(&target,
					Coord(Item_getX(*npc) + DirDeltaX[(unsigned char) (NPC(npc)->status & 7)]),
					Coord(Item_getY(*npc) + DirDeltaY[(unsigned char) (NPC(npc)->status & 7)]),
					4, -1, 0xff, 0xff);
				if (target.found()) {
					/* "Step aside so that I can practice!", "Thou art in my way!" or "Kindly step aside!" */
					SpriteManager_barkOnItem(&gSpriteManager, *npc,
						GetGameText(1, GenerateRandomIntegerInRange(3) + 93), 0, 15, 0);
					PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, 15, SCRIPT_END));
					CUR_SCHED(npc).state = 8;
				} else
					Npc_popSchedule(npc, -1);
			}
		}
		break;
	case 9:
		NPC(npc)->scheduleToggle = 0;
		PostScheduleScript(npc, MakeScript(SCRIPT_FACE, 54, SCRIPT_WAIT, 5, SCRIPT_END));
		break;
	case 10:
		FindItemInArea(&target, Coord(Item_getX(*npc) - 17), Coord(Item_getY(*npc) + 1),
			Item_getX(*npc), Coord(Item_getY(*npc) + 1), 0, 735 /* archery target */, 0xff, 0xff);
		if (target.found()) {
			if (NPC(npc)->scheduleToggle) {
				NPC(npc)->scheduleToggle = 0;
				item = objref(target.current.off);
				if (item.frame() == 0)
					PostScriptToItem(&item, MakeScript(SCRIPT_WAIT, 1,
						SCRIPT_FRAME, GenerateRandomIntegerInRange(8) * 3 + 1, SCRIPT_END));
				else if (item.frame() % 3 != 0)
					PostScriptToItem(&item, MakeScript(SCRIPT_WAIT, 1, SCRIPT_FRAME, item.frame() + 1,
						SCRIPT_END));
			}
			weapon = GetNpcWeapon(npc);
			if (CountWeaponAmmo(*npc, weapon)) {
				if (HasClearShot(*npc, objref(target.current.off))) {
					AttackTarget(*npc, objref(target.current.off), weapon);
					NPC(npc)->scheduleToggle = 1;
					break;
				}
				if (RollChance(4) == 0) {
					/* "Watch out!", "Move!" or "Stand aside, I am practicing!" */
					SpriteManager_barkOnItem(&gSpriteManager, *npc,
						GetGameText(1, GenerateRandomIntegerInRange(3) + 90), 0, 15, 0);
					PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, 15, SCRIPT_END));
					CUR_SCHED(npc).state = 10;
					break;
				}
			} else {
				if (RollChance(3))
					Npc_popSchedule(npc, 0);
				else
					CUR_SCHED(npc).state = 0;
				break;
			}
		}
		Npc_popSchedule(npc, -1);
		break;
	}
}
