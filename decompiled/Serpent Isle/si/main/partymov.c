/* Serpent Isle SI.EXE, resident segment 26 (file offsets 0x01671a to 0x018d52, 9784 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "objref.h"
#include "typefram.h"
#include "activity.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "u7event.h"
#include "combat.h"
#include "collide.h"
#include "crime.h"
#include "legalmov.h"
#include "party.h"
#include "use.h"
#include "itable.h"
#include "npcpath.h"
#include "movepath.h"
#include "coord.h"
#include "npcref.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define NPC(p) GetNpcBufferForIbo(p)
#define ITEM(r) ((ItemRecord far *)MK_FP(ItemBufferSegment, (r).off))

/* PartyMemberFlags bits. */
#define MEMBER_STEPPED  1   /* moved this turn */
#define MEMBER_LOST     2   /* could not keep up with its leader */
#define MEMBER_ON_PATH  4   /* walking a route back to the avatar */

/* Where a party member can be set down. */
struct Position { Coord x, y; char z; };

/* A party member's step, run as a script on it. */
struct StepScript {
	char kind, code;
	char moved;
	char direction;
	StepScript(int m, char d) { moved = m; direction = d; }
};

void far RemovePartyFromCollision(void);
void far AddPartyToCollision(void);

extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern unsigned char far Item_forceMove(objref *, Loc, Loc, int);
extern unsigned char far Item_forceMove(objref *, unsigned char, int);
extern unsigned char far Item_getQualityFlags(objref *);
extern int far IsWorldPosOnScreen(Coord a, Coord b);
extern unsigned char far Item_move(objref *, Loc, Loc, int);
extern void far Item_move(objref *, unsigned char, int);

char FormationFacing = 0;
char FormationSize = 0;
/* Formation offsets per party size: x, y, and the member each one follows. */
char FormationSlotX[8][8] = {
	{ 0 },
	{ 0, -2 },
	{ 0, -2, 2 },
	{ 0, -2, 2, 0 },
	{ 0, -2, 2, -1, 1 },
	{ 0, -2, 2, -2, 2, 0 },
	{ 0, -2, 2, -1, 1, -2, 2 },
	{ 0, -2, 2, 0, -3, 3, -2, 2 }
};
char FormationSlotY[8][8] = {
	{ 0 },
	{ 0, 2 },
	{ 0, 2, 2 },
	{ 0, 2, 2, 4 },
	{ 0, 2, 2, 4, 4 },
	{ 0, 2, 2, 4, 4, 6 },
	{ 0, 2, 2, 4, 4, 6, 6 },
	{ 0, 2, 2, 4, 4, 4, 6, 6 }
};
char FormationLeaders[8][8] = {
	{ 0 },
	{ 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0, 1 },
	{ 0, 0, 0, 1, 2 },
	{ 0, 0, 0, 1, 2, 3 },
	{ 0, 0, 0, 1, 2, 3, 4 },
	{ 0, 0, 0, 1, 1, 2, 3, 3 }
};
/* Pairs of members that may trade places, ended by 0. */
char FormationSwapPairs[8][8] = {
	{ 0 },
	{ 0 },
	{ 1, 2 },
	{ 1, 2 },
	{ 1, 2, 3, 4 },
	{ 1, 2, 3, 4 },
	{ 1, 2, 3, 4, 5, 6 },
	{ 1, 2, 4, 5, 6, 7 }
};
char DirectionDX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
char DirectionDY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
char DirectionBySign[3][3] = { { 7, 6, 5 }, { 0, -1, 4 }, { 1, 2, 3 } };
/* DirectionBySign row for each direction. */
char DirectionSignRow[8] = { 0, 0, 1, 2, 2, 2, 1, 0 };

NPCRef PartyMembers[8];
NPCRef DownedPartyMembers[16];
char PartySize = 0;
char DownedPartyCount = 0;
char PartyFacing = 0;
unsigned char PartyMemberFlags[8] = { 0 };
char PartyFollowTicks = 0;
char AvatarIdleSteps = 0;

inline char GetFormationLeader(char n) { return FormationLeaders[FormationSize - 1][n]; }

