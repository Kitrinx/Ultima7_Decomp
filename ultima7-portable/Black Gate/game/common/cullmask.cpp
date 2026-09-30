/* Black Gate U7.EXE, resident segment 76 (file offsets 0x02b142 to 0x02b73a, 1528 bytes).
 * Borland C++ 2.0 -mm -O -G -P -b- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "typefram.h"
#include "lowlevel.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "vooalloc.h"
#include "bitarray.h"
#include "chkfile.h"
#include "memapi.h"
#include "oops.h"
#include "type.h"
#include "cullmask.h"

/* The occlusion mask: 98 rows of 128 depth cells, the view's origin 24 cells in from the top left. */
#define MASK_WIDTH  128
#define MASK_ROWS   98
#define MASK_BYTES  ((int32_t) MASK_ROWS * MASK_WIDTH)
#define MASK_MARGIN 24

/* the first type that is not terrain; only those have dimensions */
#define FIRST_OBJECT_TYPE 150

/* a type's extent in cells, from SHPDIMS.DAT */
struct ShapeExtent {
	uint8_t x, y;
};

struct WorldMask {
	WorldMask();
};

ShapeExtent *ShapeDimensions;
WorldMask *WorldMaskObject;
int16_t OcclusionBoxX, OcclusionBoxY, OcclusionBoxZ, OcclusionBoxWidth, OcclusionBoxLength, OcclusionBoxHeight;
int32_t OcclusionMaskEnd, OcclusionMaskSize;
int32_t OcclusionRows = 0;
Coord OcclusionOriginX = 0, OcclusionOriginY = 0;
int32_t OcclusionMask = 0, OcclusionMaskOrigin = 0, OcclusionMaskInset = 0;
uint8_t OccludersPresent = 0, OcclusionEnabled = 1;
uint8_t CheatKeyWToggle = 1, CheatKeyDToggle = 0;

inline uint8_t IsOccludedAt(objref *ref, int16_t x, int16_t y, int16_t z, int8_t flag)
{
	return IsShapeOccluded(ITEM(ref->off)->typeFrame, x, y, z, flag);
}

WorldMask::WorldMask()
{
	int32_t row, table;
	int16_t count;

	OcclusionMaskSize = MASK_BYTES;
	OcclusionMask = AllocateVoodooMemory(&VoodooXmsBlock, OcclusionMaskSize);
	if (OcclusionMask == 0)
		ReportOutOfVoodooMemory();
	OcclusionRows = AllocateVoodooMemory(&VoodooXmsBlock, MASK_ROWS * INT32_C(4));
	if (OcclusionRows == 0)
		ReportOutOfVoodooMemory();
	OcclusionMaskEnd = OcclusionMask + OcclusionMaskSize;
	OcclusionMaskOrigin = OcclusionMask + (MASK_MARGIN * MASK_WIDTH + MASK_MARGIN);
	OcclusionOriginX = 0;
	OcclusionOriginY = 0;
	/* a table of each row's address */
	row = OcclusionMask;
	table = OcclusionRows;
	count = MASK_ROWS;
	while (count--) {
		PokeLong(table, row);
		row += MASK_WIDTH;
		table += 4;
	}
	FillLinear(OcclusionMask, 0, MASK_BYTES, 0x111);
	/* cell 20 of row 20 */
	OcclusionMaskInset = PeekLong(OcclusionRows + 20 * 4) + 20;
}

void SetOcclusionOrigin(Coord x, Coord y)
{
	OcclusionOriginX = Coord(x.value - 1);
	OcclusionOriginY = Coord(y.value - 1);
}

void ClearOcclusionMask(void)
{
	FillLinear(OcclusionMask, 0, MASK_BYTES, 0x111);
	OccludersPresent = 0;
}

