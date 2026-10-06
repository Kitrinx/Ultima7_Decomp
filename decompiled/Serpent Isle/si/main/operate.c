/* Serpent Isle SI.EXE, overlay segment 230 (file offsets 0x0635d0 to 0x063b4e, 1406 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "collide.h"
#include "sortitem.h"
#include "npcpath.h"
#include "type.h"
#include "legalmov.h"
#include "actitem.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)
#define IS_SOLID(rec) ((char) gItemTypeInfo[TYPE(rec)].solid)

/* A type number with its frame bits. */
struct TypeNumber {
	unsigned bits;
	TypeNumber(unsigned type) { bits = type; }
	int height() { return gItemTypeInfo[bits & 0x3ff].height; }
};

/* Where to stand to operate a type, and which way to face. */
struct OperateSpot {
	char dx;
	char dy;
	char dz;
	unsigned char dir;
};

extern int DiscardedPathLength[2];

/* The types that can be operated; the spots for entry n run from OperateSpotStarts[n] up to
 * OperateSpotStarts[n + 1]. Each spot's comment names its type. */
static int OperableTypes[58] = {
	431, 991, 719, 810, 642, 470, 1011, 377, 717, 696, 889, 995, 340, 739,
	668, 602, 735, 597, 860, 722, 261, 651, 698, 851, 654, 653, 403, 971,
	623, 994, 1003, 633, 1018, 872, 944, 300, 616, 681, 628, 863, 1024, 831,
	1000, 407, 675, 697, 679, 423, 283, 624, 618, 526, 336, 338, 997, 291,
	290, 890
};
static unsigned char OperateSpotStarts[59] = {
	0, 1, 2, 4, 8, 12, 13, 16, 20, 24, 27, 31, 35, 39, 43, 45, 47, 48, 50, 51,
	53, 54, 55, 57, 59, 61, 64, 68, 76, 80, 84, 90, 96, 102, 107, 111, 115, 119, 123, 127,
	131, 133, 134, 139, 147, 149, 150, 154, 156, 164, 168, 172, 176, 180, 184, 188, 192, 196, 204
};
static OperateSpot OperateSpots[270] = {
	{ 2, 0, 0, 6 }, /* 431 */
	{ 0, 1, 0, 0 }, /* 991 */
	{ 0, 1, 0, 0 }, { -1, 1, 0, 0 }, /* 719 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 810 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 642 */
	{ -6, -2, 0, 4 }, /* 470 */
	{ -1, 1, 0, 0 }, { 1, -1, 0, 6 }, { -3, -1, 0, 2 }, /* 1011 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 377 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 717 */
	{ 1, -1, 0, 6 }, { -1, -3, 0, 4 }, { -1, 1, 0, 0 }, /* 696 */
	{ 1, 0, 0, 6 }, { 0, 1, 0, 0 }, { -1, 0, 0, 2 }, { 0, -1, 0, 4 }, /* 889 */
	{ 1, -1, 0, 6 }, { 1, 0, 0, 6 }, { 0, 1, 0, 0 }, { -1, 1, 0, 0 }, /* 995 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 340 */
	{ 1, -2, 0, 6 }, { 1, -1, 0, 6 }, { -1, 1, 0, 0 }, { -2, 1, 0, 0 }, /* 739 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 668 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 602 */
	{ 3, -1, 0, 6 }, /* 735 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 597 */
	{ -2, 3, 0, 0 }, /* 860 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 722 */
	{ -1, 1, 0, 0 }, /* 261 */
	{ 1, 0, 0, 6 }, /* 651 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 698 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 851 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 654 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 1, -1, 0 }, /* 653 */
	{ 1, -1, -1, 6 }, { 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 1, -1, 0 }, /* 403 */
	{ 1, -4, 0, 6 }, { 1, -3, 0, 6 }, { 1, -2, 0, 6 }, { 1, -1, 0, 6 }, /* 971 */
	{ 1, 0, 0, 6 }, { 0, 1, 0, 0 }, { -1, 1, 0, 0 }, { -2, 1, 0, 0 },
	{ 1, -1, -1, 6 }, { 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 1, -1, 0 }, /* 623 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 994 */
	{ 1, -3, 0, 6 }, { 1, -2, 0, 6 }, { 1, -1, 0, 6 }, { 1, 0, 0, 6 }, /* 1003 */
	{ 0, 1, 0, 0 }, { -1, 1, 0, 0 },
	{ 1, -2, 0, 6 }, { 1, -1, 0, 6 }, { 1, 0, 0, 6 }, { 0, 1, 0, 0 }, /* 633 */
	{ -1, 1, 0, 0 }, { -2, 1, 0, 0 },
	{ 1, -1, 0, 6 }, { 1, 0, 0, 6 }, { 0, 1, 0, 0 }, { -1, 1, 0, 0 }, /* 1018 */
	{ -2, 1, 0, 0 }, { -3, 1, 0, 0 },
	{ 1, -1, 0, 6 }, { 1, 0, 0, 6 }, { 0, 1, 0, 0 }, { -1, 1, 0, 0 }, /* 872 */
	{ -2, 1, 0, 0 },
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 944 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 300 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 616 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 681 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 628 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 863 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 1024 */
	{ -1, 1, 0, 0 }, /* 831 */
	{ 1, -1, 0, 6 }, { 1, 0, 0, 6 }, { 0, 1, 0, 0 }, { -1, 1, 0, 0 }, /* 1000 */
	{ -2, 1, 0, 0 },
	{ 1, -2, 0, 6 }, { 1, -1, 0, 6 }, { 1, 0, 0, 6 }, { 0, 1, 0, 0 }, /* 407 */
	{ -1, 1, 0, 0 }, { -2, 1, 0, 0 }, { -3, 1, 0, 0 }, { -4, 1, 0, 0 },
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, /* 675 */
	{ -1, 0, 0, 2 }, /* 697 */
	{ 1, -3, 0, 6 }, { 1, -2, 0, 6 }, { 1, -1, 0, 6 }, { 1, 0, 0, 6 }, /* 679 */
	{ 1, 0, 0, 6 }, { 0, 1, 0, 0 }, /* 423 */
	{ 1, -4, 0, 6 }, { 1, -3, 0, 6 }, { 1, -2, 0, 6 }, { 1, -1, 0, 6 }, /* 283 */
	{ 1, 0, 0, 6 }, { 0, 1, 0, 0 }, { -1, 1, 0, 0 }, { -2, 1, 0, 0 },
	{ 1, 0, -1, 6 }, { 1, -1, -1, 6 }, { 0, 1, -1, 0 }, { -1, 1, -1, 0 }, /* 624 */
	{ 3, 0, -1, 6 }, { 0, 3, -1, 0 }, { -3, 0, -1, 2 }, { 0, -3, -1, 4 }, /* 618 */
	{ 1, 0, 0, 6 }, { 0, 1, 0, 0 }, { -1, 0, 0, 2 }, { 0, -1, 0, 4 }, /* 526 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 336 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 338 */
	{ 1, 0, -1, 6 }, { 0, 1, -1, 0 }, { -1, 0, -1, 2 }, { 0, -1, -1, 4 }, /* 997 */
	{ 1, -2, -1, 6 }, { 1, -1, -1, 6 }, { -1, -2, -1, 2 }, { -1, -1, -1, 2 }, /* 291 */
	{ -2, 1, -1, 0 }, { -1, 1, -1, 0 }, { -1, -1, -1, 4 }, { -2, -1, -1, 4 }, /* 290 */
	{ 1, -2, 0, 6 }, { 1, -1, 0, 6 }, { 1, 0, 0, 6 }, { 0, 1, 0, 0 }, /* 890 */
	{ -1, 1, 0, 0 }, { -2, 1, 0, 0 }, { -3, 1, 0, 0 }, { -4, 1, 0, 0 }
};