/* Formation slot offsets for the current party size. */
#define SLOT_X(n) FormationSlotX[FormationSize - 1][n]
#define SLOT_Y(n) FormationSlotY[FormationSize - 1][n]
/* The larger of the x and y distances. */
#define DISTANCE(x1, y1, x2, y2) MAX(GetMagnitude(GetDelta(x1, x2)), GetMagnitude(GetDelta(y1, y2)))
#define AHEAD(n) (&PartyMembers[GetFormationLeader(n)])

char far CanReachLeader(Coord x, Coord y, char z, char member, Coord leaderX, Coord leaderY, char leaderZ);

char far GetSwapPair(int pair, char *first, char *second)
{
	if (FormationSwapPairs[FormationSize - 1][pair * 2] == 0)
		return 0;
	*first = FormationSwapPairs[FormationSize - 1][pair * 2];
	*second = FormationSwapPairs[FormationSize - 1][pair * 2 + 1];
	return 1;
}

void far RotateFormation(unsigned char turn)
{
	int i, x;

	switch (turn) {
	case 3:
		for (i = 0; i < 8; i++) {
			SLOT_X(i) = -SLOT_X(i);
			SLOT_Y(i) = -SLOT_Y(i);
		}
		FormationFacing = (FormationFacing + 4) & 7;
		break;
	case 1:
		for (i = 0; i < 8; i++) {
			x = SLOT_X(i);
			SLOT_X(i) = -SLOT_Y(i);
			SLOT_Y(i) = x;
		}
		FormationFacing = (FormationFacing + 2) & 7;
		break;
	case 2:
		for (i = 0; i < 8; i++) {
			x = SLOT_X(i);
			SLOT_X(i) = SLOT_Y(i);
			SLOT_Y(i) = -x;
		}
		FormationFacing = (FormationFacing + 6) & 7;
		break;
	}
}

char far TurnFormation(char facing, char turn)
{
	int facing2, steps, target;

	if (turn > 4)
		turn = turn - 8;
	else if (turn < -4)
		turn = turn + 8;
	facing2 = (facing + turn + 8) & 7;
	if (FormationFacing == facing2)
		return 0;
	if ((facing & 1) == 0)
		steps = GetMagnitude(turn);
	else {
		if (facing2 == 0 && FormationFacing > 4)
			facing2 = 8;
		target = FormationFacing;
		if (FormationFacing == 0 && facing2 > 4)
			target = 8;
		steps = GetMagnitude(target - facing2);
	}
	switch (steps) {
	case 2:
		if (GetSign(turn) == -1) {
			RotateFormation(2);
			return 2;
		}
		RotateFormation(1);
		return 1;
	case 3:
	case 4:
	case 5:
		RotateFormation(3);
		return 3;
	}
}

void far SeparateDownedMembers(void)
{
	int i;

	for (i = 1; i < PartySize; i++)
		if (IsNpcUnconscious(&PartyMembers[i])) {
			DeactivatePartyMember(i);
			i--;
		}
	for (i = 0; i < DownedPartyCount; i++)
		if (!IsNpcUnconscious(&DownedPartyMembers[i])) {
			ReactivatePartyMember(i);
			i--;
		}
}

void far SwapPartyOrder(char first, char second)
{
	NPCRef member = PartyMembers[first];

	PartyMembers[first] = PartyMembers[second];
	PartyMembers[second] = member;
}

char far CanSwapMembers(char first, char second)
{
	objref a = PartyMembers[first];
	objref b = PartyMembers[second];
	objref c = PartyMembers[FormationLeaders[FormationSize - 1][second]];
	objref d = PartyMembers[FormationLeaders[FormationSize - 1][first]];

	if (CanReachLeader(Item_getX(a), Item_getY(a), Item_getZ(&a), first,
		Item_getX(c), Item_getY(c), Item_getZ(&c)) == 1
		&& CanReachLeader(Item_getX(b), Item_getY(b), Item_getZ(&b), second,
			Item_getX(d), Item_getY(d), Item_getZ(&d)) == 1)
		return 1;
	return 0;
}

