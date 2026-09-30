/* Black Gate U7.EXE, overlay segment 225 (file offsets 0x0672c0 to 0x067ed9, 3097 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "iteminfo.h"
#include "item.h"
#include "npcref.h"
#include "voolook.h"
#include "u7npc.h"
#include "actqueue.h"
#include "cheat.h"
#include "crime.h"
#include "makemojo.h"
#include "monsters.h"
#include "script.h"
#include "special.h"
#include "random.h"
#include "damage.h"
#include "itemovr1.h"
#include "legalmov.h"
#include "slime.h"
#include "type.h"
#include "coord.h"
#include "collide.h"
#include "search.h"
#include "daze.h"

struct MonsterRef {
	int index;
	MonsterRef(int n) { index = n; }
	MonsterRecord *operator->() { return MonsterRecords.get(index); }
};
struct ScriptBuffer {
	unsigned char length;
	unsigned char data[127];
	ScriptBuffer() { length = 1; }
};

inline unsigned char ItemTop(objref *r) { return Item_getZ(r) + gItemTypeInfo[r->type()].height; }

int far SplitCreature(objref ref)
{
	objref created;
	Coord x, y;
	AreaSearch nearby;
	int attempts = 8;

	while (1) {
		int z;

		if ((unsigned char)gItemTypeInfo[ref.type()].strangeMovement && HasEvenCellPlacement(ref.type())) {
			x = Coord((int)Item_getX(ref) - (1 - GenerateRandomIntegerInRange(3)) * 2);
			y = Coord((int)Item_getY(ref) - (1 - GenerateRandomIntegerInRange(3)) * 2);
		} else {
			x = Coord((int)Item_getX(ref) + GenerateRandomIntegerInRange(3) - 1);
			y = Coord((int)Item_getY(ref) + GenerateRandomIntegerInRange(3) - 1);
		}
		attempts--;
		if (attempts < 0)
			return 0;
		if (Item_getX(ref) == x && Item_getY(ref) == y)
			continue;
		if (FindItemInArea(&nearby, x, y, 4, -1, 255, 255))
			continue;
		/* z takes the result of the comparison, not the support level */
		if ((z = FindSupportLevel(x, y, ItemTop(&ref), &TypeFrame(ref.ptr()->typeFrame)) > Item_getZ(&ref) + 1) != 0)
			continue;
		if (!CanWalkAt(x, y, z, ref.ptr()->typeFrame))
			continue;
		CloneItem(&ref);
		PlaceItem(&created, x, y, z);
		return created.off;
	}
}

unsigned char far FallToGround(objref ref)
{
	int oldZ = Item_getZ(&ref);
	int newZ;
	Coord x, y;

	Item_getXAndY(ref, &x.value, &y.value);
	Item_detach(&ref);
	newZ = FindSupportLevel(x, y, oldZ, &TypeFrame(ref.ptr()->typeFrame));
	PlaceItem(&ref, x, y, newZ);
	return oldZ != newZ;
}

unsigned char far PutToSleep(NPCRef ref)
{
	if (HasStatus(&ref, 0x80) || HasStatus(&ref, 0x8000) || (PowerAvatar && (unsigned char)Item_isAvatar(&ref)))
		return 0;
	MonsterRef monster = MonsterLookup.get(ref.type());
	if ((unsigned char)monster->powerSafe || (unsigned char)monster->sleepSafe)
		return 0;
	if (ref.type() == 529 /* slime */) {
		LayDown(ref, 0);
	} else {
		ScriptBuffer script;

		ActionQueue.remove(ref.off, 1, 0);
		AppendScriptByte(&script.length, SCRIPT_STAND_FRAME);
		AppendScriptByte(&script.length, SCRIPT_KNEEL_FRAME);
		AppendScriptByte(&script.length, SCRIPT_SFX);
		AppendScriptByte(&script.length, 86);
		AppendScriptByte(&script.length, SCRIPT_LIE_FRAME);
		ActionQueue.add(ref.off, (char *)&script);
	}
	ChangeStatus(&ref, 0, 0x80);
	return 1;
}

void far ApplyCharm(objref *ref, unsigned char mode)
{
	CharmNpc(ref, mode);
}

void far ApplyCurse(NPCRef ref)
{
	if (HasStatus(&ref, 0x200) || (PowerAvatar && (unsigned char)Item_isAvatar(&ref)))
		return;
	MonsterRef monster = MonsterLookup.get(ref.type());
	if ((unsigned char)monster->powerSafe || (unsigned char)monster->curseSafe)
		return;
	ChangeStatus(&ref, 0, 0x200);
}

unsigned char far ApplyPoison(NPCRef ref)
{
	if (HasStatus(&ref, 0x2000) || (PowerAvatar && (unsigned char)Item_isAvatar(&ref)))
		return 0;
	MonsterRef monster = MonsterLookup.get(ref.type());
	if ((unsigned char)monster->poisonSafe)
		return 0;
	ChangeStatus(&ref, 0, 0x2000);
	return 1;
}

void far ApplyParalysis(NPCRef ref)
{
	if (HasStatus(&ref, 0x1000) || (PowerAvatar && (unsigned char)Item_isAvatar(&ref)))
		return;
	MonsterRef monster = MonsterLookup.get(ref.type());
	if ((unsigned char)monster->powerSafe || (unsigned char)monster->paralysisSafe)
		return;
	ChangeStatus(&ref, 0, 0x1000);
	/* a paralyzed flyer falls, and is hurt if it drops */
	if (GetNpcBufferForIbo(&ref)->moving(0x10)) {
		if (FallToGround(ref))
			ReduceHealth(ref, GenerateRandomIntegerInRange(5) + 1, 0, 0);
	}
}

void far EndSleep(objref ref)
{
	if (!HasStatus(&ref, 0x80))
		return;
	ActionQueue.remove(ref.off, 1, 0);
	ActionQueue.add(ref.off, (char *)MakeScript(SCRIPT_KNEEL_FRAME, SCRIPT_STAND_FRAME, SCRIPT_END));
	ChangeStatus(&ref, 0x80, 0);
}

void far EndCharm(objref ref)
{
	if (!HasStatus(&ref, 0x100))
		return;
	UncharmNpc(&ref);
}

void far EndCurse(NPCRef ref)
{
	if (!HasStatus(&ref, 0x200))
		return;
	ChangeStatus(&ref, 0x200, 0);
}

void far EndPoison(NPCRef ref)
{
	if (!HasStatus(&ref, 0x2000))
		return;
	ChangeStatus(&ref, 0x2000, 0);
}

void far EndParalysis(NPCRef ref)
{
	if (!HasStatus(&ref, 0x1000))
		return;
	ChangeStatus(&ref, 0x1000, 0);
}
