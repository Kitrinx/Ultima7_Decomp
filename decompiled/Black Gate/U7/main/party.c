/* Black Gate U7.EXE, overlay segment 249 (file offsets 0x075460 to 0x076389, 3881 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

#include "objref.h"
#include "activity.h"
#include "dosio.h"
#include "typefram.h"
#include "itemrec.h"
#include "coord.h"
#include "u7npc.h"
#include "easyfile.h"
#include "chkfile.h"
#include "actqueue.h"
#include "death.h"
#include "search.h"
#include "random.h"
#include "barge.h"
#include "mapview.h"
#include "itable.h"
#include "datanode.h"
#include "camera.h"
#include "item.h"
#include "legalmov.h"
#include "partymov.h"
#include "party.h"

/* NPC alignment bits, where 1 is good. */
#define STATUS_ALIGNMENT    0x18
#define STATUS_GOOD         0x08

#define NPC(p) GetNpcBufferForIbo(p)

struct Position { Coord x, y; char z; };

extern objref AvatarRef;

inline void SetNpcStatus(objref *ref, unsigned clear, unsigned set)
{
	NpcBuffer far *npc = NPC(ref);
	npc->status &= ~clear;
	npc->status |= set;
}

inline int IsOnScreen(Coord x, Coord y)
{
	if (x < CellWindowX || x >= CellWindowX + 80 ||
		y < CellWindowY || y >= CellWindowY + 80)
		return 0;
	else
		return 1;
}

/* Start-up state for the party's formation. */
struct Formation {
	Formation() { FormationFacing = 0; FormationSize = 0; }
};

/* Start-up state for the party list. */
struct Party { Party(); };

/* The PARTY file in a saved game. */
struct PartySaver : DataNode {
	Formation marching;
	Party members;
	char *name();
	void load(char *);
	void save(char *);
};

char PartyFileName[] = "PARTY";
PartySaver PartyFile;

void far TeleportParty(CellCoord x, CellCoord y, char z)
{
	PlacePartyAround(x, y, z);
}

void far RecentreFormation()
{
	char unusedFacing = FormationFacing;
	TurnFormation(FormationFacing, -(int)FormationFacing);
}

void far GrowFormation()
{
	if (FormationSize >= 8)
		ReportError(0x6a01);
	char oldFacing = FormationFacing;
	TurnFormation(FormationFacing, -(int)FormationFacing);
	++FormationSize;
	TurnFormation(FormationFacing, oldFacing - FormationFacing);
}

void far ShrinkFormation()
{
	if (FormationSize <= 0)
		ReportError(0x6a02);
	char oldFacing = FormationFacing;
	TurnFormation(FormationFacing, -(int)FormationFacing);
	--FormationSize;
	TurnFormation(FormationFacing, oldFacing - FormationFacing);
}

Party::Party()
{
	PartySize = 0;
	for (int i = 0; i < 8; ++i)
		PartyMembers[i].off = 0;
	DownedPartyCount = 0;
	/* 16 entries: runs on through the first half of DownedPartyMembers */
	for (i = 0; i < 16; ++i)
		PartyMembers[i].off = 0;
}

void far SetPartyWorkType(char activity)
{
	objref *members = PartyMembers;
	int count = PartySize;
	for (int i = 0; i < count; ++i) {
		objref *member = &members[i];
		Npc_setSchedule(member, activity);
		NPC(member)->schedules[NPC(member)->currentSchedule].state = 1;
	}
}

void far ResetFormation()
{
	RecentreFormation();
	PartyFacing = 0;
}