void far ReorderParty(void)
{
	char first, second;
	int pair = 0;

	while (GetSwapPair(pair++, &first, &second))
		if (CanSwapMembers(first, second))
			SwapPartyOrder(first, second);
}

char far FindMemberInDirection(Coord x, Coord y, char member, char direction)
{
	CellCoord tx = x + DirectionDX[direction];
	CellCoord ty = y + DirectionDY[direction];
	char i;

	for (i = 0; i < PartySize; i++) {
		if (i == member)
			continue;
		if (Item_getX(PartyMembers[i]) == tx && Item_getY(PartyMembers[i]) == ty)
			return i;
	}
	return -1;
}

void far SwapMemberPositions(char first, char second)
{
	objref a = PartyMembers[first];
	objref b = PartyMembers[second];
	Coord x = Item_getX(a);
	Coord y = Item_getY(a);
	char z = Item_getZ(&a);

	Item_forceMove(&a, Item_getX(b), Item_getY(b), Item_getZ(&b));
	Item_forceMove(&b, x, y, z);
}

/* Can this member step next to its leader? 1 when already beside it, otherwise
 * the result of trying the eight squares around it. */
char far CanReachLeader(Coord x, Coord y, char z, char member, Coord leaderX, Coord leaderY, char leaderZ)
{
	unsigned flags;
	char dir;
	char other;
	char result = 0;
	unsigned shape = ITEM(PartyMembers[member])->typeFrame;
	unsigned char typeFlags = GetNpcBufferForIbo(&PartyMembers[member])->typeFlags;

	switch (DISTANCE(x, y, leaderX, leaderY)) {
	case 1:
		if (GetMagnitude(leaderZ - z) <= 1)
			return 1;
		break;
	case 2:
		for (dir = 0; dir < 8; dir++) {
			if (DISTANCE(Coord(x + DirectionDX[dir]), Coord(y + DirectionDY[dir]), leaderX, leaderY) != 1)
				continue;
			other = FindMemberInDirection(x, y, member, dir);
			if (other != -1 && other <= member)
				continue;
			flags = CheckMove(x, y, z, shape, dir, 1, typeFlags);
			if (GetMagnitude((char) flags + z - leaderZ) > 1 || !(flags & 0x8000))
				continue;
			if (flags & 0x2000)
				result = 0;
			else if (flags & 0x4000)
				result = 2;
			else
				return 1;
		}
		return result;
	}
	return 0;
}

void far StepAroundMember(char first, char member, char avoid)
{
	PartyMemberFlags[first] |= MEMBER_STEPPED;
	PartyMemberFlags[member] |= MEMBER_STEPPED;
	objref ref = PartyMembers[member];
	Coord x = Item_getX(ref);
	Coord y = Item_getY(ref);
	char z = Item_getZ(&ref);
	char head = FormationLeaders[FormationSize - 1][member];
	Coord tx = Item_getX(PartyMembers[head]) + (SLOT_X(member) - SLOT_X(head));
	Coord ty = Item_getY(PartyMembers[head]) + (SLOT_Y(member) - SLOT_Y(head));
	char direction = DirectionBySign[GetSign(GetDelta(tx, x)) + 1][GetSign(GetDelta(ty, y)) + 1];
	const TypeFrame shape = ITEM(PartyMembers[member])->typeFrame;
	unsigned char flags = GetNpcBufferForIbo(&PartyMembers[member])->typeFlags;

	for (char i = 0; i < 8; i++) {
		char d = (direction + i + 8) & 7;
		if (d == avoid)
			continue;
		unsigned result = CheckMove(x, y, z, shape, d, 1, flags);
		if ((result & 0xc000) && !(result & 0x2000) && FindMemberInDirection(x, y, member, d) == -1) {
			Item_forceMove(&PartyMembers[member], d, (char) result);
			unsigned other = CheckMove(Item_getX(PartyMembers[first]),
				Item_getY(PartyMembers[first]), Item_getZ(&PartyMembers[first]),
				TypeFrame(ITEM(PartyMembers[first])->typeFrame), avoid, 1,
				GetNpcBufferForIbo(&PartyMembers[first])->typeFlags);
			Item_forceMove(&PartyMembers[first], avoid, (char) other);
			return;
		}
	}
	SwapMemberPositions(first, member);
}

