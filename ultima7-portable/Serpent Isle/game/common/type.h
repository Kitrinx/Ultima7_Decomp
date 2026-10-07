#ifndef TYPE_H
#define TYPE_H

#ifdef __cplusplus
#include "objref.h"
#endif

/* A type's three flag bytes in TFA.DAT. File records keep each 8-bit group of bit-fields on a
 * uint8_t: on a wider type, the Microsoft layout (Windows) starts a new unit where GCC and clang
 * elsewhere pack on, and the record grows. */
struct TypeInfo {
	uint8_t hasSfx:1, strangeMovement:1, animated:1, solid:1, water:1, height:3;
	uint8_t typeClass:4, field:1, door:1, bargePart:1, transparent:1;
	uint8_t footprintX:3, footprintY:3, light:1, translucent:1;
};
#ifdef __cplusplus
static_assert(sizeof(TypeInfo) == 3, "TypeInfo is read from a 3-byte file record");
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern struct TypeInfo gItemTypeInfo[];

void LoadTfa(char *buf, char *name);
int16_t GetTypeAnimation(uint16_t type);
extern char *TypeAnimations;
extern struct TypeInfo gItemTypeInfo[1024];
#ifdef __cplusplus
}
#endif

/* A type's footprint, x and y swapped when flipped. C++ callers pass the word as a TypeFrame. */
#ifdef __cplusplus
struct TypeFrame;
extern "C" {
uint16_t GetFootprintX(TypeFrame &typeFrame);
uint16_t GetFootprintY(TypeFrame &typeFrame);
}
inline uint16_t GetFootprintX(TypeFrame &&typeFrame) { return GetFootprintX(typeFrame); }
inline uint16_t GetFootprintY(TypeFrame &&typeFrame) { return GetFootprintY(typeFrame); }
#else
uint16_t GetFootprintX(uint16_t *typeFrame);
uint16_t GetFootprintY(uint16_t *typeFrame);
#endif

/* type classes, TypeInfo's typeClass */
#define TYPE_CLASS_NONE         0
#define TYPE_CLASS_QUANTITY     3
#define TYPE_CLASS_EGG          7
#define TYPE_CLASS_SPELLBOOK    8
#define TYPE_CLASS_BARGE        9
#define TYPE_CLASS_VIRTUE_STONE 11
#define TYPE_CLASS_MONSTER      12
#define TYPE_CLASS_HUMAN        13
#define TYPE_CLASS_BUILDING     14

/* what an item of a class has, one word per class */
extern const uint16_t ItemTypeClassFlags[16];

#define CLASS_RECORDS           0x003   /* its extra records */
#define CLASS_REGION            0x004
#define CLASS_QUALITY           0x008
#define CLASS_QUANTITY          0x010
#define CLASS_HIT_POINTS        0x020
#define CLASS_QUALITY_FLAGS     0x040
#define CLASS_CONTENTS          0x080
#define CLASS_NPC               0x100

#ifdef __cplusplus
/* What an item's type says about it. */
inline uint8_t objref::isNpc()
{
	return (ItemTypeClassFlags[gItemTypeInfo[type()].typeClass] & CLASS_NPC) != 0;
}
inline uint8_t objref::isBody() { return gItemTypeInfo[type()].typeClass == TYPE_CLASS_HUMAN; }
inline uint8_t objref::isContainer()
{
	return ItemTypeClassFlags[gItemTypeInfo[type()].typeClass] & CLASS_CONTENTS;
}
inline uint8_t objref::multipart() { return gItemTypeInfo[type()].strangeMovement; }
#endif


#endif
