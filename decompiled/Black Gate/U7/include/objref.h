#ifndef OBJREF_H
#define OBJREF_H

#ifndef MK_FP
#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))
#endif

/* far, so its members take a far this. */
struct far ItemRecord;
struct Coord;
struct objref;
extern unsigned ItemBufferSegment;

/* An item passed by value: one word on the stack. */
struct ItemId {
	int off;
	unsigned char valid() { return off != 0; }
	unsigned char operator!=(const ItemId &other) { return off != other.off; }
	unsigned char operator!=(const objref &other);
	ItemId &operator=(int n) { off = n; return *this; }
	ItemId &operator=(const objref &other);
};

/* An item's offset in the item buffer; zero is no item. */
struct objref {
	int off;
	objref() {}
	objref(int n) { off = n; }
	objref(ItemId id) { off = id.off; }
	objref &operator=(const objref &other) { off = other.off; return *this; }
	objref &operator=(int n) { off = n; return *this; }
	operator int() { return off; }
	operator ItemId() { ItemId id; id.off = off; return id; }
	unsigned char operator==(objref other) { return off == other.off; }
	unsigned char operator!=(objref other) { return off != other.off; }
	unsigned char valid() { return off != 0; }
	ItemRecord far *ptr() { return (ItemRecord far *)MK_FP(ItemBufferSegment, off); }

	/* Defined in itemrec.h. */
	objref next();
	unsigned type();
	unsigned frame();
	unsigned z();

	/* Defined in type.h. */
	unsigned char isNpc();
	unsigned char isBody();
	unsigned char isContainer();
	unsigned char multipart();

	/* Defined in itable.c. */
	unsigned char cellX();
	void cellX(int x);
	unsigned char cellY();
	void cellY(int y);
	unsigned char region();
	void region(unsigned char r);
	Coord worldX();
	Coord worldY();
	void position(Coord x, Coord y);
};

inline unsigned char ItemId::operator!=(const objref &other) { return off != other.off; }
inline ItemId &ItemId::operator=(const objref &other) { off = other.off; return *this; }

#endif