/* Picks the step that brings a party member closest to its formation slot beside `head`.
 * Returns what CanReachLeader said of the chosen square (0 when none), and reports the direction
 * (-1 to stay), the z change, a member to swap with, and the distance to the slot. */
char far ChooseFollowStep(char member, char head, char direction, char *outDir, char *outDz,
	char *outOther, int *outDist)
{
	Coord mx, my, cx, cy, hx, hy, tx, ty, nx, ny;
	char other;
	int candHead, candScore, bestHead, bestScore, dir;
	char found;
	int a, b, better;   /* a, b: distances, later the "blocked" tests */
	unsigned char reach;
	char best;
	unsigned flags;
	int bestSum, candSum, candDist;
	char memberZ, headZ;
	objref m, h;

	m = PartyMembers[member];
	h = PartyMembers[head];
	mx = Item_getX(m);
	my = Item_getY(m);
	memberZ = Item_getZ(&m);
	hx = Item_getX(h);
	hy = Item_getY(h);
	headZ = Item_getZ(&h);
	tx = hx + (SLOT_X(member) - SLOT_X(head));
	ty = hy + (SLOT_Y(member) - SLOT_Y(head));
	if (direction != -1) {
		nx = hx + DirectionDX[direction] + (SLOT_X(member) - SLOT_X(head));
		ny = hy + DirectionDY[direction] + (SLOT_Y(member) - SLOT_Y(head));
	}
	*outOther = 0;
	if (DISTANCE(mx, my, hx, hy) > 3 || GetMagnitude(memberZ - headZ) > 3) {
		*outDir = -1;
		return 0;
	}
	reach = CanReachLeader(mx, my, memberZ, member, hx, hy, headZ);
	if (reach != 0) {
		found = 1;
		bestHead = DISTANCE(mx, my, hx, hy);
		a = DISTANCE(mx, my, tx, ty);
		b = DISTANCE(mx, my, Item_getX(*AHEAD(member)), Item_getY(*AHEAD(member)));
		*outDist = a;
		if (a > 0)
			bestScore = a + b;
		else
			bestScore = 0;
		*outDir = -1;
		best = reach;
		bestSum = GetMagnitude(GetDelta(tx, mx)) + GetMagnitude(GetDelta(ty, my));
		/* already beside the slot the head is walking toward: stay */
		if (reach == 1 && headZ == memberZ && direction != -1) {
			a = DISTANCE(mx, my, tx, ty);
			b = DISTANCE(mx, my, nx, ny);
			if (b <= a)
				return reach;
		}
	} else {
		found = 0;
		bestHead = 999;
		bestScore = 999;
		bestSum = 999;
		best = 2;
		*outDir = -1;
	}
	unsigned shape = ITEM(m)->typeFrame;
	unsigned char typeFlags = NPC(&m)->typeFlags;
	for (dir = 0; dir < 8; dir++) {
		flags = CheckMove(mx, my, memberZ, shape, dir, 1, typeFlags);
		if (!(flags & 0xc000))
			continue;
		if (flags & 0x2000)
			continue;
		other = FindMemberInDirection(mx, my, member, dir);
		if (other != -1 && other <= member)
			continue;
		cx = mx + DirectionDX[dir];
		cy = my + DirectionDY[dir];
		if (!(reach = CanReachLeader(cx, cy, memberZ + (char) flags, member, hx, hy, headZ)))
			continue;
		found = 1;
		if ((flags & 0x4000) && !(flags & 0x2000))
			reach = 2;
		a = DISTANCE(cx, cy, tx, ty);
		b = DISTANCE(cx, cy, Item_getX(*AHEAD(member)), Item_getY(*AHEAD(member)));
		candDist = a;
		if (a > 0)
			candScore = a + b;
		else
			candScore = 0;
		candHead = DISTANCE(cx, cy, hx, hy);
		candSum = GetMagnitude(GetDelta(tx, cx)) + GetMagnitude(GetDelta(ty, cy));
		a = best == 2;
		b = reach == 2;
		better = candScore <= bestScore;
		if ((a && b && better) || (a && !b) || (!a && !b && better)) {
			if (candScore != bestScore || candHead < bestHead || candHead != bestHead ||
				(a && !b) || candSum <= bestSum) {
				bestScore = candScore;
				bestHead = candHead;
				best = reach;
				bestSum = candSum;
				*outDir = dir;
				*outDz = flags;
				*outDist = candDist;
				if (other > member)
					*outOther = other;
				else
					*outOther = 0;
			}
		}
	}
	if (found)
		return best;
	return 0;
}

