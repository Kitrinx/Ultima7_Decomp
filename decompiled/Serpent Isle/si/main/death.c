/* Serpent Isle SI.EXE, overlay segment 356 (file offsets 0x0b0330 to 0x0b1b36, 6150 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: death.c */
#include "activity.h"
#include "death.h"
#include "lowlevel.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "combatai.h"
#include "init.h"
#include "actqueue.h"
#include "barge.h"
#include "bogus.h"
#include "cheat.h"
#include "itemovr1.h"
#include "makemojo.h"
#include "monsters.h"
#include "party.h"
#include "search.h"
#include "special.h"
#include "random.h"
#include "damage.h"
#include "script.h"
#include "bodies.h"
#include "cullmask.h"
#include "type.h"
#include "daze.h"
#include "palfade.h"
#include "voolook.h"
#include "ready.h"
#include "weapons.h"
#include "armor.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)
#define FLIPPED(rec) ((char) (((rec)->typeFrame & 0x8000) == 0x8000))
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define IS_NPC(rec) ((unsigned char) ((CLASS_FLAGS(rec) & CLASS_NPC) != 0))

#define NPC(p) GetNpcBufferForIbo(p)
#define NPC_FLAG(n, bit) ((unsigned char) (((n)->status & (bit)) != 0))
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* 10-byte armour records */
struct ArmorRecord {
	int type;
	char protection;
	char unusedTail[7];
};

extern int DeadPartyCount;
extern objref DeadPartyMembers[];

extern int far PlaceItem(objref *, Loc, Loc, char);
extern unsigned char far IsTypeBlockedAt(CellCoord x, CellCoord y, int z, TypeFrame typeFrame);
extern unsigned char far ShapeCrossesChunkX(TypeFrame far &, unsigned char);
extern unsigned char far ShapeCrossesChunkY(TypeFrame far &, unsigned char);
extern unsigned char IsSpeechPlaying(void);

/* a record number looked up by type; zero means none */
inline unsigned char IsNone(int type)
{
	return type == 0;
}

/* type number of an item handed over by value */
inline unsigned GetType(objref r)
{
	return TYPE(ITEM(r.off));
}

/* nothing stops a body of this shape standing at (x, y, z) */
#define ROOM(x, y, z, shape) (!IsTypeBlockedAt(x, y, z, shape) && !IsShapeOccluded(shape, x, y, z, 0) && \
	!ShapeCrossesChunkX(shape, (x) & 0xff) && !ShapeCrossesChunkY(shape, (y) & 0xff))

/* Adds half of amount to an NPC's experience, at most 999999, and three training
 * points for each doubling of 100 the total passes. */
void far AwardExperience(objref npc, long amount)
{
	long old;
	long now;
	long level;
	int tp;

	if (IS_NPC(ITEM(npc.off)) && amount > 0) {
		old = NPC(&npc)->experience;
		now = old + (amount >> 1);
		if (now > 999999L)
			now = 999999L;
		NPC(&npc)->experience = now;
		tp = NPC(&npc)->training;
		for (level = 100; level != 0 && level <= old; level <<= 1)
			;
		for (; level != 0 && level <= now; level <<= 1)
			tp += 3;
		if (tp > 255)
			tp = 255;
		NPC(&npc)->training = tp;
	}
}

/* An NPC's fighting strength: its attributes, the traits of its monster record and
 * the weapons and armour it carries; never less than 1. */
