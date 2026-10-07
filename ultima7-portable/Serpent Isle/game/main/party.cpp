/* Serpent Isle SI.EXE, overlay segment 231 (file offsets 0x063b70 to 0x0654b0, 6464 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
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
#include "npcpath.h"
#include "movepath.h"

/* NPC alignment bits, where 1 is good. */
#define STATUS_ALIGNMENT    0x18
#define STATUS_GOOD         0x08

#define NPC(p) GetNpcBufferForIbo(p)

struct Position { Coord x, y; int8_t z; };

extern objref AvatarRef;

inline void SetNpcStatus(objref *ref, uint16_t clear, uint16_t set)
{
	NpcBuffer *npc = NPC(ref);
	npc->status &= ~clear;
	npc->status |= set;
}

inline int16_t IsOnScreen(Coord x, Coord y)
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

void TeleportParty(CellCoord x, CellCoord y, int8_t z)
{
	PlacePartyAround(x, y, z);
}

/* Tries paths to the avatar from successive positions around the screen edge. */
uint8_t BringNpcNearAvatar(objref member, int16_t unused, int16_t limit)
{
	if (!member.valid())
		return 0;
	SetNpcStatus(&member, NPC_ASLEEP, 0);
	Coord offsetX = -22;
	Coord oldX = Item_getX(member);
	Coord x = oldX;
	Coord offsetY = -14;
	Coord oldY = Item_getY(member);
	Coord y = oldY;
	int8_t oldZ = Item_getZ(&member);
	TypeFrame typeFrame(ITEM(member.off)->typeFrame);
	uint8_t edge = 1;
	int8_t dx = 0;
	int8_t dy = 0;
	if (CanTypeMoveTo(Item_getX(AvatarRef), Item_getY(AvatarRef) - 2, Item_getZ(&AvatarRef), typeFrame.bits))
		dy = -2;
	if (!dx && !dy &&
		CanTypeMoveTo(Item_getX(AvatarRef) + 2, Item_getY(AvatarRef), Item_getZ(&AvatarRef), typeFrame.bits))
		dx = 2;
	if (!dx && !dy &&
		CanTypeMoveTo(Item_getX(AvatarRef), Item_getY(AvatarRef) + 2, Item_getZ(&AvatarRef), typeFrame.bits))
		dy = 2;
	if (!dx && !dy &&
		CanTypeMoveTo(Item_getX(AvatarRef) - 2, Item_getY(AvatarRef), Item_getZ(&AvatarRef), typeFrame.bits))
		dx = -2;
	if (!dx && !dy &&
		CanTypeMoveTo(Item_getX(AvatarRef) - 2, Item_getY(AvatarRef) - 2, Item_getZ(&AvatarRef), typeFrame.bits)) {
		dx = -2;
		dy = -2;
	}
	if (!dx && !dy &&
		CanTypeMoveTo(Item_getX(AvatarRef) + 2, Item_getY(AvatarRef) + 2, Item_getZ(&AvatarRef), typeFrame.bits)) {
		dx = 2;
		dy = 2;
	}
	if (!dx && !dy &&
		CanTypeMoveTo(Item_getX(AvatarRef) - 2, Item_getY(AvatarRef) + 2, Item_getZ(&AvatarRef), typeFrame.bits)) {
		dx = -2;
		dy = 2;
	}
	if (!dx && !dy &&
		CanTypeMoveTo(Item_getX(AvatarRef) + 2, Item_getY(AvatarRef) - 2, Item_getZ(&AvatarRef), typeFrame.bits)) {
		dx = 2;
		dy = -2;
	}
	while (edge < 9) {
		int8_t result = StartPath(member, Item_getX(AvatarRef) + dx, Item_getY(AvatarRef) + dy,
			Item_getZ(&AvatarRef), limit, DiscardedPathLength, 0);
		if (result == 0)
			return 1;
		if (edge == 1 || edge == 5)
			offsetX += 2;
		if (offsetX > CellCoord(22)) {
			if (edge == 1)
				offsetX -= 2;
			else {
				offsetX -= 1;
				offsetY += 1;
			}
			edge++;
		}
		if (edge == 2 || edge == 6)
			offsetY += 2;
		if (offsetY > CellCoord(14)) {
			if (edge == 2)
				offsetY -= 2;
			else {
				offsetX -= 1;
				offsetY -= 1;
			}
			edge++;
		}
		if (edge == 3 || edge == 7)
			offsetX -= 2;
		if (offsetX < CellCoord(-22)) {
			if (edge == 3)
				offsetX += 2;
			else {
				offsetX += 1;
				offsetY -= 1;
			}
			edge++;
		}
		if (edge == 4 || edge == 8)
			offsetY -= 2;
		if (offsetY < CellCoord(-14)) {
			edge++;
			offsetX += 1;
			offsetY += 2;
		}
		x = Item_getX(AvatarRef) + offsetX;
		y = Item_getY(AvatarRef) + offsetY;
		for (int8_t z = 0; z <= 15; z++) {
			if (CanTypeMoveTo(x, y, z, typeFrame.bits)) {
				Item_move(&member, x, y, z);
				break;
			}
		}
	}
	Item_move(&member, oldX, oldY, oldZ);
	return 0;
}

void RecentreFormation()
{
	TurnFormation(FormationFacing, -(int16_t)FormationFacing);
}

void GrowFormation()
{
	if (FormationSize >= 8)
		ReportError(0x6a01);
	int8_t oldFacing = FormationFacing;
	TurnFormation(FormationFacing, -(int16_t)FormationFacing);
	++FormationSize;
	TurnFormation(FormationFacing, oldFacing - FormationFacing);
}