void far AddToParty(objref member, unsigned char quiet)
{
	int i;
	for (i = 0; i < PartySize; i++) {
		if (PartyMembers[i] == member && PartyMembers[i].valid())
			return;
	}
	if (PartySize + 1 <= 8) {
		PartyMembers[PartySize] = member.off;
		PartyMemberFlags[PartySize] = 0;
		PartyMemberFlags[PartySize] |= MEMBER_LOST;   /* it finds its way to the avatar */
		PartySize++;
		GrowFormation();
		SetNpcStatus(&member, 0, NPC_IN_PARTY);
		if (!quiet) {
			SetNpcStatus(&member, STATUS_ALIGNMENT, STATUS_GOOD);
			Npc_setSchedule(&member, WORK_FOLLOW_AVT);
			NPC(&member)->schedules[NPC(&member)->currentSchedule].state = 1;
			NPC(&member)->defaultAttackMode = 0;
			ActionQueue.remove(member.off, 1, 0);
		}
		AreaSearch iter;
		FindItemInContainer(&iter, member, 0, -1, 255, 255);
		while (iter.current.valid()) {
			Item_setOkayToTake(&iter.current);
			FindItem(&iter);
		}
		RefitInventory(member);
	} else
		ReportError(0x6a01);
}

void far LeaveParty(objref *member)
{
	RemoveFromParty(*member, 1);
}

void far RemoveFromParty(objref member, unsigned char removeAll)
{
	unsigned char found = 0, foundDowned = 0;
	int i;
	for (i = 0; i < PartySize; ++i) {
		if (!found && PartyMembers[i] == member)
			found = 1;
		else if (found) {
			PartyMemberFlags[i - 1] = PartyMemberFlags[i];
			PartyMembers[i - 1] = PartyMembers[i];
		}
	}
	if (removeAll) {
		for (i = 0; i < DownedPartyCount; ++i) {
			if (!foundDowned && DownedPartyMembers[i] == member)
				foundDowned = 1;
			else if (foundDowned)
				DownedPartyMembers[i - 1] = DownedPartyMembers[i];
		}
		if (!foundDowned && !found)
			return;
	}
	if (found) {
		--PartySize;
		ShrinkFormation();
	}
	if (foundDowned)
		--DownedPartyCount;
	if (removeAll && (found || foundDowned)) {
		SetNpcStatus(&member, NPC_IN_PARTY, 0);
		SetNpcStatus(&member, STATUS_ALIGNMENT, 0);
		NPC(&member)->defaultAttackMode = 0;
		ActionQueue.remove(member.off, 1, 0);
		Npc_setSchedule(&member, WORK_WANDER);
		NPC(&member)->schedules[NPC(&member)->currentSchedule].state = 1;
		AreaSearch iter;
		FindItemInContainer(&iter, member, 0, -1, 255, 255);
		while (iter.current.valid()) {
			Item_clearOkayToTake(&iter.current);
			FindItem(&iter);
		}
	}
}

void far DeactivatePartyMember(char index)
{
	objref member = PartyMembers[index];
	RemoveFromParty(member, 0);
	DownedPartyMembers[DownedPartyCount] = member;
	++DownedPartyCount;
}

void far ReactivatePartyMember(char index)
{
	objref member = DownedPartyMembers[index];
	AddToParty(member, 1);
	for (int i = index + 1; i < DownedPartyCount; ++i)
		DownedPartyMembers[i - 1] = DownedPartyMembers[i];
	--DownedPartyCount;
}

extern "C" int far GetPartyIndex(objref member)
{
	for (int i = 0; i < PartySize; ++i)
		if (PartyMembers[i] == member)
			return i;
}

/* A random spot just past the screen edge on the member's side of the avatar, at the lowest height
 * the member fits. */