int far GetCombatStrength(NPCRef who, int type)
{
	int t = type;
	int wtype;
	int atype;
	MonsterRecord m;
	AreaSearch found;
	WeaponRecord wr;
	int score;
	int bits;

	if (IsNone(t))
		t = MonsterLookup.get(TYPE(ITEM(who.off)));
	MonsterRecords.read(t, &m);
	score = (unsigned char) ((NPC(&who)->strength & 31) + (NPC(&who)->combat & 31)) + (NPC(&who)->dexterity + 1) / 3 +
		(unsigned char) (NPC(&who)->intelligence & 31) / 5;
	score += m.sleepSafe + m.charmSafe + m.paralysisSafe + m.poisonSafe + m.intFlag + m.deathSafe + m.fly + m.swim +
		m.armor + m.damage + (m.range > 3) + (m.range > 5) + (m.powerSafe + m.startInvisible) * 8 +
		(m.ethereal + m.splits + m.seeInvisible) * 2 + m.curseSafe;
	for (bits = m.immune; bits != 0; bits >>= 1)
		if (bits & 1)
			score++;
	for (bits = m.vulnerable; bits != 0; bits >>= 1)
		if (bits & 1)
			score--;
	FindItemInContainer(&found, who, 1, -1, 255, 255);
	while (found.found()) {
		wtype = WeaponLookup.get(TYPE(ITEM(found.current.off)));
		if (!IsNone(wtype)) {
			WeaponRecords.read(wtype, &wr);
			score += wr.damage + wr.explodes + wr.passesBlockers + wr.autoHit * 10 +
				(wr.sleep + wr.charm + wr.paralyze + wr.noDamage) * 2 +
				(wr.curse + wr.poison + wr.drainMana + wr.drainHealth) * 2;
			switch ((unsigned char) wr.uses) {
			case 0:
				score += (wr.range > 3) + (wr.range > 5);
				break;
			case 1:
				score += (unsigned char) (NPC(&who)->combat & 31) / 5;
				break;
			case 2:
				score += (unsigned char) (NPC(&who)->combat & 31) / 3;
				break;
			case 3:
				score += wr.range / 2;
				break;
			}
		}
		atype = ArmorLookup.get(TYPE(ITEM(found.current.off)));
		if (!IsNone(atype))
			score += ArmorRecords.get(atype)->protection;
		FindItem(&found);
	}
	if (score <= 0)
		score = 1;
	return score;
}

/* Knocks an NPC out (asleep) and returns its strength, or 0 when it cannot fall or is dead. */
int far KnockOut(NPCRef who, char hp)
{
	int type;

	type = MonsterLookup.get(TYPE(ITEM(who.off)));
	if ((char) MonsterRecords.get(type)->cantDie || NPC_FLAG(NPC(&who), NPC_DEAD))
		return 0;
	if ((unsigned char)Item_isAvatar(&who) && PowerAvatar)
		return 0;
	if (NPC_FLAG(NPC(&who), NPC_ASLEEP) && (char)Item_getHitPoints(&who) <= 0) {
		Item_setHitPoints(&who, hp);
		return 0;
	}
	LayDown(who, 0);
	Item_setHitPoints(&who, hp);
	NPC(&who)->changeFlags(0, NPC_ASLEEP);
	if ((unsigned char) (NPC(&who)->typeFlags & NPC_FLY)) {
		if (FallToGround(who))
			ReduceHealth(who, GenerateRandomIntegerInRange(5) + 1, 0, 0);
	}
	return GetCombatStrength(who, 0);
}

/* An NPC dies: a body of the matching shape takes its place nearby, its belongings go
 * into the body or to the ground, and a fallen party member joins the dead list.
 * Returns its strength. */
