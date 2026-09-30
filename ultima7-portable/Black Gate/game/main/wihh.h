#ifndef WIHH_H
#define WIHH_H

struct Hotspot;
struct VoodooAddress;
struct ItemId;

ItemId GetItemInSlot(ItemId owner, uint8_t slot);

extern int16_t CastingFramesLeft;
extern VoodooAddress WihhTable;
extern char LightSourceStrength[32];
extern int16_t HeldItemFrame[32];
extern int8_t CurrentNpcTint;
extern uint8_t NpcTintColors[];
int16_t AllocateVoodooBlock(int32_t *address, uint16_t size);
struct Hotspot ReadHotspot(VoodooAddress table, int16_t frame);
void LoadWihh(char *name);
int8_t GetWihhHotspot(uint16_t type, int16_t frame, int8_t flip, int16_t *x, int16_t *y);
int16_t GetLightStrength(int16_t type, int16_t frame, ItemId obj);
void DrawHeldItem(ItemId obj, int16_t x, int16_t y);
void SetNpcTint(int16_t obj);
void CopyWihhEntry(uint16_t type, uint16_t source);

#endif
