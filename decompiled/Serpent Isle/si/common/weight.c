/* Serpent Isle SI.EXE, overlay segment 255 (file offsets 0x06ed80 to 0x06f058, 728 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "dosio.h"
#include "item.h"
#include "memapi.h"
#include "npcref.h"
#include "type.h"

#define ITEM_TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[ITEM_TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define HAS_CONTENTS(rec) ((char) (CLASS_FLAGS(rec) & CLASS_CONTENTS))

#define TYPE_COUNT 1024

#define GOLD_COIN 644
#define REAGENT 842

/* weight and bulk of a type */
struct WeightVolume {
	unsigned char weight;
	unsigned char bulk;
};

WeightVolume far *TypeWeightVolumes;

inline char IsTypeClass(NPCRef r, int typeClass)
{
	return TYPE_CLASS(ITEM(r.off)) == typeClass;
}

int far GetItemTypeWeight(int type)
{
	return TypeWeightVolumes[type].weight;
}

/* a stack weighs its quantity times its type's weight, a tenth of that for coins and reagents */
int far Item_getWeight(objref item)
{
	unsigned type;
	char stackable;
	int weight;

	type = ITEM_TYPE(ITEM(item.off));
	stackable = gItemTypeInfo[type].typeClass == TYPE_CLASS_QUANTITY;
	weight = GetItemTypeWeight(type);
	if (stackable) {
		weight *= (unsigned char)Item_getQuantity(&item);
		if (type == GOLD_COIN || type == REAGENT || type == 951 || type == 948 || type == 952)
			weight /= 10;
		if (weight == 0)
			weight = 1;
	}
	return weight;
}

int far GetItemTypeBulk(int type)
{
	return TypeWeightVolumes[type].bulk;
}

void far LoadWgtVol(char far *name)
{
	int fd;

	TypeWeightVolumes = (WeightVolume far *) AllocateFarHeap(TYPE_COUNT * sizeof(WeightVolume), 0);
	if (TypeWeightVolumes == 0)
		ReportError(0xeb01);
	fd = DosOpen(name);
	if (fd < 0)
		ReportError(0xeb02);
	DosRead(fd, 0L, TYPE_COUNT * sizeof(WeightVolume), TypeWeightVolumes);
	DosClose(fd);
}

int far DetermineWeightOfContents(objref item)
{
	int weight = 0;

	if (item.valid() && (IsTypeClass(item, TYPE_CLASS_HUMAN) || HAS_CONTENTS(ITEM(item.off)))) {
		objref child = GetContainedItem(&item);

		while (child.valid()) {
			weight += Item_getWeight(child);
			weight += DetermineWeightOfContents(child);
			child = child.next();
		}
	}
	return weight;
}

int far DetermineBulkOfContents(objref item)
{
	int bulk = 0;

	if (item.valid() && (IsTypeClass(item, TYPE_CLASS_HUMAN) || HAS_CONTENTS(ITEM(item.off)))) {
		objref child = GetContainedItem(&item);

		while (child.valid()) {
			bulk += GetItemTypeBulk(ITEM_TYPE(ITEM(child.off)));
			child = child.next();
		}
	}
	return bulk;
}
