/* Serpent Isle SI.EXE, overlay segment 234 (file offsets 0x065fa0 to 0x0663a4, 1028 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

/* path: polymorp.c */
#include "objref.h"
#include "itemrec.h"
#include "coord.h"
#include "maps.h"
#include "type.h"
#include "dosio.h"
#include "easyfile.h"
#include "oops.h"
#include "init.h"
#include "debug.h"
#include "npcref.h"
#include "u7npc.h"
#include "u7manage.h"
#include "wihh.h"
#include "flags.h"
#include "usehook.h"
#include "polymorp.h"

#define EVENT_ACTION        2
#define EVENT_POLYMORPH     8

#define AVATAR_MALE         721
#define AVATAR_FEMALE       989

#define BANES_RELEASED      4
#define MOONSHADE_RAVAGED   792

extern objref AvatarRef;

inline unsigned char IsGroundTile(int shape) { return shape < 150; }

/* Gives shape the look of record look in the shapes file. */
void ChangeShape(int shape, int look)
{
	if (IsGroundTile(shape) || IsGroundTile(look))
		return;
	RemapShapeRecord(shape, look);
}

/* After a restore: every NPC back to its own shape, then the polymorphed ones changed again. */
void RestorePolymorphs(void)
{
	objref npc;
	int i;

	for (i = 0; i < 356; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && Item_getNpcNumber(&npc) == i)
			ChangeShape(npc.type(), npc.type());
	}
	FindAvatarNpc();
	for (i = 0; i < 356; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && Npc_hasPolymorphFlag(&npc) && Item_getNpcNumber(&npc) == i)
			RunUsable(EVENT_POLYMORPH, npc, -1);
	}
}

/* Makes an NPC look like shape, or its own shape again when shape is its type. */
void SetPolymorph(objref npc, unsigned shape)
{
	TypeInfo info;
	TypeInfo *entry;
	unsigned look;
	int fd;

	switch (shape) {
	case 239:
		Npc_setPolymorphFlag(&npc);
		ChangeShape(npc.type(), shape);
		return;
	case 0x400:
	case 0x402:
	case 0x404:
	case 0x406:
	case 0x408:
	case 0x40a:
		look = AVATAR_MALE;
		break;
	case 0x401:
	case 0x403:
	case 0x405:
	case 0x407:
	case 0x409:
	case 0x40b:
		look = AVATAR_FEMALE;
		break;
	default:
		look = shape;
		break;
	}
	if (npc.type() != shape) {
		Npc_setPolymorphFlag(&npc);
		if (shape == AVATAR_MALE) {
			shape = 0x405;
			shape -= Npc_isMale(&AvatarRef) ? 1 : 0;
			shape -= Npc_getSkinColor(&AvatarRef) * 2;
			ChangeShape(npc.type(), shape);
			CopyWihhEntry(npc.type(), look);
		} else {
			ChangeShape(npc.type(), shape);
			CopyWihhEntry(npc.type(), look);
		}
	} else {
		Npc_clearPolymorphFlag(&npc);
		if ((unsigned char)Item_isAvatar(&npc))
			FindAvatarNpc();
		else {
			ChangeShape(npc.type(), shape);
			CopyWihhEntry(npc.type(), look);
		}
	}
	fd = DosOpen(BuildPath(StaticPath, "TFA.DAT", 0));
	if (fd < 0)
		ReportFileNotFound(BuildPath(StaticPath, "TFA.DAT", 0));
	DosRead(fd, look * 3L, 3L, &info);
	DosClose(fd);
	entry = (TypeInfo *)((long)gItemTypeInfo + npc.type() * sizeof(TypeInfo));
	entry->footprintX = info.footprintX;
	entry->footprintY = info.footprintY;
	entry->height = info.height;
}

/* Turns a world position into one within its region, and the map number into the region's. */
void GetRegionPosition(int *x, int *y, int *map)
{
	*map = (unsigned char)GetRegionAt(*x, *y, *map);
	int i;
	for (i = 0; i < 25; i++)
		if ((int)RegionX[(unsigned char)i] > *x) {
			i--;
			break;
		}
	*x -= RegionX[(unsigned char)i];
	for (i = 0; i < 25; i++)
		if ((int)RegionY[(unsigned char)i] > *y) {
			i--;
			break;
		}
	*y -= RegionY[(unsigned char)i];
}

/* Once the Banes are free, Moonshade is ravaged the first time a game is restored. */
void CheckMoonshadeRavaged(void)
{
	if (GameFlags.get(BANES_RELEASED) && !GameFlags.get(MOONSHADE_RAVAGED))
		RunUsable(EVENT_ACTION, AvatarRef, 0x68c);
}
