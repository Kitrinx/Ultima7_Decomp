/* Serpent Isle SI.EXE, overlay segment 320 (file offsets 0x08dcc0 to 0x08e6da, 2586 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "objref.h"
#include "typefram.h"
#include "lowlevel.h"
#include "activity.h"
#include "iteminfo.h"
#include "coord.h"
#include "voolook.h"
#include "u7npc.h"
#include "combat.h"
#include "voice.h"
#include "itemcmd.h"
#include "main.h"
#include "preload.h"
#include "ucvalue.h"
#include "uclist.h"
#include "crime.h"
#include "crimeov1.h"
#include "makemojo.h"
#include "misstrac.h"
#include "text.h"
#include "palctrl.h"
#include "palfade.h"
#include "item.h"
#include "sche.h"
#include "schedit.h"
#include "gtimer.h"
#include "type.h"
#include "itemrec.h"
#include "u7sound.h"

extern objref AvatarRef;

const char UsecodeSpeechFileName[] = "u7speech.spc";  /* never referenced */
int16_t RemoteViewActive = 0;
extern "C" {
int16_t MarkedX = 0, MarkedY = 0, MarkedZ = 0;      /* declared Coord where they are used */
}
int8_t UnusedMarkByte = 0;

extern uint8_t StrikeItemWithWeapon(objref actor, objref target, int16_t weapon);

extern void SetPolymorph(objref npc, uint16_t shape);
extern void RevertSchedule(int16_t npc);
extern void GetRegionPosition(int16_t *x, int16_t *y, int16_t *region);

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x69: the first item attacks the second with the weapon shape in the third */
void UC_AttackObject(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	int16_t target = GetItemRef(GetListNode(args - 2, 1));
	objref r = obj;
	objref t = target;
	uint16_t weapon = ARG(args - 3, 1);
	int16_t type;

	CheckMojoBounds(INT32_C(1024), (uint32_t) weapon);
	type = PeekWord(WeaponLookup.addr + weapon * 2);
	ret->appendInt(StrikeItemWithWeapon(r, t, type));
}

/* 0x7f: flash the cursor that says why something cannot be done */
void UC_FlashMouse(Value *args, Value *)
{
	ReportNoCanDo(ARG(args - 1, 1));
}

/* 0x8c: fire a projectile from an item */
void UC_FireProjectile(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	int8_t dir = GetItemRef(GetListNode(args - 2, 1));
	int16_t type = ARG(args - 3, 1);
	int16_t speed = ARG(args - 4, 1);
	uint16_t weapon = ARG(args - 5, 1);
	uint16_t ammo = ARG(args - 6, 1);
	objref r = obj;
	objref from = obj;
	objref proj;
	int16_t wtype, atype;

	CheckMojoBounds(INT32_C(1024), (uint32_t) weapon);
	wtype = PeekWord(WeaponLookup.addr + weapon * 2);
	if (ammo == (uint16_t)UC_ALL) {
		CheckMojoBounds(INT32_C(1024), INT32_C(0));
		atype = PeekWord(AmmoLookup.addr);
	} else {
		CheckMojoBounds(INT32_C(1024), (uint32_t) ammo);
		atype = PeekWord(AmmoLookup.addr + ammo * 2);
	}
	if (CreateItem(&proj, type, Item_getX(r), Item_getY(r), Item_getZ(&r)))
		FireMissileInDirection(proj, wtype, atype, r, speed, dir, 3);
}

/* 0x8e: how fast time passes */
void UC_AdvanceTime(Value *args, Value *)
{
	TimeAdvanceRate = ARG(args - 1, 1);
}

/* 0x8d: the bed to nap in */
void UC_NapTime(Value *args, Value *)
{
	NapBed = ARG(args - 1, 1);
}

/* 0x8a: start a speech track; the result says whether it played */
void UC_StartSpeech(Value *args, Value *ret)
{
	int16_t played = 0;
	int16_t track = ARG(args - 1, 1);

	if ((int8_t)SpeechPlayer.selectTrack(track)) {
		SpeechPlayer.playFile(SpeechFileName, track);
		played = 1;
	}
	ret->appendInt(played);
}

/* 0x90: report a crime to the guards */
void UC_CallGuards(Value *, Value *)
{
	ReportCrime(0, 1, 0);
}

/* 0x92: set the guards on the avatar */
void UC_AttackAvatar(Value *, Value *)
{
	SetGuardsOnAvatar();
}

/* 0x93: guards near the avatar give up their arrests */
void UC_StopArrest(Value *, Value *)
{
	DismissRegionalGuards();
}

/* 0xa8: fade the palette in when the third argument is set, else out */
void UC_FadePalette(Value *args, Value *)
{
	int16_t ticks = ARG(args - 1, 1);
	int16_t unused = ARG(args - 2, 1);

	if (ARG(args - 3, 1))
		FadeScreenIn(ticks, unused);
	else
		FadeScreenOut(ticks, unused);
}