unsigned char far FindArrivalSpot(objref member, Position *destination)
{
	Coord avatarX = Item_getX(AvatarRef);
	Coord avatarY = Item_getY(AvatarRef);
	char direction;
	Coord x, y;
	if (!member.valid())
		direction = GenerateRandomIntegerInRange(8);
	else {
		x = Item_getX(member);
		y = Item_getY(member);
		direction = DirectionBySign[GetSign(GetDelta(avatarX, x)) + 1][GetSign(GetDelta(y, avatarY)) + 1];
	}
	Coord lowX, lowY, highX, highY;
	switch (direction) {
	case 0: case 1:
		lowX = avatarX - 22; highX = avatarX + 22;
		lowY = avatarY - 18; highY = avatarY - 14;
		break;
	case 2: case 3:
		lowX = avatarX + 22; highX = avatarX + 26;
		lowY = avatarY - 14; highY = avatarY + 14;
		break;
	case 4: case 5:
		lowX = avatarX - 22; highX = avatarX + 22;
		lowY = avatarY + 14; highY = avatarY + 18;
		break;
	case 6: case 7:
		lowX = avatarX - 26; highX = avatarX - 22;
		lowY = avatarY - 14; highY = avatarY + 14;
	}
	destination->x = GenerateRandomIntegerInRange(GetDelta(highX, lowX)) + (int)lowX;
	destination->y = GenerateRandomIntegerInRange(GetDelta(highY, lowY)) + (int)lowY;
	unsigned typeFrame = ITEM(member.off)->typeFrame;
	for (char z = 0; z <= 15; ++z) {
		if (CanTypeMoveTo(destination->x, destination->y, z, typeFrame)) {
			destination->z = z;
			return 1;
		}
	}
	return 0;
}

void far PlacePartyAround(Coord x, Coord y, char z)
{
	char dx, dy;
	int placed;
	LeaveVehicle();
	if (!IsOnScreen(x, y)) {
		MainWorldView.setCenter(x, y);
	}
	Item_move(&PartyMembers[0], x, y, z);
	for (int i = 1; i < PartySize; ++i) {
		placed = 0;
		for (int direction = 0; direction < 8; ++direction) {
			dx = DirectionDX[direction];
			dy = DirectionDY[direction];
			if (CanTypeMoveTo(x + dx, y + dy, z, ITEM(PartyMembers[i].off)->typeFrame)) {
				if (NPC(&PartyMembers[i])->workType != WORK_WAIT) {
					Item_move(&PartyMembers[i], x + dx, y + dy, z);
					placed = 1;
				}
				break;
			}
		}
		if (!placed && NPC(&PartyMembers[i])->workType != WORK_WAIT) {
			Item_move(&PartyMembers[i], x, y, z);
		}
	}
	SetPointerZ(z);
	CenterOnAvatar();
}

void far ResetPartyFormation()
{
	ResetFormation();
}

char *PartySaver::name()
{
	return PartyFileName;
}

void PartySaver::save(char *directory)
{
	if (PartySize == 0) {
		AddToParty(AvatarRef, 0);
	}
	DataFile f(BuildPath(directory, PartyFileName, 0), 0);
	f.write(PartyMembers, 16L);
	f.writeByte(PartySize);
	f.writeByte(PartyFacing);
	f.write(PartyMemberFlags, 8L);
	f.writeByte(PartyFollowTicks);
	f.writeByte(AvatarIdleSteps);
	f.write(FormationSlotX, 64L);
	f.write(FormationSlotY, 64L);
	f.writeByte(FormationFacing);
	f.writeByte(FormationSize);
	f.write(DownedPartyMembers, 32L);
	f.writeByte(DownedPartyCount);
}

void PartySaver::load(char *directory)
{
	DataFile f(BuildPath(directory, PartyFileName, 0), 1);
	f.read(PartyMembers, 16L);
	PartySize = f.readByte();
	PartyFacing = f.readByte();
	f.read(PartyMemberFlags, 8L);
	PartyFollowTicks = f.readByte();
	AvatarIdleSteps = f.readByte();
	f.read(FormationSlotX, 64L);
	f.read(FormationSlotY, 64L);
	FormationFacing = f.readByte();
	FormationSize = f.readByte();
	f.read(DownedPartyMembers, 32L);
	DownedPartyCount = f.readByte();
}