Operate UseSpotFinder;

/* Finds a free spot beside the target from which the NPC can operate it and starts the NPC
 * walking there. The spot and the way to face come back through spotX, spotY, spotZ and dir;
 * topType, unless -1, is a type that must also fit on top of the target in front of the spot. */
unsigned char far Operate::findUseSpot(objref *npc, objref target, int *spotX, int *spotY, int *spotZ,
	unsigned char *dir, int topType)
{
	int i, j, count, zlo, zhi, z, found, dz, startZ;
	Coord x, y, x0, y0;
	unsigned type;

	x0 = Item_getX(*npc);
	y0 = Item_getY(*npc);
	startZ = Item_getZ(npc);
	RemoveTypeFromCollision(*npc);
	type = TYPE(ITEM(target.off));
	*spotX = Item_getX(target).value;
	*spotY = Item_getY(target).value;
	*spotZ = Item_getZ(&target);
	if (type == 873)   /* chair */
		count = 1;
	else {
		for (i = 0; i < 58; i++)
			if (OperableTypes[i] == type)
				break;
		if (i == 58)
			return 0;
		count = OperateSpotStarts[i + 1] - OperateSpotStarts[i];
	}
	for (j = 0; j < count; j++) {
		x.value = *spotX;
		y.value = *spotY;
		if (type == 873) { /* chair */
			x += DirDeltaX[(FRAME(ITEM(target.off)) & 3) * 2];
			y += DirDeltaY[(FRAME(ITEM(target.off)) & 3) * 2];
			zlo = 0;
			zhi = 0;
		} else if (type == 697) {  /* podium */
			if (FRAME(ITEM(target.off)) == 0) {
				x += -1;
				y += 0;
			} else if (FRAME(ITEM(target.off)) == 2) {
				x += -1;
				y += -1;
			} else
				return 0;
			zlo = *spotZ;
			zhi = *spotZ;
		} else {
			x += OperateSpots[OperateSpotStarts[i] + j].dx;
			y += OperateSpots[OperateSpotStarts[i] + j].dy;
			dz = OperateSpots[OperateSpotStarts[i] + j].dz;
			if (dz == 0) {
				zlo = *spotZ;
				zhi = *spotZ;
			} else if (dz == -1) {
				zlo = *spotZ - 2;
				zhi = *spotZ;
			} else if (dz == 1) {
				zlo = *spotZ;
				zhi = *spotZ + 2;
			}
		}
		if (zlo < 0)
			zlo = 0;
		if (zhi > 15)
			zhi = 15;
		found = 0;
		for (z = zlo; z <= zhi; z++) {
			if (CanTypeMoveTo(x, y, z, ITEM(npc->off)->typeFrame)) {
				found = 1;
				break;
			}
		}
		if (found) {
			if (type == 873)   /* chair */
				*dir = (FRAME(ITEM(target.off)) & 3) * 2;
			else if (type == 697)  /* podium */
				*dir = FRAME(ITEM(target.off)) ? 4 : 2;
			else
				*dir = OperateSpots[OperateSpotStarts[i] + j].dir;
			if (topType == -1 || CanTypeMoveTo(x + DirDeltaX[*dir], y + DirDeltaY[*dir],
				Item_getZ(&target) + TypeNumber(type).height(), topType)) {
				if (StartPath(*npc, x, y, z, 100, DiscardedPathLength, 0) == 0) {
					StopPaths(*npc);
					*spotX = x.value;
					*spotY = y.value;
					*spotZ = z;
					if (IS_SOLID(ITEM(npc->off)))
						AddTypeToCollision(x0, y0, startZ, ITEM(npc->off)->typeFrame);
					return 1;
				}
			}
		}
	}
	if (IS_SOLID(ITEM(npc->off)))
		AddTypeToCollision(x0, y0, startZ, ITEM(npc->off)->typeFrame);
	return 0;
}
