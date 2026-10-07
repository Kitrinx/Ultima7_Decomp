#ifndef ITEMINFO_H
#define ITEMINFO_H

struct objref;

/* An item's location kind, the low three bits of its placement byte. 0 to 3 are the region slot it
 * lies in on the map. */
#define LOCATION_OFF_MAP    4
#define LOCATION_DELETED    5
#define LOCATION_CONTAINED  6
#define LOCATION_EQUIPPED   7

/* An item's placement byte: kind in the low 3 bits, z in the high nibble. */
struct ItemInfo {
	uint8_t flags;
	ItemInfo() {}
	ItemInfo(uint8_t n) { flags = n; }
	uint8_t z() { return (uint8_t)(flags & 0xf0) >> 4; }
	uint8_t kind() { return (int16_t)flags & 7; }
};
extern ItemInfo GetItemZAndStuff(objref *ref);

/* An item's z, from its placement byte. */
inline uint16_t Item_getZ(objref *ref) { return GetItemZAndStuff(ref).z(); }

#endif