/* 0xa9: as 0xa8, with the music of falling asleep and waking */
void UC_FadeForSleep(Value *args, Value *)
{
	int16_t ticks = ARG(args - 1, 1);
	int16_t unused = ARG(args - 2, 1);

	if (ARG(args - 3, 1)) {
		PlayMusic(22);
		FadeScreenIn(ticks, unused);
		MarkPaletteFadedIn();
	} else {
		PlayMusic(24);
		FadeScreenOut(ticks, unused);
		MarkPaletteFadedOut();
	}
}

/* 0xab: whether combat music plays */
void UC_InCombat(Value *, Value *ret)
{
	ret->appendInt(CombatGroups.battleMusic);
}

/* 0x55: summon a monster */
void UC_Summon(Value *args, Value *)
{
	CombatGroups.spawnGroup(ARG(args - 1, 1), 1, 1,
		(uint8_t) (GetNpcBufferForIbo(&AvatarRef)->workType == WORK_COMBAT ? WORK_COMBAT : WORK_FOLLOW_AVT),
		10, 0, 0, 0, objref(0), 1);
}

/* 0xb3: make an NPC look like another shape */
void UC_Polymorph(Value *args, Value *)
{
	objref npc = GetItemRef(GetListNode(args - 1, 1));
	int16_t shape = ARG(args - 2, 1);

	if (npc.valid())
		SetPolymorph(npc, shape);
}

/* 0xb4: give an NPC back the schedules the game started with */
void UC_RevertSchedule(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid()) {
		int16_t npc = Item_getNpcNumber(&r);

		if (npc < 256)
			RevertSchedule(npc);
	}
}

/* 0xb7: put an NPC to its schedule for the time of day */
void UC_Schedule(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	uint8_t npc = Item_getNpcNumber(&r);

	if (npc <= 255 && r.valid()) {
		uint8_t time = GameTime.getHour() / 3;
		uint8_t type = Schedule_getWorkType(&ScheduleTable, npc, time);

		if (type == 255)
			AssertFail("This npc, %d, has no schedules!", npc);
		Npc_setSchedule(&r, type);
	}
}

/* 0xb5: set what an NPC does from a time of day, and where */
void UC_ChangeSchedule(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	uint8_t npc = Item_getNpcNumber(&r);

	if (!r.valid())
		return;
	uint8_t time = ARG(args - 2, 1);
	uint8_t type = ARG(args - 3, 1);
	int16_t x = ARG(args - 4, 1);
	int16_t y = ARG(args - 4, 2);
	int16_t region = 0;

	if (npc > 255)
		return;
	uint8_t n = Schedule_findEntry(&ScheduleTable, npc, time);

	if (n == 255 && (int16_t)Schedule_insertEntry(&ScheduleTable, npc))
		n = (uint8_t)(*(ScheduleTable.index + npc + 1) - ScheduleTable.index[npc]) - (uint8_t)1;
	Schedule_setEntryTime(&ScheduleTable, npc, n, time);
	Schedule_setEntryWorkType(&ScheduleTable, npc, n, type);
	GetRegionPosition(&x, &y, &region);
	Schedule_setEntryPlace(&ScheduleTable, npc, n, x, y, region);
}

/* 0xb6: replace an NPC's schedules: a list of times, a list of activities and a list of x, y pairs */
void UC_NewSchedule(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	uint8_t npc, type;
	int16_t x, y, region;
	uint8_t n;
	int16_t i, have, want;
	uint8_t time;
	int16_t extra;
	int16_t slot;

	if (!r.valid())
		return;
	if (!r.isBody())
		return;
	if ((uint16_t)Item_getNpcNumber(&r) > 255)
		return;
	npc = Item_getNpcNumber(&r);
	have = (uint8_t)(*(ScheduleTable.index + npc + 1) - ScheduleTable.index[npc]);
	want = LinkList_count(args - 2);
	if (want > have) {
		extra = want - have;
		for (i = 0; i < extra; i++)
			if (!Schedule_insertEntry(&ScheduleTable, npc))
				;       /* a full table is no error */
	} else if (want < have) {
		extra = have - want;
		for (i = 7; i >= 0; i--)
			if ((int16_t)Schedule_removeEntry(&ScheduleTable, npc, i) && --extra == 0)
				break;
	}
	slot = 0;
	time = 0;
	i = 0;
	for (n = 0; n < want; n++) {
		i++;
		time = ARG(args - 2, i);
		Schedule_setEntryTime(&ScheduleTable, npc, n, time);
		type = ARG(args - 3, i);
		Schedule_setEntryWorkType(&ScheduleTable, npc, n, type);
		x = ARG(args - 4, ++slot);
		y = ARG(args - 4, ++slot);
		region = 0;
		GetRegionPosition(&x, &y, &region);
		Schedule_setEntryPlace(&ScheduleTable, npc, n, x, y, region);
	}
}

extern "C" void ResetUccomm5Globals(void)
{
	RemoteViewActive = 0;
	MarkedX = 0;
	MarkedY = 0;
	MarkedZ = 0;
	UnusedMarkByte = 0;
}
