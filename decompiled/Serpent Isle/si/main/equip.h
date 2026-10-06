#ifndef EQUIP_H
#define EQUIP_H

#include "lowlevel.h"
#include "makemojo.h"

/* A linear address in voodoo memory; adding an offset gives another address. */
struct MemoryAddress {
	long address;
	MemoryAddress operator+(unsigned n)
	{
		MemoryAddress r;
		r.address = address + n;
		return r;
	}
	operator long() { return address; }
};

struct ItemId;
struct objref;
struct far TypeFrame;
struct EquipmentList;

/* Creature equipment lists: 60-byte records in voodoo memory, the last one read kept in current. */
struct EquipTable {
	long base;
	char current[60];
	long cached, count;
	EquipTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	EquipmentList *get(long n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 60, 60);
			cached = n;
		}
		return (EquipmentList *)current;
	}
	void read(long n, EquipmentList *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 60, 60);
	}
};

extern MemoryAddress EquipList;

void QueueUnequipScript(ItemId item);
unsigned char HasEquipUsecode(objref item);
void InitEquipment(void);
unsigned char EquipItem(ItemId item, ItemId wearer, unsigned char slot, unsigned char combine);
void RunEquipUsecode(objref item);
unsigned char CanEquipInSlot(objref item, ItemId wearer, unsigned char slot, unsigned char combine);
void UnequipItem(ItemId item);

/* Creature equipment, defined with the combat code. */
extern char EquipFileName[];
extern EquipTable EquipRecords;
extern unsigned char EquipCount;

void far RandomizeCreature(objref ref, TypeFrame far &appearance);
void far EquipCreatureByType(objref *ref);
unsigned char far EquipCreature(objref ref, int monsterIndex);

#endif
