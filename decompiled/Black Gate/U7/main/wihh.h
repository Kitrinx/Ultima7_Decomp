#ifndef WIHH_H
#define WIHH_H

struct Hotspot;
struct VoodooAddress;
struct ItemId;

ItemId far GetItemInSlot(ItemId owner, unsigned char slot);

extern int CastingFramesLeft;
extern VoodooAddress WihhTable;
extern char LightSourceStrength[32];
extern int HeldItemFrame[32];
extern char CurrentNpcTint;
extern unsigned char NpcTintColors[];
int far AllocateVoodooBlock(long *address, unsigned size);
struct Hotspot far ReadHotspot(VoodooAddress table, int frame);
void far LoadWihh(char far *name);
char far GetWihhHotspot(unsigned type, int frame, char flip, int *x, int *y);
int far GetLightStrength(int type, int frame, ItemId obj);
void far DrawHeldItem(ItemId obj, int x, int y);
void far SetNpcTint(int obj);
void far CopyWihhEntry(unsigned type, unsigned source);

#endif
