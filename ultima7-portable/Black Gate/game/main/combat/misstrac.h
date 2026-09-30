#ifndef MISSTRAC_H
#define MISSTRAC_H

#define MISSILE_COUNT 16

struct Coord;
struct ItemId;

void ResetMissileTrackers();
uint8_t FireMissileAtCoords(ItemId missile, int16_t weapon, int16_t ammo, ItemId attacker,
	uint8_t damage, Coord targetX, Coord targetY, int16_t targetZ, uint8_t kind);
uint8_t FireMissileAtItem(ItemId missile, int16_t weapon, int16_t ammo, ItemId attacker, uint8_t damage,
	ItemId target, uint8_t kind);
uint8_t FireMissileInDirection(ItemId missile, int16_t weapon, int16_t ammo, ItemId attacker,
	uint8_t damage, uint8_t direction, uint8_t kind);
int16_t GetMissileDistance(ItemId startItem, ItemId targetItem);
int16_t GetMissileDistanceTo(int16_t item, Coord targetX, Coord targetY, int16_t &targetZ);
void CheckMissilesForItem(ItemId item);
uint8_t StopMissile(int16_t index);

int16_t AllocMissileTracker();

#endif
