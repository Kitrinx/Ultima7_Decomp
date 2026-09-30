#ifndef PARTY_H
#define PARTY_H

struct Coord;
struct PartySaver;

struct objref;
struct Position;
struct CellCoord;

void SetPartyWorkType(int8_t activity);
extern "C" int16_t GetPartyIndex(objref member);
void RemoveFromParty(objref, uint8_t);
void TeleportParty(CellCoord x, CellCoord y, int8_t z);
void RecentreFormation();
void GrowFormation();
void ShrinkFormation();
void ResetFormation();
void AddToParty(objref member, uint8_t quiet);
void LeaveParty(objref *member);
void DeactivatePartyMember(int8_t index);
void ReactivatePartyMember(int8_t index);
uint8_t FindArrivalSpot(objref member, Position *destination);
void PlacePartyAround(Coord x, Coord y, int8_t z);
void ResetPartyFormation();

extern char PartyFileName[];
extern PartySaver PartyFile;

#endif
