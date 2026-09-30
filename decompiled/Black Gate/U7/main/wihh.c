/* Black Gate U7.EXE, resident segment 70 (file offsets 0x026f08 to 0x0277fd, 2293 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <stdlib.h>
#include <io.h>
#include "objref.h"
#include "lowlevel.h"
#include "dosio.h"
#include "iteminfo.h"
#include "coord.h"
#include "u7npc.h"
#include "easyfile.h"
#include "vooalloc.h"
#include "u7manage.h"
#include "oops.h"
#include "mapview.h"
#include "bltshape.h"
#include "sounds.h"
#include "item.h"
#include "type.h"
#include "xformtbl.h"
#include "debug.h"
#include "initwp.h"
#include "equip.h"

struct VoodooAddress {
	long address;
	VoodooAddress(long n) { address = n; }
	VoodooAddress operator+(unsigned n) { return address + n; }
	operator long() { return address; }
};

/* Where a frame's hand, or the held item's grip, sits within the frame. */
struct Hotspot {
	unsigned char x, y;
};

extern objref AvatarRef;
#define NPC(p) GetNpcBufferForIbo(p)
#define FLAG(p, m) ((unsigned char) ((NPC(p)->status & (m)) != 0))

extern View Viewport;

int CastingFramesLeft = 0;
/* WIHH.DAT: per type, the offset of its frames' hotspots. */
VoodooAddress WihhTable = 0L;
/* Per frame of the lit light source. */
char LightSourceStrength[32] = {
	1, 1, 2, 5, 5, 5, 1, 2, 2, 2, 5, 5, 2, 1, 1, 1,
	1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
/* The frame of a held item for each frame of the one holding it; 0 hides it. */
int HeldItemFrame[32] = {
	1, 1, 1, 1, 4, 3, 2, 4, 3, 2, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 2, 3, 4, 2, 3, 4, 0, 0, 0, 0, 0, 0
};
char CurrentNpcTint = -1;
unsigned char NpcTintColors[] = {
	0x00, 0x0e, 0x1d, 0x1e, 0x2d, 0x2e, 0x3a, 0x3b, 0x49, 0x58, 0x65, 0x66, 0x75, 0x76,
	0x85, 0x86, 0x93, 0x94, 0xa3, 0xb1, 0xb2, 0xbf, 0xc0, 0xd0, 0xd1, 0xe0, 0xdf, 0xff
};

int far AllocateVoodooBlock(long *address, unsigned size)
{
	if ((*address = AllocateVoodooMemory(&VoodooXmsBlock, (long) size)) == 0)
		ReportOutOfVoodooMemory();
	return *address != 0;
}

/* The item in one of the owner's 12 slots; 20 asks for either hand, 21 for slot 6 or 7. */
ItemId far GetItemInSlot(ItemId owner, unsigned char slot)
{
	ItemId result;
	objref ref;
	int npc;

	result = 0;
	ref = owner;
	npc = Item_getNpcNumber(&ref);
	if (npc != -1 && slot <= 12) {
		if (slot == 20) {
			result.off = PeekWord(EquipList + npc * 24 + 2);
			if (!result.valid())
				result.off = PeekWord(EquipList + npc * 24 + 4);
		} else if (slot == 21) {
			result.off = PeekWord(EquipList + npc * 24 + 12);
			if (!result.valid())
				result.off = PeekWord(EquipList + npc * 24 + 14);
		} else
			result.off = PeekWord(EquipList + npc * 24 + slot * 2);
	}
	return result;
}

struct Hotspot far ReadHotspot(VoodooAddress table, int frame)
{
	struct Hotspot spot;
	unsigned word;

	word = PeekWord(table.address + frame * 2);
	spot.x = word & 0xff;
	spot.y = word >> 8;
	return spot;
}

void far LoadWihh(char far *name)
{
	int fd;
	long len;

	fd = DosOpen(name);
	if (fd < 0)
		ReportFileNotFound(name);
	len = filelength(fd);
	AllocateVoodooBlock(&WihhTable.address, (unsigned) len);
	if ((char) (WihhTable == 0))
		ReportOutOfVoodooMemory();
	ReadHandleToVoodoo(fd, 0L, (unsigned) len, &WihhTable.address);
	DosClose(fd);
}

/* A frame's hotspot, x and y swapped when the frame is flipped; 255 marks none. */
char far GetWihhHotspot(unsigned type, int frame, char flip, int *x, int *y)
{
	Hotspot spot;
	unsigned entry;
	*x = 255;
	*y = 255;
	entry = PeekWord(WihhTable.address + type * 2);
	if (entry != 0) {
		spot = ReadHotspot(WihhTable.address + entry, frame);
		if (flip) {
			*x = spot.y;
			*y = spot.x;
		} else {
			*x = spot.x;
			*y = spot.y;
		}
	}
	return *x != 255 && *y != 255;
}

/* A light source's strength, fading with its distance from the avatar. */
int far GetLightStrength(int type, int frame, ItemId obj)
{
	int strength = 1;
	int value, dx, dy;
	if (type == 338)   /* lit light source */
		strength = LightSourceStrength[frame];
	else if (type == 739)  /* firepit */
		strength = frame * 2;
	else if (type == 179) {    /* lightning */
		if (frame)
			strength = 5;
		else
			strength = 0;
	} else if (type == 198)    /* light */
		strength = 3;
	else if (type == 701)  /* lit torch */
		strength = 5;
	else if (type == 782 || type == 553 || type == 551)  /* flaming oil, firedoom staff, fire sword */
		strength = 5;
	else if (type == 526 || type == 825)  /* lit lamp, campfire */
		strength = 7;
	dx = GetDelta(Item_getX(obj.off), Item_getX(AvatarRef));
	dy = GetDelta(Item_getY(obj.off), Item_getY(AvatarRef));
	value = 75 - (abs(dy) * 3 + abs(dx) * 2);
	if (value < 0)
		value = 0;
	strength *= value;
	return strength;
}

/* Draws the item in the NPC's hand at its hotspot, and adds the light of what it holds. */
void far DrawHeldItem(ItemId obj, int x, int y)
{
	ItemSnapshot base;
	ItemRecord far *source;
	ItemSnapshot *target;
	objref npc = obj.off;
	objref held;
	TypeFrame npcTypeFrame, heldTypeFrame;
	int type, frame, npcFrame;
	char flipped, light;
	int height, npcX, npcY, heldX, heldY;
	source = ITEM(npc.off);
	target = &base;
	target->position = source->position;
	target->typeFrame = source->typeFrame;
	npcTypeFrame = base.typeFrame;
	flipped = (npcTypeFrame.bits & 0x8000) == 0x8000;
	held.off = 0;
	if (CastingFramesLeft != 0 && npc.off == AvatarRef.off) {
		CastingFramesLeft--;
		heldTypeFrame = 859;   /* casting frames */
	} else {
		held = objref(GetItemInSlot(npc, 1));
		heldTypeFrame = ITEM(held.off)->typeFrame;
	}
	type = heldTypeFrame.type();
	frame = heldTypeFrame.frame();
	light = gItemTypeInfo[type].light;
	if (GetWihhHotspot(npcTypeFrame.type(), npcTypeFrame.frame(), flipped, &npcX, &npcY)) {
		npcFrame = npcTypeFrame.frame();
		if (type != 338)   /* lit light source */
			frame = HeldItemFrame[npcFrame];
		if (frame != 0 && GetWihhHotspot(type, frame, flipped, &heldX, &heldY)) {
			height = Item_getZ(&npc) * 4;
			ShapeManager_draw(&gShapeManager, &Viewport, x * 8 - height - npcX + heldX + 7,
				y * 8 - height - npcY + heldY + 7, type, frame, flipped,
				(unsigned char) (Item_getQualityFlags(&npc) & QUALITY_INVISIBLE) != 0);
		}
	}
	if (held.valid()) {
		if (light)
			LightTotal += GetLightStrength(type, frame, held);
		PlayItemAmbientSound(type, frame, GetDelta(Item_getX(held), MainWorldView.centerX),
			GetDelta(Item_getY(held), MainWorldView.centerY));
	}
	held = objref(GetItemInSlot(npc, 2));
	heldTypeFrame = ITEM(held.off)->typeFrame;
	type = heldTypeFrame.type();
	frame = heldTypeFrame.frame();
	light = gItemTypeInfo[type].light;
	if (light)
		LightTotal += GetLightStrength(type, frame, held);
}

/* Tints the NPC about to be drawn by its state: a one-shot flash, then charmed, cursed, paralyzed,
 * protected or poisoned. */
void far SetNpcTint(int obj)
{
	char frame = -1;
	if (obj != 0) {
		objref ref = obj;
		if ((unsigned char)(NPC(&ref)->typeFlags & 8)) {
			frame = 12;
			NPC(&ref)->typeFlags = NPC(&ref)->typeFlags & ~8;
		} else if (FLAG(&ref, NPC_CHARMED))
			frame = 14;
		else if (FLAG(&ref, NPC_CURSED))
			frame = 15;
		else if (FLAG(&ref, NPC_PARALYZED))
			frame = 16;
		else if (FLAG(&ref, NPC_PROTECTED))
			frame = 17;
		else if (FLAG(&ref, NPC_POISONED))
			frame = 13;
	}
	if (CurrentNpcTint != frame) {
		SetXformEntries(&gShapeManager.translations, NpcTintColors, frame);
		CurrentNpcTint = frame;
	}
}

/* Gives type the hotspots WIHH.DAT holds for type source. */
void far CopyWihhEntry(unsigned type, unsigned source)
{
	unsigned offset;
	int fd;
	fd = DosOpen(BuildPath(StaticPath, WihhFileName, 0));
	if (fd < 0)
		ReportFileNotFound(BuildPath(StaticPath, WihhFileName, 0));
	ReadFileBlock(fd, (long)(source * 2), 2L, &offset);
	DosClose(fd);
	PokeWord(WihhTable.address + type * 2, offset);
}
