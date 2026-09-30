/* Black Gate U7.EXE, overlay segment 320 (file offsets 0x094160 to 0x09464e, 1262 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "plat.h"
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

const char UsecodeSpeechFileName[] = "u7speech.spc";  /* never referenced */
int16_t RemoteViewActive = 0;

extern uint8_t StrikeItemWithWeapon(objref actor, objref target, int16_t weapon);

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x54: the first item attacks the second with the weapon shape in the third */
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

/* 0x6a: flash the cursor that says why something cannot be done */
void UC_FlashMouse(Value *args, Value *)
{
	ReportNoCanDo(ARG(args - 1, 1));
}

/* 0x76: fire a projectile from an item */
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

/* 0x78: how fast time passes */
void UC_AdvanceTime(Value *args, Value *)
{
	TimeAdvanceRate = ARG(args - 1, 1);
}

/* 0x77: the bed to nap in */
void UC_NapTime(Value *args, Value *)
{
	NapBed = ARG(args - 1, 1);
}

/* 0x74: start a speech track; the result says whether it played */
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

/* 0x7a: report a crime to the guards */
void UC_CallGuards(Value *, Value *)
{
	ReportCrime(0, 1, 0);
}

/* 0x7c: set the guards on the avatar */
void UC_AttackAvatar(Value *, Value *)
{
	SetGuardsOnAvatar();
}

/* 0x8c: fade the palette in when the third argument is set, else out */
void UC_FadePalette(Value *args, Value *)
{
	int16_t ticks = ARG(args - 1, 1);
	int16_t unused = ARG(args - 2, 1);

	if (ARG(args - 3, 1)) {
		FadeScreenIn(ticks, unused);
		MarkPaletteFadedIn();
	} else {
		FadeScreenOut(ticks, unused);
		MarkPaletteFadedOut();
	}
}

/* 0x8e: whether combat music plays */
void UC_InCombat(Value *, Value *ret)
{
	ret->appendInt(CombatGroups.battleMusic);
}

/* 0x8f: as 0x74, waiting for the speech to end */
void UC_StartBlockingSpeech(Value *args, Value *ret)
{
	int16_t track = ARG(args - 1, 1);

	if ((int8_t)SpeechPlayer.selectTrack(track)) {
		SpeechPlayer.playFile(SpeechFileName, track);
		while (SpeechPlayer.isPlaying()) {
			SpeechPlayer.continuePlaying();
			plat_yield();
		}
		ret->appendInt(1);
	} else
		ret->appendInt(0);
}

/* 0x47: summon a monster */
void UC_Summon(Value *args, Value *)
{
	CombatGroups.spawnGroup(ARG(args - 1, 1), 1, 1,
		(uint8_t) (GetNpcBufferForIbo(&AvatarRef)->workType == WORK_COMBAT ? WORK_COMBAT : WORK_FOLLOW_AVT),
		10, 0, 0, 0, objref(0), 1);
}

extern "C" void ResetUccomm5Globals(void)
{
	RemoteViewActive = 0;
}
