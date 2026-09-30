#ifndef OBJREF_H
#define OBJREF_H


/* far, so its members take a far this. */
struct ItemRecord;
struct Coord;
struct objref;
#include "itembase.h"

/* An item passed by value: one word on the stack. */
struct ItemId {
	int16_t off;
	uint8_t valid() { return off != 0; }
	uint8_t operator!=(const ItemId &other) { return off != other.off; }
	uint8_t operator!=(const objref &other);
	ItemId &operator=(int16_t n) { off = n; return *this; }
	ItemId &operator=(const objref &other);
};

/* An item's offset in the item buffer; zero is no item. */
struct objref {
	int16_t off;
	objref() {}
	objref(int16_t n) { off = n; }
	objref(ItemId id) { off = id.off; }
	objref &operator=(const objref &other) { off = other.off; return *this; }
	objref &operator=(int16_t n) { off = n; return *this; }
	operator int16_t() { return off; }
	operator ItemId() { ItemId id; id.off = off; return id; }
	uint8_t operator==(objref other) { return off == other.off; }
	uint8_t operator==(int32_t n) { return off == (int16_t)n; }
	uint8_t operator!=(objref other) { return off != other.off; }
	uint8_t operator!=(int32_t n) { return off != (int16_t)n; }
	uint8_t valid() { return off != 0; }
	ItemRecord *ptr() const { return (ItemRecord *)ItemAt(off); }

	/* Defined in itemrec.h. */
	objref next();
	uint16_t type();
	uint16_t frame();
	uint16_t z();

	/* Defined in type.h. */
	uint8_t isNpc();
	uint8_t isBody();
	uint8_t isContainer();
	uint8_t multipart();

	/* Defined in itable.c. */
	uint8_t cellX();
	void cellX(int16_t x);
	uint8_t cellY();
	void cellY(int16_t y);
	uint8_t region();
	void region(uint8_t r);
	Coord worldX();
	Coord worldY();
	void position(Coord x, Coord y);
};

inline uint8_t ItemId::operator!=(const objref &other) { return off != other.off; }
inline ItemId &ItemId::operator=(const objref &other) { off = other.off; return *this; }

#endif