int far KillNpc(objref target, int attacker)
{
	AreaSearch contentsSearch;
	NPCRef who = target;
	char hp;
	unsigned char strangeMover;
	unsigned char lying;
	int result;
	int type;

	if (!IS_NPC(ITEM(target.off))) {
		Item_delete(&target);
		return 0;
	}
	if (NPC_FLAG(NPC(&who), NPC_DEAD))
		return 0;
	hp = (char)Item_getHitPoints(&who);
	AreaSearch spellSearch;
	objref body = 0;
	strangeMover = gItemTypeInfo[TYPE(ITEM(who.off))].strangeMovement;
	type = MonsterLookup.get(TYPE(ITEM(target.off)));
	if (NPC_FLAG(NPC(&who), NPC_ASLEEP) && (char)Item_getHitPoints(&who) <= 0)
		result = 0;
	else
		result = GetCombatStrength(target, 0);
	if (strangeMover)
		lying = IsInSleepFrame(who);
	else
		lying = FRAME(ITEM(who.off)) == 13 || FRAME(ITEM(who.off)) == 29;
	if (FindItemInContainer(&spellSearch, who, 0, -1, 255, 255)) {
		while (spellSearch.found()) {
			objref spell;

			if ((char) ReadyRecords.get(ReadyLookup.get(TYPE(ITEM(spellSearch.current.off))))->spell)
				spell = spellSearch.current;
			else
				spell = 0;
			FindItem(&spellSearch);
			if (spell.valid())
				Item_delete(&spell);
		}
	}
	NPC(&who)->changeFlags(0, NPC_DEAD);
	if ((char) MonsterRecords.get(type)->noBody) {
		if (who == AvatarRef)
			AssertFail(__FILE__, 430);
		body = 0;
	} else {
		unsigned npcnum;
		unsigned char quality;
		unsigned char quantity;
		unsigned bodyType = GetBodyType(GetType(who));
		Coord x, y, x0, y0;
		int z;

		if (FLIPPED(ITEM(who.off)))
			bodyType |= 0x8000;
		x0 = x = Item_getX(who);
		y0 = y = Item_getY(who);
		z = GetItemZAndStuff(&who).z();
		Item_detach(&who);
		if (FindBargeUnder(who).valid())
			goto place;
		if (ROOM(x, y, z, bodyType))
			goto place;
		y = Coord(Coord(y0 - GetFootprintY(ITEM(who.off)->asTypeFrame())) + GetFootprintY(*(TypeFrame _ss *)&bodyType));
		if (ROOM(x, y, z, bodyType))
			goto place;
		x = Coord(Coord(x0 - GetFootprintX(ITEM(who.off)->asTypeFrame())) + GetFootprintX(*(TypeFrame _ss *)&bodyType));
		if (ROOM(x, y, z, bodyType))
			goto place;
		y = y0;
		if (ROOM(x, y, z, bodyType))
			goto place;
		x = x0;
	place:
		PlaceItem(&who, x0, y0, z);
		CreateItem(&body, bodyType, x, y, z);
		npcnum = Item_getNpcNumber(&who);
		quantity = npcnum & 0x7f;
		quality = (npcnum & 0x180) >> 7;
		ItemInfo info;
		info = GetItemZAndStuff(&who);
		if ((unsigned char) (info.flags & 8)) {
			quality += 4;
			Item_setTemporary(&body);
		}
		Item_setQuality(&body, quality);
		Item_storeQuantity(&body, quantity);
		Item_setOkayToTake(&body);
	}
	if (!(unsigned char)Item_isAvatar(&who)) {
		if (body.valid() && (char) (CLASS_FLAGS(ITEM(body.off)) & CLASS_CONTENTS)) {
			FindItemInContainer(&contentsSearch, who, 0, -1, 255, 255);
			while (contentsSearch.found()) {
				Item_setOkayToTake(&contentsSearch.current);
				FindItem(&contentsSearch);
			}
			MoveContents(&who, &objref(body.off));
		} else {
			FindItemInContainer(&contentsSearch, who, 0, -1, 255, 255);
			while (contentsSearch.found()) {
				Item_setOkayToTake(&contentsSearch.current);
				FindItem(&contentsSearch);
			}
			Item_spillContents(&who);
		}
	}
	int bodyFrame = GetBodyFrame(GetType(who));
	if (!lying) {
		LayDown(who, 1);
		if (who != AvatarRef && body.valid())
			ActionQueue.add(4, body, MakeScript(SCRIPT_FRAME, bodyFrame, SCRIPT_END));
	} else {
		ActionQueue.remove(who, 1, 0);
		ActionQueue.add(who, MakeScript(SCRIPT_REMOVE, SCRIPT_END));
		if (who != AvatarRef && body.valid())
			ActionQueue.add(body, MakeScript(SCRIPT_FRAME, bodyFrame, SCRIPT_END));
	}
	/* the braces are needed to reproduce the shipped code */
	if ((unsigned char) (NPC(&who)->typeFlags & NPC_FLY)) {
		FallToGround(who);
	}
	HandleNpcHit(attacker, who, hp);
	if (!(who != AvatarRef && NPC_FLAG(NPC(&who), NPC_IN_PARTY) && DeadPartyCount < 12))
		goto done;
	DeadPartyMembers[DeadPartyCount++] = objref(who);
	LeaveParty(&who);
done:
	return result;
}

