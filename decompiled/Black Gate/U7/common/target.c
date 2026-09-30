/* Black Gate U7.EXE, overlay segment 262 (file offsets 0x07be50 to 0x07c33a, 1258 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "lowlevel.h"
#include "iteminfo.h"
#include "objref.h"
#include "itemrec.h"
#include "vooalloc.h"
#include "u7manage.h"
#include "itembuf.h"
#include "sortitem.h"
#include "coord.h"
#include "mapview.h"
#include "oops.h"
#include "type.h"
#include "npcref.h"
#include "item.h"

/* 7-byte NPC records kept in Voodoo memory */
struct NPCPose {
	unsigned typeFrame;
	int x;
	int y;
	unsigned char z;
};

/* a point on the screen */
struct Point {
	int x, y;
	Point(int px, int py) { x = px; y = py; }
};

long ShadowNpcBuffer;

extern "C" char far TestShapeHit(long, int, const Point far *, const Point far *, int);

#define ITEM_TYPE(sf) ((sf) & 0x3ff)
#define FRAME(sf) (((sf) & 0x7c00) >> 10)
#define FLIPPED(sf) (((sf) & 0x8000) == 0x8000)
#define INFO(sf) (gItemTypeInfo[ITEM_TYPE(sf)])
#define IS_NPC(sf) ((unsigned char) ((ItemTypeClassFlags[INFO(sf).typeClass] & CLASS_NPC) != 0))
#define IS_CLASS0(sf) ((char) (INFO(sf).typeClass == TYPE_CLASS_NONE))
#define IS_VALID(r) ((char) ((r) != 0))

void far AllocateNPCPoses(long *table)
{
	*table = AllocateVoodooMemory(&VoodooXmsBlock, (NpcRecordCount + ExtraNpcRecordCount) * sizeof(struct NPCPose));
	if (*table == 0)
		ReportOutOfVoodooMemory();
}

void far GetNPCPose(long *table, int i, struct NPCPose *buf)
{
	if (i >= 0 && i < NpcRecordCount + ExtraNpcRecordCount)
		CopyLinearToFar(buf, *table + i * sizeof(struct NPCPose), sizeof(struct NPCPose));
}

/* Finds the item drawn under screen point (x, y). Pass 0 tries NPCs only, pass 1 skips type class 0,
 * and transparent types wait for pass 3; opaqueOnly runs pass 3 alone, without them. */
char far FindItemAtScreenPoint(objref *hit, int x, int y, WorldView *view, ShapeManager *shapes, char opaqueOnly)
{
	int obj;
	unsigned sf;
	int type;
	int x0;
	int y0;
	int px;
	int py;
	char found = 0;
	int pass;
	int frame;
	int npc;
	int z;
	struct NPCPose pose;
	struct NPCPose *unusedPose = &pose;
	char flipped;

	for (pass = opaqueOnly ? 3 : 0; pass <= 3; pass++) {
		*hit = 0;
		for (obj = ItemRenderOrder.first(); IS_VALID(obj); obj = ItemRenderOrder.nextItem()) {
			sf = ITEM(obj)->typeFrame;
			if (pass < 3 && (char) INFO(sf).transparent)
				continue;
			if (pass == 0) {
				if (!IS_NPC(sf))
					continue;
			} else if (pass == 1) {
				if (IS_CLASS0(sf))
					continue;
			}
			if (opaqueOnly && (char) INFO(sf).transparent)
				continue;
			x0 = view->cellToScreenX(Item_getX((objref &)obj)) - Item_getZ((objref *)&obj) * 4;
			y0 = view->cellToScreenY(Item_getY((objref &)obj)) - Item_getZ((objref *)&obj) * 4;
			type = ITEM_TYPE(sf);
			if (shapes->get(type) == 0)
				continue;
			if (IS_NPC(sf)) {
				npc = Item_getNpcNumber((objref *)&NPCRef(obj));
				GetNPCPose(&ShadowNpcBuffer, npc, &pose);
				flipped = FLIPPED(pose.typeFrame);
			} else
				flipped = FLIPPED(ITEM(obj)->typeFrame);
			/* test the point, in the shape's own axes, against its pixels */
			{
				if (flipped) {
					px = y - y0 + x0;
					py = x - x0 + y0;
				} else {
					px = x;
					py = y;
				}
				if (IS_NPC(sf)) {
					type = ITEM_TYPE(pose.typeFrame);
					frame = FRAME(pose.typeFrame);
					z = pose.z;
					x0 = view->cellToScreenX(pose.x) - z * 4;
					y0 = view->cellToScreenY(pose.y) - z * 4;
				} else
					frame = FRAME(ITEM(obj)->typeFrame);
				if (TestShapeHit(shapes->get(type), frame, &Point(x0, y0), &Point(px, py), 0x101)) {
					*hit = obj;
					found = 1;
					break;
				}
			}
		}
		if (found)
			break;
	}
	return found;
}
