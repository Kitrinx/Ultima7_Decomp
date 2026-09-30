#ifndef TYPE_H
#define TYPE_H

#ifdef __cplusplus
#include "objref.h"
#endif

/* A type's three flag bytes in TFA.DAT. */
struct TypeInfo {
	unsigned hasSfx:1, strangeMovement:1, animated:1, solid:1, water:1, height:3;
	unsigned typeClass:4, field:1, door:1, bargePart:1, transparent:1;
	unsigned footprintX:3, footprintY:3, light:1, translucent:1;
};

extern struct TypeInfo gItemTypeInfo[];

#ifdef __cplusplus
extern "C" {
#endif
void LoadTfa(char *buf, char *name);
int GetTypeAnimation(unsigned type);
#ifdef __cplusplus
}
#endif

/* A type's footprint, x and y swapped when flipped. C++ callers pass the word as a TypeFrame. */
#ifdef __cplusplus
struct far TypeFrame;
extern "C" {
unsigned GetFootprintX(TypeFrame far &typeFrame);
unsigned GetFootprintY(TypeFrame far &typeFrame);
}
#else
unsigned GetFootprintX(unsigned far *typeFrame);
unsigned GetFootprintY(unsigned far *typeFrame);
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
extern unsigned ItemTypeClassFlags[16];

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
inline unsigned char objref::isNpc()
{
	return (ItemTypeClassFlags[gItemTypeInfo[type()].typeClass] & CLASS_NPC) != 0;
}
inline unsigned char objref::isBody() { return gItemTypeInfo[type()].typeClass == TYPE_CLASS_HUMAN; }
inline unsigned char objref::isContainer()
{
	return ItemTypeClassFlags[gItemTypeInfo[type()].typeClass] & CLASS_CONTENTS;
}
inline unsigned char objref::multipart() { return gItemTypeInfo[type()].strangeMovement; }
#endif

#endif