char far MoveParty(char direction)
{
	char facing = PartyFacing;
	char bestDir, tryDir, altDir;
	Coord x, y;
	char head, blocker, swap;
	int i, j;
	char tried, result;
	unsigned char found;
	int tryDist, bestDist;
	char bestDz, tryDz;
	int passes;
	unsigned flags;
	int pass;

	x = Item_getX(PartyMembers[0]);
	y = Item_getY(PartyMembers[0]);
	unsigned shape = ITEM(PartyMembers[0])->typeFrame;

	SeparateDownedMembers();
	RemovePartyFromCollision();
	char z = Item_getZ(&AvatarRef);
	objref who;

	/* the avatar steps first, veering one point either way around an obstacle */
	if (direction != -1) {
		passes = 2;
		flags = CheckMove(x, y, z, TypeFrame(shape), direction, 1, NPC(&AvatarRef)->typeFlags);
		if (!(flags & 0x8000) || (flags & 0x2000)) {
			flags = CheckMove(x, y, z, TypeFrame(shape), (direction + 1) & 7, 1, NPC(&AvatarRef)->typeFlags);
			if (!(flags & 0x8000) || (flags & 0x2000)) {
				flags = CheckMove(x, y, z, TypeFrame(shape), (direction + 7) & 7, 1, NPC(&AvatarRef)->typeFlags);
				if (!(flags & 0x8000) || (flags & 0x2000)) {
					AddPartyToCollision();
					return flags;
				}
				direction--;
			} else
				direction++;
		}
		direction &= 7;
		if (direction != facing && TurnFormation(facing, direction - facing) == 3)
			ReorderParty();
		blocker = FindMemberInDirection(x, y, 0, direction);
		if (blocker != -1)
			StepAroundMember(0, blocker, direction);
		else {
			Item_forceMove(&PartyMembers[0], direction, (char) flags);
			PartyMemberFlags[0] |= MEMBER_STEPPED;
			PartyFacing = direction;
			AvatarIdleSteps = 0;
		}
	} else
		passes = 1;

	/* each follower steps toward its leader, or failing that toward the nearest member ahead */
	for (pass = 0; pass < passes; pass++)
		for (i = 1; i < PartySize; i++) {
			if (NPC(&PartyMembers[i])->workType != WORK_FOLLOW_AVT)
				continue;
			PartyMemberFlags[i] &= ~MEMBER_LOST;
			head = GetFormationLeader(i);
			result = ChooseFollowStep(i, head, direction, &bestDir, &bestDz, &swap, &bestDist);
			if (result != 1) {
				found = 0;
				bestDist = 999;
				for (j = 0; j < i; j++) {
					if (head == j)
						continue;
					tried = ChooseFollowStep(i, j, direction, &tryDir, &tryDz, &swap, &tryDist);
					if (tried == 1 && (tryDist < bestDist || bestDist == 1 && tryDist == 2)) {
						bestDir = altDir = tryDir;
						bestDist = tryDist;
						bestDz = tryDz;
						found = 1;
					}
					if (tried == 2) {
						altDir = tryDir;
						found = 1;
					}
				}
				if (!found)
					PartyMemberFlags[i] |= MEMBER_LOST;
			}
			if (result == 0 && found)
				bestDir = altDir;
			if (bestDir != -1) {
				if (swap)
					StepAroundMember(i, swap, bestDir);
				else {
					Item_forceMove(&PartyMembers[i], bestDir, bestDz);
					PartyMemberFlags[i] |= MEMBER_STEPPED;
				}
			}
		}

	/* post each member's step and set it down where it now stands */
	for (int k = 0; k < PartySize; k++)
		if ((k == 0 || NPC(&PartyMembers[k])->workType == WORK_FOLLOW_AVT) &&
			!(unsigned char) (PartyMemberFlags[k] & MEMBER_ON_PATH) &&
			!(unsigned char) (Item_getQualityFlags(&PartyMembers[k]) & 0x20)) {
			StepScript s(PartyMemberFlags[k] & MEMBER_STEPPED, direction);
			who = PartyMembers[k];
			RunItemScript((unsigned char *)&s, who);
			Item_move(&who, Item_getX(who), Item_getY(who), Item_getZ(&who));
		}
	return flags;
}

