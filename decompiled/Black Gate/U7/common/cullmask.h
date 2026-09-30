#ifndef CULLMASK_H
#define CULLMASK_H

#include "typefram.h"
#include "bitarray.h"

struct ShapeExtent;

struct Coord;
struct WorldMask;

extern WorldMask *WorldMaskObject;
extern int OcclusionBoxX;
extern int OcclusionBoxY;
extern int OcclusionBoxZ;
extern int OcclusionBoxWidth;
extern int OcclusionBoxLength;
extern int OcclusionBoxHeight;
extern long OcclusionMaskEnd;
extern long OcclusionMaskSize;
extern long OcclusionRows;
extern Coord OcclusionOriginX;
extern Coord OcclusionOriginY;
extern long OcclusionMask;
extern long OcclusionMaskOrigin;
extern long OcclusionMaskInset;
unsigned char IsShapeOccluded(TypeFrame far &shape, int x, int y, int z, char flag);
void SetOcclusionOrigin(Coord x, Coord y);
void ClearOcclusionMask(void);
void AddOccluder(int off);
unsigned char IsItemOccluded(int off, char flag);
void LoadShpDims(char *name);

extern unsigned char OccludersPresent, OcclusionEnabled;
extern unsigned char CheatKeyWToggle, CheatKeyDToggle;
void OcclusionStub(void);

/* the occlusion table, kept in OCCLUDE.DAT */
struct Occlusion : BitArray {
	void save(char *dir);
	void load(char *dir);
};

extern Occlusion OcclusionTable;
void InitOcclusionTable(void);

extern ShapeExtent far *ShapeDimensions;

#endif
