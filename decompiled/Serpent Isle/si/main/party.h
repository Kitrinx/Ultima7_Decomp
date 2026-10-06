#ifndef PARTY_H
#define PARTY_H

struct Coord;
struct PartySaver;

struct objref;
struct Position;
struct CellCoord;

void far SetPartyWorkType(char activity);
extern "C" int far GetPartyIndex(objref member);
void far RemoveFromParty(objref, unsigned char);
void far TeleportParty(CellCoord x, CellCoord y, char z);
void far RecentreFormation();
void far GrowFormation();
void far ShrinkFormation();
void far ResetFormation();
void far AddToParty(objref member, unsigned char quiet);
void far LeaveParty(objref *member);
void far DeactivatePartyMember(char index);
void far ReactivatePartyMember(char index);
unsigned char far FindArrivalSpot(objref member, Position *destination);
void far PlacePartyAround(Coord x, Coord y, char z);
void far ResetPartyFormation();

extern char PartyFileName[];
extern PartySaver PartyFile;

#endif
