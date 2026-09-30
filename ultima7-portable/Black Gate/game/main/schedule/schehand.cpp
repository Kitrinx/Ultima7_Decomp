/* Black Gate U7.EXE, overlay segment 309 (file offsets 0x08f4b0 to 0x08f7f8, 840 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "activity.h"
#include "u7npc.h"
#include "item.h"
#include "actitem.h"
#include "actmove.h"
#include "wihh.h"
#include "actutil.h"
#include "equip.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/*
 * Take an item of the wanted type into the hand (slot 1): from the pack, else from nearby, else, when
 * the second argument asks for it, a new one.
 */
void DoWorkReadyHand(objref *npc)
{
	objref held;
	objref item;

	switch (CUR_SCHED(npc).state) {
	case 0:
		held = objref(GetItemInSlot(*npc, 1));
		if (held.valid() && (uint16_t)CUR_SCHED(npc).x == held.type()) {
			Npc_popSchedule(npc, 0);
			break;
		}
		StowHeldItems(npc);
		CUR_SCHED(npc).state++;
		break;
	case 1:
		item = FindCarriedItem(npc, CUR_SCHED(npc).x);
		if (item.valid()) {
			EquipItem(item, *npc, 1, 0);
			Npc_popSchedule(npc, 0);
		} else
			CUR_SCHED(npc).state++;
		break;
	case 2:
		Npc_pushSchedule(npc, WORK_GRAB_ITEM, CUR_SCHED(npc).x, -1);
		break;
	case 3:
		if (IsBlocked(npc)) {
			if (CUR_SCHED(npc).y != 0) {
				item = CreateCarriedItem(MakeTypeFrame(CUR_SCHED(npc).x, 0), *npc);
				if (item.valid()) {
					EquipItem(item, *npc, 1, 0);
					Npc_popSchedule(npc, 0);
				} else
					Npc_popSchedule(npc, -1);
			} else
				Npc_popSchedule(npc, -1);
		} else
			CUR_SCHED(npc).state = 1;
		break;
	}
}