void far RemovePartyFromCollision(void)
{
	int i;

	for (i = 0; i < PartySize; i++)
		RemoveTypeFromCollision(PartyMembers[i]);
}

void far AddPartyToCollision(void)
{
	int i;

	for (i = 0; i < PartySize; i++)
		AddTypeToCollision(PartyMembers[i]);
}

void far UpdatePartyFollow(void)
{
	char flags;
	objref member;
	char nudged = 0;
	int i;

	SeparateDownedMembers();
	PartyFollowTicks++;
	AvatarIdleSteps++;
	if (!IsNpcUnconscious(&AvatarRef)) {
		if (AvatarIdleSteps > 16 && !(Item_getQualityFlags(&AvatarRef) & 0x20)) {
			if (NPC(&AvatarRef)->workType == WORK_FOLLOW_AVT || IsInCombat(&NPCRef(AvatarRef))) {
				StepScript s(0, -1);
				RunItemScript((unsigned char *)&s, AvatarRef);
				AvatarIdleSteps = 0;
				nudged = 1;
			}
		}
	} else
		AvatarIdleSteps = 0;

	for (i = 1; i < PartySize; i++) {
		member = PartyMembers[i];
		if (NPC(&member)->workType != WORK_FOLLOW_AVT)
			continue;
		if (nudged && !(Item_getQualityFlags(&member) & 0x20)) {
			StepScript s(0, -1);
			RunItemScript((unsigned char *)&s, member);
		}
		PartyMemberFlags[i] &= ~MEMBER_STEPPED;
		flags = PartyMemberFlags[i];
		if (flags & MEMBER_ON_PATH) {
			if (!(flags & MEMBER_LOST)) {
				StopPaths(member);
				PartyMemberFlags[i] &= ~MEMBER_ON_PATH;
			} else {
				char result;
				int steps;

				if (MouseExists()) {
					if (CombatGroups.battleMusic | AvatarInCombat)
						steps = Npc_getDexterity(&AvatarRef) / 16 + 1;
					else
						steps = GetCursorLength() + 2;
				} else
					steps = 5;
				result = WalkNPCRoute(member, steps);
				if (result == 0) {
					PartyMemberFlags[i] &= ~MEMBER_ON_PATH;
					break;
				}
				if (result == 2) {
					if (!(unsigned char)IsWorldPosOnScreen(Item_getX(member), Item_getY(member))) {
						Position next;
						if (FindArrivalSpot(member, &next))
							Item_move(&member, next.x, next.y, next.z);
					}
					break;
				}
			}
		} else if (flags & MEMBER_LOST) {
			char k = GetFormationLeader(i);
			Coord unusedX = Item_getX(PartyMembers[k]);
			Coord unusedY = Item_getY(PartyMembers[k]);

			StopPaths(member);
			char status = StartPath(member, Coord(Item_getX(AvatarRef) + 1), Item_getY(AvatarRef),
				Item_getZ(&AvatarRef), -1, DiscardedPathLength, 0);
			if (status == 0)
				PartyMemberFlags[i] |= MEMBER_ON_PATH;
			else if (!(unsigned char)IsWorldPosOnScreen(Item_getX(member), Item_getY(member))) {
				Position next;
				if (FindArrivalSpot(member, &next))
					Item_move(&member, next.x, next.y, next.z);
			}
		}
	}
}

objref *far GetPartyMembers(void)
{
	return PartyMembers;
}

int far GetPartySize(void)
{
	return PartySize;
}

unsigned char far IsPartyMember(objref *item)
{
	return Item_isPartyMember(*item);
}
