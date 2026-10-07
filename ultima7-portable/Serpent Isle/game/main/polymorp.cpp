/* Serpent Isle SI.EXE, overlay segment 234 (file offsets 0x065fa0 to 0x0663a4, 1028 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

/* path: polymorp.c */
#include "u7port.h"
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

inline uint8_t IsGroundTile(int16_t shape) { return shape < 150; }

/* Gives shape the look of record look in the shapes file. */
void ChangeShape(int16_t shape, int16_t look)
{
	if (IsGroundTile(shape) || IsGroundTile(look))
		return;
	RemapShapeRecord(shape, look);
}

/* After a restore: every NPC back to its own shape, then the polymorphed ones changed again. */
void RestorePolymorphs(void)
{
	objref npc;
	int16_t i;

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
void SetPolymorph(objref npc, uint16_t shape)
{
	TypeInfo info;
	TypeInfo *entry;
	uint16_t look;
	int16_t fd;

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
		if ((uint8_t)Item_isAvatar(&npc))
			FindAvatarNpc();
		else {
			ChangeShape(npc.type(), shape);
			CopyWihhEntry(npc.type(), look);
		}
	}
	fd = DosOpen(BuildPath(StaticPath, "TFA.DAT", 0));
	if (fd < 0)
		ReportFileNotFound(BuildPath(StaticPath, "TFA.DAT", 0));
	DosRead(fd, look * INT32_C(3), INT32_C(3), &info);
	DosClose(fd);
	entry = &gItemTypeInfo[npc.type()];
	entry->footprintX = info.footprintX;
	entry->footprintY = info.footprintY;
	entry->height = info.height;
}

/* Turns a world position into one within its region, and the map number into the region's. */
void GetRegionPosition(int16_t *x, int16_t *y, int16_t *map)
{
	*map = (uint8_t)GetRegionAt(*x, *y, *map);
	int16_t i;
	for (i = 0; i < 25; i++)
		if ((int16_t)RegionX[(uint8_t)i] > *x) {
			i--;
			break;
		}
	*x -= RegionX[(uint8_t)i];
	for (i = 0; i < 25; i++)
		if ((int16_t)RegionY[(uint8_t)i] > *y) {
			i--;
			break;
		}
	*y -= RegionY[(uint8_t)i];
}

/* Once the Banes are free, Moonshade is ravaged the first time a game is restored. */
void CheckMoonshadeRavaged(void)
{
	if (GameFlags.get(BANES_RELEASED) && !GameFlags.get(MOONSHADE_RAVAGED))
		RunUsable(EVENT_ACTION, AvatarRef, 0x68c);
}
