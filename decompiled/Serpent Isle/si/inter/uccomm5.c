/* Serpent Isle SI.EXE, overlay segment 320 (file offsets 0x08dcc0 to 0x08e6da, 2586 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

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

char UsecodeSpeechFileName[] = "u7speech.spc";  /* never referenced */
int RemoteViewActive = 0;
int MarkedX = 0, MarkedY = 0, MarkedZ = 0;      /* declared Coord where they are used */
char UnusedMarkByte = 0;

extern unsigned char far StrikeItemWithWeapon(objref actor, objref target, int weapon);

extern void far SetPolymorph(objref npc, unsigned shape);
extern void far RevertSchedule(int npc);
extern void far GetRegionPosition(int *x, int *y, int *region);

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x69: the first item attacks the second with the weapon shape in the third */
void far UC_AttackObject(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	int target = GetItemRef(GetListNode(args - 2, 1));
	objref r = obj;
	objref t = target;
	unsigned weapon = ARG(args - 3, 1);
	int type;

	CheckMojoBounds(1024L, (unsigned long) weapon);
	type = PeekWord(WeaponLookup.addr + weapon * 2);
	ret->appendInt(StrikeItemWithWeapon(r, t, type));
}

/* 0x7f: flash the cursor that says why something cannot be done */
void far UC_FlashMouse(Value *args, Value *)
{
	ReportNoCanDo(ARG(args - 1, 1));
}

/* 0x8c: fire a projectile from an item */
void far UC_FireProjectile(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	char dir = GetItemRef(GetListNode(args - 2, 1));
	int type = ARG(args - 3, 1);
	int speed = ARG(args - 4, 1);
	unsigned weapon = ARG(args - 5, 1);
	unsigned ammo = ARG(args - 6, 1);
	objref r = obj;
	objref from = obj;
	objref proj;
	int wtype, atype;

	CheckMojoBounds(1024L, (unsigned long) weapon);
	wtype = PeekWord(WeaponLookup.addr + weapon * 2);
	if (ammo == UC_ALL) {
		CheckMojoBounds(1024L, 0L);
		atype = PeekWord(AmmoLookup.addr);
	} else {
		CheckMojoBounds(1024L, (unsigned long) ammo);
		atype = PeekWord(AmmoLookup.addr + ammo * 2);
	}
	if (CreateItem(&proj, type, Item_getX(r), Item_getY(r), Item_getZ(&r)))
		FireMissileInDirection(proj, wtype, atype, r, speed, dir, 3);
}

/* 0x8e: how fast time passes */
void far UC_AdvanceTime(Value *args, Value *)
{
	TimeAdvanceRate = ARG(args - 1, 1);
}

/* 0x8d: the bed to nap in */
void far UC_NapTime(Value *args, Value *)
{
	NapBed = ARG(args - 1, 1);
}

/* 0x8a: start a speech track; the result says whether it played */
void far UC_StartSpeech(Value *args, Value *ret)
{
	int played = 0;
	int track = ARG(args - 1, 1);

	if ((char)SpeechPlayer.selectTrack(track)) {
		SpeechPlayer.playFile(SpeechFileName, track);
		played = 1;
	}
	ret->appendInt(played);
}

/* 0x90: report a crime to the guards */
void far UC_CallGuards(Value *, Value *)
{
	ReportCrime(0, 1, 0);
}

/* 0x92: set the guards on the avatar */
void far UC_AttackAvatar(Value *, Value *)
{
	SetGuardsOnAvatar();
}

/* 0x93: guards near the avatar give up their arrests */
void far UC_StopArrest(Value *, Value *)
{
	DismissRegionalGuards();
}

/* 0xa8: fade the palette in when the third argument is set, else out */
void far UC_FadePalette(Value *args, Value *)
{
	int ticks = ARG(args - 1, 1);
	int unused = ARG(args - 2, 1);

	if (ARG(args - 3, 1))
		FadeScreenIn(ticks, unused);
	else
		FadeScreenOut(ticks, unused);
}

