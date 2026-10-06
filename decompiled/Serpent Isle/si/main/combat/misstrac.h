#ifndef MISSTRAC_H
#define MISSTRAC_H

#define MISSILE_COUNT 16

struct Coord;
struct ItemId;

void far ResetMissileTrackers();
unsigned char far FireMissileAtCoords(ItemId missile, int weapon, int ammo, ItemId attacker,
	unsigned char damage, Coord targetX, Coord targetY, int targetZ, unsigned char kind);
unsigned char far FireMissileAtItem(ItemId missile, int weapon, int ammo, ItemId attacker, unsigned char damage,
	ItemId target, unsigned char kind);
unsigned char far FireMissileInDirection(ItemId missile, int weapon, int ammo, ItemId attacker,
	unsigned char damage, unsigned char direction, unsigned char kind);
int far GetMissileDistance(ItemId startItem, ItemId targetItem);
int far GetMissileDistanceTo(int item, Coord targetX, Coord targetY, int &targetZ);
void far CheckMissilesForItem(ItemId item);
unsigned char far StopMissile(int index);

int far AllocMissileTracker();

#endif
