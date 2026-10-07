/* Serpent Isle SI.EXE, overlay segment 255 (file offsets 0x06ed80 to 0x06f058, 728 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "dosio.h"
#include "item.h"
#include "memapi.h"
#include "npcref.h"
#include "type.h"

#define ITEM_TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[ITEM_TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define HAS_CONTENTS(rec) ((int8_t) (CLASS_FLAGS(rec) & CLASS_CONTENTS))

#define TYPE_COUNT 1024

#define GOLD_COIN 644
#define REAGENT 842

/* weight and bulk of a type */
struct WeightVolume {
	uint8_t weight;
	uint8_t bulk;
};

WeightVolume *TypeWeightVolumes;

inline int8_t IsTypeClass(NPCRef r, int16_t typeClass)
{
	return TYPE_CLASS(ITEM(r.off)) == (uint16_t)typeClass;
}

int16_t GetItemTypeWeight(int16_t type)
{
	return TypeWeightVolumes[type].weight;
}

/* a stack weighs its quantity times its type's weight, a tenth of that for coins and reagents */
int16_t Item_getWeight(objref item)
{
	uint16_t type;
	int8_t stackable;
	int16_t weight;

	type = ITEM_TYPE(ITEM(item.off));
	stackable = gItemTypeInfo[type].typeClass == TYPE_CLASS_QUANTITY;
	weight = GetItemTypeWeight(type);
	if (stackable) {
		weight *= (uint8_t)Item_getQuantity(&item);
		if (type == GOLD_COIN || type == REAGENT || type == 951 || type == 948 || type == 952)
			weight /= 10;
		if (weight == 0)
			weight = 1;
	}
	return weight;
}

int16_t GetItemTypeBulk(int16_t type)
{
	return TypeWeightVolumes[type].bulk;
}

void LoadWgtVol(char *name)
{
	int16_t fd;

	TypeWeightVolumes = (WeightVolume *) AllocateFarHeap(TYPE_COUNT * sizeof(WeightVolume), 0);
	if (TypeWeightVolumes == 0)
		ReportError(0xeb01);
	fd = DosOpen(name);
	if (fd < 0)
		ReportError(0xeb02);
	DosRead(fd, INT32_C(0), TYPE_COUNT * sizeof(WeightVolume), TypeWeightVolumes);
	DosClose(fd);
}

int16_t DetermineWeightOfContents(objref item)
{
	int16_t weight = 0;

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

int16_t DetermineBulkOfContents(objref item)
{
	int16_t bulk = 0;

	if (item.valid() && (IsTypeClass(item, TYPE_CLASS_HUMAN) || HAS_CONTENTS(ITEM(item.off)))) {
		objref child = GetContainedItem(&item);

		while (child.valid()) {
			bulk += GetItemTypeBulk(ITEM_TYPE(ITEM(child.off)));
			child = child.next();
		}
	}
	return bulk;
}

extern "C" void ResetWeightGlobals(void)
{
	TypeWeightVolumes = 0;
}