/* 0xa9: as 0xa8, with the music of falling asleep and waking */
void far UC_FadeForSleep(Value *args, Value *)
{
	int ticks = ARG(args - 1, 1);
	int unused = ARG(args - 2, 1);

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
void far UC_InCombat(Value *, Value *ret)
{
	ret->appendInt(CombatGroups.battleMusic);
}

/* 0x55: summon a monster */
void far UC_Summon(Value *args, Value *)
{
	CombatGroups.spawnGroup(ARG(args - 1, 1), 1, 1,
		(unsigned char) (GetNpcBufferForIbo(&AvatarRef)->workType == WORK_COMBAT ? WORK_COMBAT : WORK_FOLLOW_AVT),
		10, 0, 0, 0, objref(0), 1);
}

/* 0xb3: make an NPC look like another shape */
void far UC_Polymorph(Value *args, Value *)
{
	objref npc = GetItemRef(GetListNode(args - 1, 1));
	int shape = ARG(args - 2, 1);

	if (npc.valid())
		SetPolymorph(npc, shape);
}

/* 0xb4: give an NPC back the schedules the game started with */
void far UC_RevertSchedule(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid()) {
		int npc = Item_getNpcNumber(&r);

		if (npc < 256)
			RevertSchedule(npc);
	}
}

/* 0xb7: put an NPC to its schedule for the time of day */
void far UC_Schedule(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	unsigned char npc = Item_getNpcNumber(&r);

	if (npc <= 255 && r.valid()) {
		unsigned char time = GameTime.getHour() / 3;
		unsigned char type = Schedule_getWorkType(&ScheduleTable, npc, time);

		if (type == 255)
			AssertFail("This npc, %d, has no schedules!", npc);
		Npc_setSchedule(&r, type);
	}
}

/* 0xb5: set what an NPC does from a time of day, and where */
void far UC_ChangeSchedule(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	unsigned char npc = Item_getNpcNumber(&r);

	if (!r.valid())
		return;
	unsigned char time = ARG(args - 2, 1);
	unsigned char type = ARG(args - 3, 1);
	int x = ARG(args - 4, 1);
	int y = ARG(args - 4, 2);
	int region = 0;

	if (npc > 255)
		return;
	unsigned char n = Schedule_findEntry(&ScheduleTable, npc, time);

	if (n == 255 && (int)Schedule_insertEntry(&ScheduleTable, npc))
		n = (unsigned char)(*(ScheduleTable.index + npc + 1) - ScheduleTable.index[npc]) - (unsigned char)1;
	Schedule_setEntryTime(&ScheduleTable, npc, n, time);
	Schedule_setEntryWorkType(&ScheduleTable, npc, n, type);
	GetRegionPosition(&x, &y, &region);
	Schedule_setEntryPlace(&ScheduleTable, npc, n, x, y, region);
}

/* 0xb6: replace an NPC's schedules: a list of times, a list of activities and a list of x, y pairs */
void far UC_NewSchedule(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	unsigned char npc, type;
	int x, y, region;
	unsigned char n;
	int i, have, want;
	unsigned char time;
	int extra;
	int slot;

	if (!r.valid())
		return;
	if (!r.isBody())
		return;
	if ((unsigned)Item_getNpcNumber(&r) > 255)
		return;
	npc = Item_getNpcNumber(&r);
	have = (unsigned char)(*(ScheduleTable.index + npc + 1) - ScheduleTable.index[npc]);
	want = LinkList_count(args - 2);
	if (want > have) {
		extra = want - have;
		for (i = 0; i < extra; i++)
			if (!Schedule_insertEntry(&ScheduleTable, npc))
				;       /* a full table is no error */
	} else if (want < have) {
		extra = have - want;
		for (i = 7; i >= 0; i--)
			if ((int)Schedule_removeEntry(&ScheduleTable, npc, i) && --extra == 0)
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
