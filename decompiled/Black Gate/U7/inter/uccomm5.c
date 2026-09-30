/* Black Gate U7.EXE, overlay segment 320 (file offsets 0x094160 to 0x09464e, 1262 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Original folder unknown.
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
#include "makemojo.h"
#include "misstrac.h"
#include "text.h"
#include "palctrl.h"
#include "palfade.h"
#include "item.h"
#include "sche.h"

extern objref AvatarRef;

char UsecodeSpeechFileName[] = "u7speech.spc";  /* never referenced */
int RemoteViewActive = 0;

extern unsigned char far StrikeItemWithWeapon(objref actor, objref target, int weapon);

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x54: the first item attacks the second with the weapon shape in the third */
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

/* 0x6a: flash the cursor that says why something cannot be done */
void far UC_FlashMouse(Value *args, Value *)
{
	ReportNoCanDo(ARG(args - 1, 1));
}

/* 0x76: fire a projectile from an item */
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

/* 0x78: how fast time passes */
void far UC_AdvanceTime(Value *args, Value *)
{
	TimeAdvanceRate = ARG(args - 1, 1);
}

/* 0x77: the bed to nap in */
void far UC_NapTime(Value *args, Value *)
{
	NapBed = ARG(args - 1, 1);
}

/* 0x74: start a speech track; the result says whether it played */
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

/* 0x7a: report a crime to the guards */
void far UC_CallGuards(Value *, Value *)
{
	ReportCrime(0, 1, 0);
}

/* 0x7c: set the guards on the avatar */
void far UC_AttackAvatar(Value *, Value *)
{
	SetGuardsOnAvatar();
}

/* 0x8c: fade the palette in when the third argument is set, else out */
void far UC_FadePalette(Value *args, Value *)
{
	int ticks = ARG(args - 1, 1);
	int unused = ARG(args - 2, 1);

	if (ARG(args - 3, 1)) {
		FadeScreenIn(ticks, unused);
		MarkPaletteFadedIn();
	} else {
		FadeScreenOut(ticks, unused);
		MarkPaletteFadedOut();
	}
}

/* 0x8e: whether combat music plays */
void far UC_InCombat(Value *, Value *ret)
{
	ret->appendInt(CombatGroups.battleMusic);
}

/* 0x8f: as 0x74, waiting for the speech to end */
void far UC_StartBlockingSpeech(Value *args, Value *ret)
{
	int track = ARG(args - 1, 1);

	if ((char)SpeechPlayer.selectTrack(track)) {
		SpeechPlayer.playFile(SpeechFileName, track);
		while (SpeechPlayer.isPlaying())
			SpeechPlayer.continuePlaying();
		ret->appendInt(1);
	} else
		ret->appendInt(0);
}

/* 0x47: summon a monster */
void far UC_Summon(Value *args, Value *)
{
	CombatGroups.spawnGroup(ARG(args - 1, 1), 1, 1,
		(unsigned char) (GetNpcBufferForIbo(&AvatarRef)->workType == WORK_COMBAT ? WORK_COMBAT : WORK_FOLLOW_AVT),
		10, 0, 0, 0, objref(0), 1);
}
