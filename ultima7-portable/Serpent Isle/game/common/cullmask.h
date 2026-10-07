#ifndef CULLMASK_H
#define CULLMASK_H

#include "typefram.h"
#include "bitarray.h"

struct ShapeExtent;

struct Coord;
struct WorldMask;

extern WorldMask *WorldMaskObject;
extern int16_t OcclusionBoxX;
extern int16_t OcclusionBoxY;
extern int16_t OcclusionBoxZ;
extern int16_t OcclusionBoxWidth;
extern int16_t OcclusionBoxLength;
extern int16_t OcclusionBoxHeight;
extern int32_t OcclusionMaskEnd;
extern int32_t OcclusionMaskSize;
extern int32_t OcclusionRows;
extern Coord OcclusionOriginX;
extern Coord OcclusionOriginY;
extern int32_t OcclusionMask;
extern int32_t OcclusionMaskOrigin;
extern int32_t OcclusionMaskInset;
uint8_t IsShapeOccluded(TypeFrame &shape, int16_t x, int16_t y, int16_t z, int8_t flag);
inline uint8_t IsShapeOccluded(TypeFrame &&shape, int16_t x, int16_t y, int16_t z, int8_t flag)
{
	return IsShapeOccluded(shape, x, y, z, flag);
}
void SetOcclusionOrigin(Coord x, Coord y);
void ClearOcclusionMask(void);
void AddOccluder(int16_t off);
uint8_t IsItemOccluded(int16_t off, int8_t flag);
void LoadShpDims(char *name);

extern uint8_t OccludersPresent, OcclusionEnabled;
extern uint8_t CheatKeyWToggle, CheatKeyDToggle;
void OcclusionStub(void);

/* the occlusion table, kept in OCCLUDE.DAT */
struct Occlusion : BitArray {
	void save(char *dir);
	void load(char *dir);
};

extern Occlusion OcclusionTable;
void InitOcclusionTable(void);

extern ShapeExtent *ShapeDimensions;

#endif