/* Sets down beside an item what a 0x102 search of its contents turns up, where it fits. */
void far RefitInventory(objref item)
{
	AreaSearch found;

	while (FindItemInContainer(&found, item, 0x102, -1, 255, 255)) {
		Item_detach(&found.current);
		if (TryToPlaceItem(item, 0, 1)) {
			GetItemBeingDragged(&found.current);
			PlaceItem(&found.current, Item_getX(item), Item_getY(item), GetItemZAndStuff(&item).z());
		}
	}
}

/* The NPC number a body's quality and quantity stand for, or 0. */
int far GetBodyNpc(objref body)
{
	objref npc;
	unsigned char quality;
	unsigned char quantity;
	int n;

	quality = (unsigned char)Item_getQuality(&body);
	quantity = (unsigned char)Item_getQuantity(&body);
	n = (quality & 3) * 0x80 + (quantity & 0x7f);
	GetNpcIbo(&npc, n);
	if (n == 1 && (TYPE(ITEM(body.off)) != 414 || FRAME(ITEM(body.off)) != 20))
		return 0;
	if (quality & 4 || !npc.valid() || n == 0 || FindBargeUnder(AvatarRef).valid())
		return 0;
	return n;
}

/* Raises the NPC a body stands for at (x, y, z) and takes it off the dead list;
 * 0 when the body stands for nobody. */
char far ResurrectBody(objref body, Coord x, Coord y, int z)
{
	objref who;
	int n;
	int i = 0;

	n = GetBodyNpc(body);
	if (n == 0) {
		Item_delete(&body);
		return 0;
	}
	GetNpcIbo(&who, n);
	MoveContents(&body, &objref(who.off));
	RefitInventory(who);
	NPC(&who)->changeFlags(NPC_DEAD, 0);
	NPC(&who)->changeFlags(NPC_ASLEEP, 0);
	NPC(&who)->changeFlags(NPC_CHARMED, 0);
	NPC(&who)->changeFlags(NPC_CURSED, 0);
	NPC(&who)->changeFlags(NPC_PARALYZED, 0);
	NPC(&who)->changeFlags(NPC_POISONED, 0);
	NPC(&who)->changeFlags(NPC_PROTECTED, 0);
	Item_setQualityFlags(&who, Item_getQualityFlags(&who) & 0xfe);
	Item_setHitPoints(&who, NPC(&who)->strength & 31);
	Item_move(&who, x, y, z);
	Item_delete(&body);
	Npc_setSchedule(&who, WORK_LOITER);
	CUR_SCHED(&who).state = 1;
	ActionQueue.remove(who, 1, 0);
	ActionQueue.add(who, MakeScript(SCRIPT_LIE_FRAME, SCRIPT_KNEEL_FRAME, SCRIPT_STAND_FRAME, SCRIPT_END));
	for (i = 0; i < 12; i++) {
		if (DeadPartyMembers[i] == who) {
			for (; i < 11; i++)
				DeadPartyMembers[i] = DeadPartyMembers[i + 1];
			DeadPartyMembers[11] = 0;
			DeadPartyCount--;
			AddToParty(who, 0);
			break;
		}
	}
	return 1;
}

/* The dead list and its length. */
int far GetDeadPartyList(objref **list)
{
	*list = DeadPartyMembers;
	return DeadPartyCount;
}

/* Empties the dead list. */
void far ClearDeadPartyList(void)
{
	DeadPartyCount = 0;
}
