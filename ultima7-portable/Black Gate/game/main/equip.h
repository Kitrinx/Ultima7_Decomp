#ifndef EQUIP_H
#define EQUIP_H

#include "lowlevel.h"
#include "makemojo.h"

/* A linear address in voodoo memory; adding an offset gives another address. */
struct MemoryAddress {
	int32_t address;
	MemoryAddress operator+(int32_t n)
	{
		MemoryAddress r;
		r.address = address + n;
		return r;
	}
	operator int32_t() { return address; }
};

struct ItemId;
#include "typefram.h"
#include "objref.h"

struct EquipmentList;

/* Creature equipment lists: 60-byte records in voodoo memory, the last one read kept in current. */
struct EquipTable {
	int32_t base;
	char current[60];
	int32_t cached, count;
	EquipTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	EquipmentList *get(int32_t n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 60, 60);
			cached = n;
		}
		return (EquipmentList *)current;
	}
	void read(int32_t n, EquipmentList *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 60, 60);
	}
};

extern MemoryAddress EquipList;

uint8_t HasEquipUsecode(objref item);
void InitEquipment(void);
uint8_t EquipItem(ItemId item, ItemId wearer, uint8_t slot, uint8_t combine);
uint8_t CanEquipInSlot(objref item, ItemId wearer, uint8_t slot, uint8_t combine);
void UnequipItem(ItemId item);

/* Creature equipment, defined with the combat code. */
extern char EquipFileName[];
extern EquipTable EquipRecords;
extern uint8_t EquipCount;

void RandomizeCreature(objref ref, TypeFrame &appearance);
inline void RandomizeCreature(objref ref, TypeFrame &&appearance) { RandomizeCreature(ref, appearance); }
void EquipCreatureByType(objref *ref);
uint8_t EquipCreature(objref ref, int16_t monsterIndex);

#endif
