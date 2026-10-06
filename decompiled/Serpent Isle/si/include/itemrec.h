#ifndef ITEMREC_H
#define ITEMREC_H

#include "typefram.h"
#include "objref.h"
#include "iteminfo.h"

/* Where an item lies: a cell in its region on the map, or its container. */
union far ItemPosition {
	struct far { unsigned char x, y; } world;
	int parent;
};

/* The 8-byte record in the item buffer. */
struct far ItemRecord {
	int next;
	ItemPosition position;
	unsigned typeFrame;
	union far {
		int extra;
		struct far { unsigned char z, quality; } direct;
	} data;
	void setTypeFrame(TypeFrame value) { typeFrame = value.bits; }
	TypeFrame far &asTypeFrame() { return *(TypeFrame far *)&typeFrame; }
};

/* A record's first six bytes, copied to draw an item without changing it. */
struct ItemSnapshot {
	int next;
	ItemPosition position;
	unsigned typeFrame;
	ItemSnapshot() {}
};

/* The next item in the same list. */
inline objref objref::next() { return ptr()->next; }
inline unsigned objref::type() { return ptr()->typeFrame & 0x3ff; }
inline unsigned objref::frame() { return (ptr()->typeFrame & 0x7c00) >> 10; }
inline unsigned objref::z() { return GetItemZAndStuff(this).z(); }

/* An item's type number. */
inline unsigned GetItemType(objref item) { return item.type(); }

/* Where an item lies: 0 to 3 a map region, then off the map, deleted, contained or equipped. */
inline unsigned char GetItemKind(objref *item)
{
	ItemInfo info = GetItemZAndStuff(item);
	return (unsigned char)((unsigned)info.flags & 7);
}
inline unsigned char IsOnMap(objref *item)
{
	ItemInfo info = GetItemZAndStuff(item);
	return info.kind() <= 3;
}
inline unsigned char IsContained(objref *item)
{
	ItemInfo info = GetItemZAndStuff(item);
	return (unsigned char)((unsigned)info.flags & 7) >= 6;
}
inline unsigned char IsTemporary(objref *item) { return GetItemZAndStuff(item).flags & 8; }

#endif