void ShrinkFormation()
{
	if (FormationSize <= 0)
		ReportError(0x6a02);
	int8_t oldFacing = FormationFacing;
	TurnFormation(FormationFacing, -(int16_t)FormationFacing);
	--FormationSize;
	TurnFormation(FormationFacing, oldFacing - FormationFacing);
}

Party::Party()
{
	int16_t i;
	PartySize = 0;
	for (i = 0; i < 8; ++i)
		PartyMembers[i].off = 0;
	DownedPartyCount = 0;
	/* The original cleared 16 entries here, running on into DownedPartyMembers. */
	for (i = 0; i < 8; ++i)
		PartyMembers[i].off = 0;
	for (i = 0; i < 8; ++i)
		DownedPartyMembers[i].off = 0;
}

void SetPartyWorkType(int8_t activity)
{
	objref *members = PartyMembers;
	int16_t count = PartySize;
	for (int16_t i = 0; i < count; ++i) {
		objref *member = &members[i];
		Npc_setSchedule(member, activity);
		NPC(member)->schedules[NPC(member)->currentSchedule].state = 1;
	}
}

void ResetFormation()
{
	RecentreFormation();
	PartyFacing = 0;
}

void AddToParty(objref member, uint8_t quiet)
{
	int16_t i;
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
			Item_clearTemporary(&iter.current);
			FindItem(&iter);
		}
		RefitInventory(member);
	} else
		ReportError(0x6a01);
}

void LeaveParty(objref *member)
{
	RemoveFromParty(*member, 1);
}

void RemoveFromParty(objref member, uint8_t removeAll)
{
	uint8_t found = 0, foundDowned = 0;
	int16_t i;
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

void DeactivatePartyMember(int8_t index)
{
	objref member = PartyMembers[index];
	RemoveFromParty(member, 0);
	DownedPartyMembers[DownedPartyCount] = member;
	++DownedPartyCount;
}

void ReactivatePartyMember(int8_t index)
{
	objref member = DownedPartyMembers[index];
	AddToParty(member, 1);
	for (int16_t i = index + 1; i < DownedPartyCount; ++i)
		DownedPartyMembers[i - 1] = DownedPartyMembers[i];
	--DownedPartyCount;
}

extern "C" int16_t GetPartyIndex(objref member)
{
	for (int16_t i = 0; i < PartySize; ++i)
		if (PartyMembers[i] == member)
			return i;
	/* Borland returned the PartySize that ended the loop. */
	return PartySize;
}

/* A random spot just past the screen edge on the member's side of the avatar, at the lowest height
 * the member fits. */
uint8_t FindArrivalSpot(objref member, Position *destination)
{
	Coord avatarX = Item_getX(AvatarRef);
	Coord avatarY = Item_getY(AvatarRef);
	int8_t direction;
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
	destination->x = GenerateRandomIntegerInRange(GetDelta(highX, lowX)) + (int16_t)lowX;
	destination->y = GenerateRandomIntegerInRange(GetDelta(highY, lowY)) + (int16_t)lowY;
	uint16_t typeFrame = ITEM(member.off)->typeFrame;
	for (int8_t z = 0; z <= 15; ++z) {
		if (CanTypeMoveTo(destination->x, destination->y, z, typeFrame)) {
			destination->z = z;
			return 1;
		}
	}
	return 0;
}

void PlacePartyAround(Coord x, Coord y, int8_t z)
{
	int8_t dx, dy;
	int16_t placed;
	LeaveVehicle();
	if (!IsOnScreen(x, y)) {
		MainWorldView.setCenter(x, y);
	}
	Item_move(&PartyMembers[0], x, y, z);
	for (int16_t i = 1; i < PartySize; ++i) {
		placed = 0;
		for (int16_t direction = 0; direction < 8; ++direction) {
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

void ResetPartyFormation()
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
	f.write(PartyMembers, INT32_C(16));
	f.writeByte(PartySize);
	f.writeByte(PartyFacing);
	f.write(PartyMemberFlags, INT32_C(8));
	f.writeByte(PartyFollowTicks);
	f.writeByte(AvatarIdleSteps);
	f.write(FormationSlotX, INT32_C(64));
	f.write(FormationSlotY, INT32_C(64));
	f.writeByte(FormationFacing);
	f.writeByte(FormationSize);
	f.write(DownedPartyMembers, INT32_C(32));
	f.writeByte(DownedPartyCount);
}

void PartySaver::load(char *directory)
{
	DataFile f(BuildPath(directory, PartyFileName, 0), 1);
	f.read(PartyMembers, INT32_C(16));
	PartySize = f.readByte();
	PartyFacing = f.readByte();
	f.read(PartyMemberFlags, INT32_C(8));
	PartyFollowTicks = f.readByte();
	AvatarIdleSteps = f.readByte();
	f.read(FormationSlotX, INT32_C(64));
	f.read(FormationSlotY, INT32_C(64));
	FormationFacing = f.readByte();
	FormationSize = f.readByte();
	f.read(DownedPartyMembers, INT32_C(32));
	DownedPartyCount = f.readByte();
}

extern "C" void ResetPartyGlobals(void)
{
	memcpy(PartyFileName, "PARTY", sizeof(PartyFileName));
	memset((void *)&PartyFile, 0, sizeof(PartyFile));
}

extern "C" void ConstructPartyGlobals(void)
{
	new (&PartyFile) PartySaver();
}