/* Raise the mask over the item, a step at a time up its height. */
void AddOccluder(int16_t off)
{
	objref ref = off;

	if (!OcclusionEnabled)
		return;
	TypeFrame shape = ITEM(ref.off)->typeFrame;
	if (OcclusionTable.test(shape.bits & 0x3ff)) {
		OccludersPresent = 1;
		int16_t width = (GetFootprintX(shape) + 1) * 2;
		int16_t height = (GetFootprintY(shape) + 1) * 2;
		int16_t x = GetDelta(Item_getX(ref), OcclusionOriginX) * 2;
		int16_t y = GetDelta(Item_getY(ref), OcclusionOriginY) * 2;
		int16_t z = Item_getZ(&ref);
		x -= z + width - MASK_MARGIN;
		y -= z + height - MASK_MARGIN;
		int16_t top;
		if ((top = gItemTypeInfo[shape.bits & 0x3ff].height) != 0)
			top--;
		int16_t step = width;
		if (height < width)
			step = height;
		int16_t i;
		for (i = 0; i <= top; i += step) {
			int32_t row = PeekLong(OcclusionRows + y * 4) + x;
			RaiseDepthRect(row, z, width, height);
			x -= step;
			y -= step;
			z += step;
		}
	}
}

uint8_t IsItemOccluded(int16_t off, int8_t flag)
{
	objref ref = off;

	return IsOccludedAt(&ref, Item_getX(ref), Item_getY(ref), Item_getZ(&ref), flag);
}

/* Set up the box IsShapeHidden tests: the shape's footprint and height at x, y, z in mask cells. */
uint8_t IsShapeOccluded(TypeFrame &shape, int16_t x, int16_t y, int16_t z, int8_t flag)
{
	int16_t index;

	if (!OcclusionEnabled || !OccludersPresent)
		return 0;
	OcclusionBoxX = (x - OcclusionOriginX) * 2 - z;
	OcclusionBoxY = (y - OcclusionOriginY) * 2 - z;
	OcclusionBoxZ = z;
	if (OcclusionBoxX < 0 || OcclusionBoxY < 0)
		return 1;
	index = (shape.bits & 0x3ff) - FIRST_OBJECT_TYPE;
	if (OcclusionBoxX - ShapeDimensions[index].x > 80 || OcclusionBoxY - ShapeDimensions[index].y > 50)
		return 1;
	OcclusionBoxHeight = gItemTypeInfo[shape.bits & 0x3ff].height;
	if (OcclusionBoxHeight)
		OcclusionBoxHeight--;
	OcclusionBoxWidth = (GetFootprintX(shape) + 1) * 2 + OcclusionBoxHeight;
	OcclusionBoxLength = (GetFootprintY(shape) + 1) * 2 + OcclusionBoxHeight;
	OcclusionBoxX -= OcclusionBoxWidth - MASK_MARGIN;
	OcclusionBoxY -= OcclusionBoxLength - MASK_MARGIN;
	return IsShapeHidden();
}

void OcclusionStub(void)
{
}

/* two bytes for each of the 874 types from FIRST_OBJECT_TYPE on */
void LoadShpDims(char *name)
{
	DataFile file;
	int8_t unusedFlag = 0;
	int32_t size;

	if (file.open(name, 1) != 1)
		ReportFileNotFound(name);
	size = file.getLength();
	if (size != INT32_C(1748))
		ReportFileReadError(name);
	ShapeDimensions = (ShapeExtent *) AllocateFarHeap(size, 0);
	if (ShapeDimensions == 0)
		ReportOutOfFarMemory();
	if (file.read(ShapeDimensions, size) != size)
		ReportFileReadError(name);
	file.close();
}

extern "C" void ResetCullmaskGlobals(void)
{
	ShapeDimensions = 0;
	WorldMaskObject = 0;
	OcclusionBoxX = 0;
	OcclusionBoxY = 0;
	OcclusionBoxZ = 0;
	OcclusionBoxWidth = 0;
	OcclusionBoxLength = 0;
	OcclusionBoxHeight = 0;
	OcclusionMaskEnd = 0;
	OcclusionMaskSize = 0;
	OcclusionRows = 0;
	OcclusionOriginX = 0;
	OcclusionOriginY = 0;
	OcclusionMask = 0;
	OcclusionMaskOrigin = 0;
	OcclusionMaskInset = 0;
	OccludersPresent = 0;
	OcclusionEnabled = 1;
	CheatKeyWToggle = 1;
	CheatKeyDToggle = 0;
}
